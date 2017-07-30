/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/recorder.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_sndio/default.h"

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
        roc_panic("recorder: sox_find_effect(): can't find '%s' effect", name);
    }

    sox_effect_t* effect = sox_create_effect(handler);
    if (!effect) {
        roc_panic("recorder: sox_create_effect(): can't create '%s' effect", name);
    }

    int err = sox_effect_options(effect, argc, const_cast<char**>(argv));
    if (err != SOX_SUCCESS) {
        roc_panic("recorder: sox_effect_options(): can't set '%s' effect options: %s",
                  name, sox_strerror(err));
    }

    err = sox_add_effect(chain, effect, in, out);
    if (err != SOX_SUCCESS) {
        roc_panic("recorder: sox_add_effect(): can't add gain effect: %s",
                  sox_strerror(err));
    }

    free(effect);
}

} // namespace

const sox_effect_handler_t Recorder::output_handler_ = {
    "roc_output",         //
    NULL,                 //
    SOX_EFF_MCHAN,        //
    NULL,                 //
    NULL,                 //
    Recorder::output_cb_, //
    NULL,                 //
    NULL,                 //
    Recorder::kill_cb_,   //
    0                     //
};

Recorder::Recorder(audio::IWriter& output,
                   core::BufferPool<audio::sample_t>& buffer_pool,
                   packet::channel_mask_t channels,
                   size_t n_samples,
                   size_t sample_rate)
    : input_(NULL)
    , chain_(NULL)
    , output_(output)
    , buffer_pool_(buffer_pool) {
    size_t n_channels = packet::num_channels(channels);
    if (n_channels == 0) {
        roc_panic("recorder: # of channels is zero");
    }

    if (n_samples == 0) {
        roc_panic("recorder: # of samples is zero");
    }

    if (sample_rate == 0) {
        roc_panic("recorder: sample rate is zero");
    }

    buffer_size_ = n_samples * n_channels;
    buffer_pos_ = 0;

    clips_ = 0;
    n_bufs_ = 0;

    memset(&out_signal_, 0, sizeof(out_signal_));
    out_signal_.rate = sample_rate;
    out_signal_.channels = (unsigned)n_channels;
    out_signal_.precision = SOX_SAMPLE_PRECISION;
}

Recorder::~Recorder() {
    if (joinable()) {
        roc_panic("recorder: destructor is called while thread is still running");
    }

    close_();
}

bool Recorder::open(const char* name, const char* type) {
    roc_log(LogDebug, "recorder: opening: name=%s type=%s", name, type);

    if (input_) {
        roc_panic("recorder: can't call open() more than once");
    }

    if (!detect_defaults(&name, &type)) {
        roc_log(LogError, "can't detect defaults: name=%s type=%s", name, type);
        return false;
    }

    roc_log(LogInfo, "recorder: name=%s type=%s", name, type);

    if (!(input_ = sox_open_read(name, NULL, NULL, type))) {
        roc_log(LogError, "recorder: can't open reader: name=%s type=%s", name, type);
        return false;
    }

    roc_log(LogInfo,
            "recorder:"
            " in_bits=%lu out_bits=%lu in_rate=%lu out_rate=%lu in_ch=%lu, out_ch=%lu",
            (unsigned long)input_->encoding.bits_per_sample,
            (unsigned long)out_signal_.precision, (unsigned long)input_->signal.rate,
            (unsigned long)out_signal_.rate, (unsigned long)input_->signal.channels,
            (unsigned long)out_signal_.channels);

    if (!(chain_ = sox_create_effects_chain(&input_->encoding, NULL))) {
        roc_panic("recorder: sox_create_effects_chain() failed");
    }

    {
        const char* args[] = { (const char*)input_ };

        add_effect(chain_, "input", &input_->signal, &out_signal_, ROC_ARRAY_SIZE(args),
                   args);
    }

    if (input_->signal.channels != out_signal_.channels) {
        add_effect(chain_, "channels", &input_->signal, &out_signal_, 0, NULL);
    }

    if ((size_t)input_->signal.rate != (size_t)out_signal_.rate) {
        const char* gain_h_args[] = {
            "-h",
        };

        add_effect(chain_, "gain", &input_->signal, &out_signal_,
                   ROC_ARRAY_SIZE(gain_h_args), gain_h_args);

        const char* rate_args[] = { "-b", "99.7", "-v" };

        add_effect(chain_, "rate", &input_->signal, &out_signal_,
                   ROC_ARRAY_SIZE(rate_args), rate_args);

        const char* gain_r_args[] = {
            "-r",
        };

        add_effect(chain_, "gain", &input_->signal, &out_signal_,
                   ROC_ARRAY_SIZE(gain_r_args), gain_r_args);
    }

    {
        sox_effect_t* effect = sox_create_effect(&output_handler_);
        if (!effect) {
            roc_panic("recorder: sox_create_effect(): can't create output effect");
        }

        effect->priv = this;

        int err = sox_add_effect(chain_, effect, &out_signal_, &out_signal_);
        if (err != SOX_SUCCESS) {
            roc_panic("recorder: sox_add_effect(): can't add output effect: %s",
                      sox_strerror(err));
        }

        free(effect);
    }

    return true;
}

