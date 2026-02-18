#include <benchmark/benchmark.h>
#include "roc_audio/pcm_encoder.h"
#include "roc_audio/pcm_decoder.h"
#include "roc_audio/sample_spec.h"
#include "roc_audio/channel_set.h"
#include "roc_audio/channel_defs.h"
#include <vector>
#include <cstdint>

using namespace roc::audio;

static void bench_pcm_encoder(benchmark::State& s) {
    ChannelSet channels(
        ChanLayout_Surround,
        ChanOrder_Smpte,
        ChanMask_Surround_Stereo
    );

    SampleSpec spec(
        48000,
        PcmFormat_Float32,
        channels
    );

    PcmEncoder enc(spec);

    const size_t samples_per_channel = 48000;
    const size_t num_channels = channels.num_channels();
    const size_t total_samples = samples_per_channel * num_channels;

    std::vector<float> samples(total_samples, 0.0f);

    // encoded_byte_count expects samples per channel
    size_t frame_size = enc.encoded_byte_count(samples_per_channel);
    std::vector<uint8_t> frame(frame_size);

    for (auto _ : s) {
        enc.begin(frame.data(), frame_size);
        enc.write(samples.data(), total_samples);
        enc.end();
    }

    s.SetItemsProcessed(int64_t(s.iterations()) * int64_t(total_samples));
}

static void bench_pcm_decoder(benchmark::State& s) {
    ChannelSet channels(
        ChanLayout_Surround,
        ChanOrder_Smpte,
        ChanMask_Surround_Stereo
    );

    SampleSpec spec(
        48000,
        PcmFormat_Float32,
        channels
    );

    PcmDecoder dec(spec);

    const size_t samples_per_channel = 48000;
    const size_t num_channels = channels.num_channels();
    const size_t total_samples = samples_per_channel * num_channels;

    std::vector<float> samples(total_samples, 0.0f);

    // For PCM float32 encoded frame size (bytes) = samples_per_channel * num_channels * sizeof(float)
    size_t frame_size = samples_per_channel * num_channels * sizeof(float);
    std::vector<uint8_t> frame(frame_size);

    for (auto _ : s) {
        dec.begin(0, frame.data(), frame.size());
        dec.read(samples.data(), total_samples);
        dec.end();
    }

    s.SetItemsProcessed(int64_t(s.iterations()) * int64_t(total_samples));
}

BENCHMARK(bench_pcm_encoder);
BENCHMARK(bench_pcm_decoder);
