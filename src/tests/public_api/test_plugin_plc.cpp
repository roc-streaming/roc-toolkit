/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/context.h"
#include "test_helpers/proxy.h"
#include "test_helpers/receiver.h"
#include "test_helpers/sender.h"

#include "roc_core/atomic.h"
#include "roc_core/panic.h"
#include "roc_fec/codec_map.h"

#include "roc/config.h"
#include "roc/plugin.h"

namespace roc {
namespace api {

namespace {

enum {
    SampleRate = 44100,
    Magic = 123456789,
    NumChans = 2,
    LookaheadSamples = 10,
    PluginID = ROC_PLUGIN_ID_MIN + 10
};

const float SampleStep = 1. / 32768.;

struct TestPlugin {
    // First field, so we can cast roc_plugin_plc* to TestPlugin*.
    roc_plugin_plc func_table;

    // Magic constant to ensure that all pointer casts work fine.
    int magic;

    core::Atomic<size_t> n_created;
    core::Atomic<size_t> n_deleted;

    core::Atomic<size_t> n_hist_samples;
    core::Atomic<size_t> n_lost_samples;

    TestPlugin();
};

struct TestPlc {
    TestPlugin* plugin;
    float last_sample;

    TestPlc(TestPlugin* plugin)
        : plugin(plugin)
        , last_sample(0) {
        roc_panic_if_not(plugin);
        roc_panic_if_not(plugin->magic == Magic);

        plugin->n_created++;
    }