void Recorder::stop() {
    stop_ = 1;
}

void Recorder::run() {
    roc_log(LogDebug, "recorder: starting thread");

    if (!chain_) {
        roc_panic("recorder: thread is started before open() returnes success");
    }

    int err = sox_flow_effects(chain_, NULL, NULL);
    if (err != 0) {
        roc_log(LogInfo, "recorder: sox_flow_effects(): %s", sox_strerror(err));
    }

    close_();

    roc_log(LogDebug, "recorder: finishing thread, read %lu buffers",
            (unsigned long)n_bufs_);
}

int Recorder::kill_cb_(sox_effect_t* eff) {
    roc_panic_if(!eff);
    roc_panic_if(!eff->priv);

    eff->priv = NULL; // please do not free() us

    return SOX_SUCCESS;
}

int Recorder::output_cb_(sox_effect_t* eff,
                         const sox_sample_t* ibuf,
                         sox_sample_t* obuf,
                         size_t* ibufsz,
                         size_t* obufsz) {
    roc_panic_if(!eff);
    roc_panic_if(!eff->priv);

    Recorder& self = *(Recorder*)eff->priv;
    if (self.stop_) {
        roc_log(LogInfo, "recorder: stopped, exiting");
        return SOX_EOF;
    }

    roc_panic_if(!ibuf);
    roc_panic_if(!ibufsz);

    self.write_(ibuf, *ibufsz, *ibufsz < sox_get_globals()->input_bufsiz);

    (void)obuf;
    if (obufsz) {
        *obufsz = 0;
    }

    return SOX_SUCCESS;
}

void Recorder::write_(const sox_sample_t* buf, size_t bufsz, bool eof) {
    while (bufsz != 0) {
        if (!frame_.samples) {
            frame_.samples =
                new (buffer_pool_) core::Buffer<audio::sample_t>(buffer_pool_);

            if (!frame_.samples) {
                roc_log(LogError, "recorder: can't allocate buffer");
                return;
            }

            frame_.samples.resize(buffer_size_);
        }

        audio::sample_t* samples = frame_.samples.data();

        SOX_SAMPLE_LOCALS;

        for (; buffer_pos_ < buffer_size_; buffer_pos_++) {
            if (bufsz == 0) {
                break;
            }
            samples[buffer_pos_] = (float)SOX_SAMPLE_TO_FLOAT_32BIT(*buf, clips_);
            buf++;
            bufsz--;
        }

        if (eof) {
            for (; buffer_pos_ < buffer_size_; buffer_pos_++) {
                samples[buffer_pos_] = 0;
            }
        }

        if (buffer_pos_ == buffer_size_) {
            output_.write(frame_);
            buffer_pos_ = 0;
            n_bufs_++;
        }
    }
}

void Recorder::close_() {
    if (chain_) {
        sox_delete_effects_chain(chain_);
        chain_ = NULL;
    }

    if (input_) {
        int err = sox_close(input_);
        if (err != SOX_SUCCESS) {
            roc_panic("recorder: can't close input: %s", sox_strerror(err));
        }
        input_ = NULL;
    }
}

} // namespace sndio
} // namespace roc
