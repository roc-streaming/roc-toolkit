/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/sox_reader.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_sndio/sox_controller.h"

namespace roc {
namespace sndio {

namespace {

void add_effect(sox_effects_chain_t* chain,
                const char* name,
                sox_signalinfo_t* in,
                sox_signalinfo_t* out,
                int argc,
                const char* argv[]) {
    const sox_effect_handler_t* handler = sox_find_effect(name);
    if (!handler) {
        roc_panic("sox reader: sox_find_effect(): can't find '%s' effect", name);
    }

    sox_effect_t* effect = sox_create_effect(handler);
    if (!effect) {
        roc_panic("sox reader: sox_create_effect(): can't create '%s' effect", name);
    }

    int err = sox_effect_options(effect, argc, const_cast<char**>(argv));
    if (err != SOX_SUCCESS) {
        roc_panic("sox reader: sox_effect_options(): can't set '%s' effect options: %s",
                  name, sox_strerror(err));
    }

    err = sox_add_effect(chain, effect, in, out);
    if (err != SOX_SUCCESS) {
        roc_panic("sox reader: sox_add_effect(): can't add gain effect: %s",
                  sox_strerror(err));
    }

    free(effect);
}

} // namespace

const sox_effect_handler_t SoxReader::output_handler_ = {
    "roc_output",          //
    NULL,                  //
    SOX_EFF_MCHAN,         //
    NULL,                  //
    NULL,                  //
    SoxReader::output_cb_, //
    NULL,                  //
    NULL,                  //
    SoxReader::kill_cb_,   //
    0                      //
};

SoxReader::SoxReader(core::BufferPool<audio::sample_t>& buffer_pool,
                     packet::channel_mask_t channels,
                     size_t n_samples,
                     size_t sample_rate)
    : input_(NULL)
    , chain_(NULL)
    , buffer_pool_(buffer_pool)
    , is_file_(false) {
    size_t n_channels = packet::num_channels(channels);
    if (n_channels == 0) {
        roc_panic("sox reader: # of channels is zero");
    }

    if (n_samples == 0) {
        n_samples = SoxController::instance().get_globals().bufsiz / n_channels;
    }

    buffer_size_ = n_samples * n_channels;
    buffer_pos_ = 0;

    n_bufs_ = 0;

    memset(&out_signal_, 0, sizeof(out_signal_));
    out_signal_.rate = sample_rate;
    out_signal_.channels = (unsigned)n_channels;
    out_signal_.precision = SOX_SAMPLE_PRECISION;
}

SoxReader::~SoxReader() {
    if (joinable()) {
        roc_panic("sox reader: destructor is called while thread is still running");
    }

    close_();
}

bool SoxReader::open(const char* driver, const char* input) {
    roc_log(LogDebug, "sox reader: opening: driver=%s input=%s", driver, input);

    if (buffer_ || input_) {
        roc_panic("sox reader: can't call open() more than once");
    }

    if (!prepare_()) {
        return false;
    }

    if (!open_(driver, input)) {
        return false;
    }

    return true;
}

size_t SoxReader::sample_rate() const {
    if (!input_) {
        roc_panic("sox reader: sample_rate: non-open input file or device");
    }
    return size_t(input_->signal.rate);
}

bool SoxReader::is_file() const {
    if (!input_) {
        roc_panic("sox reader: is_file: non-open input file or device");
    }
    return is_file_;
}

bool SoxReader::start(audio::IWriter& output) {
    output_ = &output;
    return Thread::start();
}

void SoxReader::stop() {
    stop_ = 1;
}

void SoxReader::join() {
    Thread::join();
}

void SoxReader::run() {
    roc_log(LogDebug, "sox reader: starting thread");

    if (!chain_) {
        roc_panic("sox reader: thread is started before open() returnes success");
    }

    if (!output_) {
        roc_panic("sox reader: thread is started not from the start() call");
    }

    int err = sox_flow_effects(chain_, NULL, NULL);
    if (err != 0) {
        roc_log(LogInfo, "sox reader: sox_flow_effects(): %s", sox_strerror(err));
    }

    flush_();
    close_();

    roc_log(LogDebug, "sox reader: finishing thread, read %lu buffers",
            (unsigned long)n_bufs_);
}

int SoxReader::kill_cb_(sox_effect_t* eff) {
    roc_log(LogDebug, "sox reader: received kill callback");

    roc_panic_if(!eff);
    roc_panic_if(!eff->priv);

    eff->priv = NULL; // please do not free() us

    return SOX_SUCCESS;
}

int SoxReader::output_cb_(sox_effect_t* eff,
                          const sox_sample_t* ibuf,
                          sox_sample_t* obuf,
                          size_t* ibufsz,
                          size_t* obufsz) {
    roc_panic_if(!eff);
    roc_panic_if(!eff->priv);

    SoxReader& self = *(SoxReader*)eff->priv;
    if (self.stop_) {
        roc_log(LogInfo, "sox reader: stopped, exiting");
        return SOX_EOF;
    }

    roc_panic_if(!ibuf);
    roc_panic_if(!ibufsz);

    self.write_(ibuf, *ibufsz);

    (void)obuf;
    if (obufsz) {
        *obufsz = 0;
    }

    return SOX_SUCCESS;
}

bool SoxReader::prepare_() {
    if (buffer_pool_.buffer_size() < buffer_size_) {
        roc_log(LogError, "sox reader: buffer size is too small: required=%lu actual=%lu",
                (unsigned long)buffer_size_, (unsigned long)buffer_pool_.buffer_size());
        return false;
    }

    buffer_ = new (buffer_pool_) core::Buffer<audio::sample_t>(buffer_pool_);

    if (!buffer_) {
        roc_log(LogError, "sox reader: can't allocate buffer");
        return false;
    }

    buffer_.resize(buffer_size_);

    return true;
}

bool SoxReader::open_(const char* driver, const char* input) {
    if (!SoxController::instance().fill_defaults(driver, input)) {
        return false;
    }

    roc_log(LogInfo, "sox reader: driver=%s input=%s", driver, input);

    if (!(input_ = sox_open_read(input, NULL, NULL, driver))) {
        roc_log(LogError, "sox reader: can't open reader: driver=%s input=%s", driver,
                input);
        return false;
    }

    is_file_ = !(input_->handler.flags & SOX_FILE_DEVICE);

    roc_log(LogInfo,
            "sox reader:"
            " in_bits=%lu out_bits=%lu in_rate=%lu out_rate=%lu"
            " in_ch=%lu, out_ch=%lu, is_file=%d",
            (unsigned long)input_->encoding.bits_per_sample,
            (unsigned long)out_signal_.precision, (unsigned long)input_->signal.rate,
            (unsigned long)out_signal_.rate, (unsigned long)input_->signal.channels,
            (unsigned long)out_signal_.channels, (int)is_file_);

    if (!(chain_ = sox_create_effects_chain(&input_->encoding, NULL))) {
        roc_panic("sox reader: sox_create_effects_chain() failed");
    }

    {
        const char* args[] = { (const char*)input_ };

        add_effect(chain_, "input", &input_->signal, &out_signal_, ROC_ARRAY_SIZE(args),
                   args);
    }

    if (input_->signal.channels != out_signal_.channels) {
        add_effect(chain_, "channels", &input_->signal, &out_signal_, 0, NULL);
    }

    {
        sox_effect_t* effect = sox_create_effect(&output_handler_);
        if (!effect) {
            roc_panic("sox reader: sox_create_effect(): can't create output effect");
        }

        effect->priv = this;

        int err = sox_add_effect(chain_, effect, &input_->signal, &out_signal_);
        if (err != SOX_SUCCESS) {
            roc_panic("sox reader: sox_add_effect(): can't add output effect: %s",
                      sox_strerror(err));
        }

        free(effect);
    }

    return true;
}

void SoxReader::write_(const sox_sample_t* buf, size_t bufsz) {
    while (bufsz != 0) {
        audio::sample_t* buffer_data = buffer_.data();

        SOX_SAMPLE_LOCALS;

        size_t clips = 0;

        for (; buffer_pos_ < buffer_size_; buffer_pos_++) {
            if (bufsz == 0) {
                break;
            }
            buffer_data[buffer_pos_] = (float)SOX_SAMPLE_TO_FLOAT_32BIT(*buf, clips);
            buf++;
            bufsz--;
        }

        if (buffer_pos_ == buffer_size_) {
            flush_();
        }
    }
}

void SoxReader::flush_() {
    if (buffer_pos_ == 0) {
        return;
    }

    audio::Frame frame(buffer_.data(), buffer_pos_);
    output_->write(frame);

    buffer_pos_ = 0;
    n_bufs_++;
}

void SoxReader::close_() {
    if (chain_) {
        sox_delete_effects_chain(chain_);
        chain_ = NULL;
    }

    if (input_) {
        int err = sox_close(input_);
        if (err != SOX_SUCCESS) {
            roc_panic("sox reader: can't close input: %s", sox_strerror(err));
        }
        input_ = NULL;
    }
}

} // namespace sndio
} // namespace roc