    ~TestPlc() {
        roc_panic_if_not(plugin);
        roc_panic_if_not(plugin->magic == Magic);

        plugin->n_deleted++;
    }
};

void* test_plc_new(roc_plugin_plc* plugin, const roc_media_encoding* encoding) {
    roc_panic_if_not(plugin);
    roc_panic_if_not(encoding);

    roc_panic_if_not(encoding->format == ROC_FORMAT_PCM);
    roc_panic_if_not(encoding->subformat == ROC_SUBFORMAT_PCM_FLOAT32);
    roc_panic_if_not(encoding->rate == SampleRate);
    roc_panic_if_not(encoding->channels == ROC_CHANNEL_LAYOUT_STEREO);

    return new TestPlc((TestPlugin*)plugin);
}

void test_plc_delete(void* plugin_instance) {
    TestPlc* plc = (TestPlc*)plugin_instance;

    roc_panic_if_not(plc);
    roc_panic_if_not(plc->plugin);
    roc_panic_if_not(plc->plugin->magic == Magic);

    delete plc;
}

unsigned int test_plc_lookahead_len(void* plugin_instance) {
    TestPlc* plc = (TestPlc*)plugin_instance;

    roc_panic_if_not(plc);
    roc_panic_if_not(plc->plugin);
    roc_panic_if_not(plc->plugin->magic == Magic);

    return LookaheadSamples;
}

void test_plc_process_history(void* plugin_instance, const roc_frame* history_frame) {
    TestPlc* plc = (TestPlc*)plugin_instance;

    roc_panic_if_not(plc);
    roc_panic_if_not(plc->plugin);
    roc_panic_if_not(plc->plugin->magic == Magic);

    roc_panic_if_not(history_frame);
    roc_panic_if_not(history_frame->samples != NULL);
    roc_panic_if_not(history_frame->samples_size > 0);

    const float* hist_samples = (const float*)history_frame->samples;
    const size_t hist_sample_count =
        history_frame->samples_size / sizeof(float) / NumChans;

    plc->last_sample = hist_samples[hist_sample_count * NumChans - 1];

    // update stats shared by all plugin instances
    plc->plugin->n_hist_samples += hist_sample_count;
}

void test_plc_process_loss(void* plugin_instance,
                           roc_frame* lost_frame,
                           const roc_frame* lookahead_frame) {
    TestPlc* plc = (TestPlc*)plugin_instance;

    roc_panic_if_not(plc);
    roc_panic_if_not(plc->plugin);
    roc_panic_if_not(plc->plugin->magic == Magic);

    roc_panic_if_not(lost_frame);
    roc_panic_if_not(lost_frame->samples != NULL);
    roc_panic_if_not(lost_frame->samples_size > 0);

    roc_panic_if_not(lookahead_frame);
    roc_panic_if_not(
        (lost_frame->samples != NULL && lost_frame->samples_size > 0)
        || (lookahead_frame->samples == NULL && lookahead_frame->samples_size == 0));

    float* lost_samples = (float*)lost_frame->samples;
    const size_t lost_sample_count = lost_frame->samples_size / sizeof(float) / NumChans;

    for (size_t ns = 0; ns < lost_sample_count; ns += NumChans) {
        // test::Sender generates an incrementing sequence of samples, so
        // we can easily restore original samples.
        plc->last_sample = test::increment_sample_value(plc->last_sample, SampleStep);

        for (size_t nc = 0; nc < NumChans; nc++) {
            lost_samples[ns + nc] = plc->last_sample;
        }
    }

    if (lookahead_frame->samples_size > 0) {
        // check that lost frame fit perfectly
        const float* lookahead_samples = (const float*)lookahead_frame->samples;
        roc_panic_if_not(lookahead_samples[0] != plc->last_sample);
    }

    // update stats shared by all plugin instances
    plc->plugin->n_lost_samples += lost_sample_count;
}

TestPlugin::TestPlugin() {
    magic = Magic;

    func_table.new_cb = &test_plc_new;
    func_table.delete_cb = &test_plc_delete;
    func_table.lookahead_len_cb = &test_plc_lookahead_len;
    func_table.process_history_cb = &test_plc_process_history;
    func_table.process_loss_cb = &test_plc_process_loss;
}

} // namespace

TEST_GROUP(plugin_plc) {
    roc_sender_config sender_conf;
    roc_receiver_config receiver_conf;

    void setup() {
        memset(&sender_conf, 0, sizeof(sender_conf));
        sender_conf.frame_encoding.format = ROC_FORMAT_PCM;
        sender_conf.frame_encoding.subformat = ROC_SUBFORMAT_PCM_FLOAT32;
        sender_conf.frame_encoding.rate = SampleRate;
        sender_conf.frame_encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;

        sender_conf.packet_encoding = ROC_PACKET_ENCODING_AVP_L16_STEREO;
        sender_conf.packet_length = test::PacketSamples * 1000000000ull / SampleRate;

        sender_conf.fec_encoding = ROC_FEC_ENCODING_RS8M;
        sender_conf.fec_block_source_packets = test::SourcePackets;
        sender_conf.fec_block_repair_packets = test::RepairPackets;

        sender_conf.clock_source = ROC_CLOCK_SOURCE_INTERNAL;

        memset(&receiver_conf, 0, sizeof(receiver_conf));
        receiver_conf.frame_encoding.format = ROC_FORMAT_PCM;
        receiver_conf.frame_encoding.subformat = ROC_SUBFORMAT_PCM_FLOAT32;
        receiver_conf.frame_encoding.rate = SampleRate;
        receiver_conf.frame_encoding.channels = ROC_CHANNEL_LAYOUT_STEREO;

        receiver_conf.clock_source = ROC_CLOCK_SOURCE_INTERNAL;

        // enable PLC plugin
        receiver_conf.plc_backend = (roc_plc_backend)PluginID;

        receiver_conf.latency_tuner_profile = ROC_LATENCY_TUNER_PROFILE_INTACT;
        receiver_conf.target_latency = test::Latency * 1000000000ull / SampleRate;
        receiver_conf.latency_tolerance =
            test::Latency * 1000000000ull / SampleRate * 10000;
        receiver_conf.no_playback_timeout =
            test::Timeout * 1000000000ull / SampleRate * 10000;
    }

    bool is_rs8m_supported() {
        return fec::CodecMap::instance().has_scheme(packet::FEC_ReedSolomon_M8);
    }
};

// Enable FEC + PLC (custom plugin).
// Lose some source packets.
// Check that all all packets were restored by FEC and not by PLC.
TEST(plugin_plc, losses_restored_by_fec) {
    if (!is_rs8m_supported()) {
        return;
    }

    enum { Flags = test::FlagRS8M | test::FlagLoseSomePkts };

    TestPlugin plugin;

    {
        test::Context context;

        // register PLC plugin
        context.register_plc_plugin(PluginID, &plugin.func_table);

        test::Receiver receiver(context, receiver_conf, SampleStep, NumChans,
                                test::FrameSamples, Flags);

        receiver.bind();

        test::Proxy proxy(receiver.source_endpoint(), receiver.repair_endpoint(),
                          test::SourcePackets, test::RepairPackets, Flags);

        test::Sender sender(context, sender_conf, SampleStep, NumChans,
                            test::FrameSamples, Flags);

        sender.connect(proxy.source_endpoint(), proxy.repair_endpoint(), NULL);

        CHECK(sender.start());
        receiver.receive();
        sender.stop();
        sender.join();

        // some packets were lost
        CHECK(proxy.n_dropped_packets() > 0);
    }

    // one plugin instance was created and deleted
    LONGS_EQUAL(1, plugin.n_created);
    LONGS_EQUAL(1, plugin.n_deleted);

    // PLC got history frames
    CHECK(plugin.n_hist_samples > 0);
    // but PLC was not asked to fill losses
    CHECK(plugin.n_lost_samples == 0);
}

// Enable FEC + PLC (custom plugin).
// Lose some source packets + lose all repair packets.
// Check that PLC was used to restore packets.
TEST(plugin_plc, losses_restored_by_plc) {
    if (!is_rs8m_supported()) {
        return;
    }

    enum {
        Flags = test::FlagRS8M | test::FlagLoseSomePkts | test::FlagLoseAllRepairPkts
    };

    TestPlugin plugin;

    {
        test::Context context;

        // register PLC plugin
        context.register_plc_plugin(PluginID, &plugin.func_table);

        test::Receiver receiver(context, receiver_conf, SampleStep, NumChans,
                                test::FrameSamples, Flags);

        receiver.bind();

        test::Proxy proxy(receiver.source_endpoint(), receiver.repair_endpoint(),
                          test::SourcePackets, test::RepairPackets, Flags);

        test::Sender sender(context, sender_conf, SampleStep, NumChans,
                            test::FrameSamples, Flags);

        sender.connect(proxy.source_endpoint(), proxy.repair_endpoint(), NULL);

        CHECK(sender.start());
        receiver.receive();
        sender.stop();
        sender.join();

        // some packets were lost
        CHECK(proxy.n_dropped_packets() > 0);
    }

    // one plugin instance was created and deleted
    LONGS_EQUAL(1, plugin.n_created);
    LONGS_EQUAL(1, plugin.n_deleted);

    // PLC got history frames
    CHECK(plugin.n_hist_samples > 0);
    // and PLC was asked to fill losses
    CHECK(plugin.n_lost_samples > 0);
}

} // namespace api
} // namespace roc
