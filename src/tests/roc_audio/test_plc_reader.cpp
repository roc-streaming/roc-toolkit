/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/mock_reader.h"

#include "roc_audio/plc_reader.h"
#include "roc_audio/processor_map.h"
#include "roc_core/heap_arena.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/scoped_ptr.h"

namespace roc {
namespace audio {

namespace {

const double Epsilon = 0.00001;

enum { MaxSz = 800 };

core::HeapArena arena;
FrameFactory frame_factory(arena, MaxSz * sizeof(sample_t));
ProcessorMap processor_map(arena);

void add_samples(test::MockReader& mock_reader,
                 size_t size,
                 sample_t value,
                 unsigned flags) {
    CHECK(size > 0);

    for (size_t n = 0; n < size; n++) {
        mock_reader.add_samples(1, value, flags);
    }
}

FramePtr expect_frame(status::StatusCode expected_code,
                      IFrameReader& reader,
                      const SampleSpec& sample_spec,
                      size_t requested_samples,
                      size_t expected_samples,
                      FrameReadMode mode) {
    CHECK(requested_samples % sample_spec.num_channels() == 0);
    CHECK(expected_samples % sample_spec.num_channels() == 0);

    FramePtr frame = frame_factory.allocate_frame_no_buffer();
    CHECK(frame);

    const status::StatusCode code =
        reader.read(*frame, requested_samples / sample_spec.num_channels(), mode);

    LONGS_EQUAL(expected_code, code);

    if (expected_code == status::StatusOK || expected_code == status::StatusPart) {
        if (sample_spec.is_raw()) {
            CHECK(frame->is_raw());
            CHECK(frame->raw_samples());
            LONGS_EQUAL(expected_samples, frame->num_raw_samples());
        } else {
            CHECK(!frame->is_raw());
        }

        LONGS_EQUAL(expected_samples / sample_spec.num_channels(), frame->duration());

        CHECK(frame->bytes());
        LONGS_EQUAL(sample_spec.stream_timestamp_2_bytes(expected_samples
                                                         / sample_spec.num_channels()),
                    frame->num_bytes());
    }

    return frame;
}

void expect_samples(const Frame& frame, size_t n_samples, sample_t value) {
    CHECK(frame.is_raw());
    LONGS_EQUAL(n_samples, frame.num_raw_samples());

    for (size_t n = 0; n < n_samples; n++) {
        DOUBLES_EQUAL((double)value, (double)frame.raw_samples()[n], Epsilon);
    }
}

void expect_zero_samples(const Frame& frame, size_t n_samples) {
    CHECK(frame.is_raw());
    LONGS_EQUAL(n_samples, frame.num_raw_samples());

    int non_zero = 0;

    for (size_t n = 0; n < n_samples; n++) {
        if (std::abs((double)frame.raw_samples()[n]) > Epsilon) {
            non_zero++;
        }
    }

    CHECK(non_zero == 0);
}

void expect_non_zero_samples(const Frame& frame, size_t n_samples) {
    CHECK(frame.is_raw());
    LONGS_EQUAL(n_samples, frame.num_raw_samples());

    int non_zero = 0;

    for (size_t n = 0; n < n_samples; n++) {
        if (std::abs((double)frame.raw_samples()[n]) > Epsilon) {
            non_zero++;
        }
    }

    CHECK(non_zero > 0);
}

template <class T>
void expect_int_samples(const Frame& frame, size_t n_samples, T value) {
    CHECK(!frame.is_raw());
    LONGS_EQUAL(n_samples, frame.num_bytes() / sizeof(T));

    for (size_t n = 0; n < n_samples; n++) {
        LONGS_EQUAL(value, ((const T*)frame.bytes())[n]);
    }
}

class MockPlc : public IPlc {
public:
    MockPlc(const SampleSpec& sample_spec, core::IArena& arena)
        : IPlc(arena)
        , sample_spec_(sample_spec)
        , lookbehind_len_(0)
        , lookahead_len_(0)
        , fill_value_(0)
        , prev_value_(0)
        , next_value_(0)
        , n_history_samples_(0)
        , n_lost_samples_(0)
        , n_prev_samples_(0)
        , n_next_samples_(0) {
    }

    virtual status::StatusCode init_status() const {
        return status::StatusOK;
    }

    virtual SampleSpec sample_spec() const {
        return sample_spec_;
    }

    virtual packet::stream_timestamp_t lookbehind_len() {
        return lookbehind_len_;
    }

    virtual packet::stream_timestamp_t lookahead_len() {
        return lookahead_len_;
    }

    virtual void process_history(Frame& hist_frame) {
        n_history_samples_ += hist_frame.num_raw_samples();

        n_prev_samples_ = 0;
        n_next_samples_ = 0;
    }

    virtual void process_loss(Frame& lost_frame, Frame* prev_frame, Frame* next_frame) {
        if (prev_frame) {
            CHECK(prev_frame->is_raw());
            LONGS_EQUAL(prev_frame->num_raw_samples(), prev_frame->duration());
            LONGS_EQUAL(prev_frame->num_bytes(),
                        prev_frame->duration() * sizeof(sample_t));
            CHECK(prev_frame->num_raw_samples() > 0);
            CHECK(prev_frame->num_raw_samples() <= lookbehind_len_);

            n_prev_samples_ = prev_frame->num_raw_samples();
            expect_samples(*prev_frame, prev_frame->num_raw_samples(), prev_value_);
        } else {
            CHECK(lookbehind_len_ == 0);
            n_prev_samples_ = 0;
        }

        if (next_frame) {
            CHECK(prev_frame);

            CHECK(next_frame->is_raw());
            LONGS_EQUAL(next_frame->num_raw_samples(), next_frame->duration());
            LONGS_EQUAL(next_frame->num_bytes(),
                        next_frame->duration() * sizeof(sample_t));
            CHECK(next_frame->num_raw_samples() > 0);
            CHECK(next_frame->num_raw_samples() <= lookahead_len_);

            n_next_samples_ = next_frame->num_raw_samples();
            expect_samples(*next_frame, next_frame->num_raw_samples(), next_value_);
        } else {
            n_next_samples_ = 0;
        }

        CHECK(lost_frame.is_raw());
        LONGS_EQUAL(lost_frame.num_raw_samples(), lost_frame.duration());
        LONGS_EQUAL(lost_frame.num_bytes(), lost_frame.duration() * sizeof(sample_t));
        CHECK(lost_frame.num_raw_samples() > 0);

        for (size_t n = 0; n < lost_frame.num_raw_samples(); n++) {
            lost_frame.raw_samples()[n] = fill_value_;
        }
        n_lost_samples_ += lost_frame.num_raw_samples();
    }

    void set_fill_value(sample_t value) {
        fill_value_ = value;
    }

    void set_expected_prev_value(sample_t value) {
        prev_value_ = value;
    }

    void set_expected_next_value(sample_t value) {
        next_value_ = value;
    }

    void set_lookbehind(packet::stream_timestamp_t value) {
        lookbehind_len_ = value;
    }

    void set_lookahead(packet::stream_timestamp_t value) {
        lookahead_len_ = value;
    }

    size_t n_history_samples() const {
        return n_history_samples_;
    }

    size_t n_lost_samples() const {
        return n_lost_samples_;
    }

    size_t n_prev_samples() const {
        return n_prev_samples_;
    }

    size_t n_next_samples() const {
        return n_next_samples_;
    }

private:
    const SampleSpec sample_spec_;
    packet::stream_timestamp_t lookbehind_len_;
    packet::stream_timestamp_t lookahead_len_;
    sample_t fill_value_;
    sample_t prev_value_;
    sample_t next_value_;
    size_t n_history_samples_;
    size_t n_lost_samples_;
    size_t n_prev_samples_;
    size_t n_next_samples_;
};

template <class T> class IntPlc : public IPlc {
public:
    IntPlc(const SampleSpec& sample_spec,
           packet::stream_timestamp_t window_len,
           core::IArena& arena)
        : IPlc(arena)
        , sample_spec_(sample_spec)
        , window_len_(window_len)
        , fill_value_(0) {
    }

    virtual status::StatusCode init_status() const {
        return status::StatusOK;
    }

    virtual SampleSpec sample_spec() const {
        return sample_spec_;
    }

    virtual packet::stream_timestamp_t lookbehind_len() {
        return window_len_;
    }

    virtual packet::stream_timestamp_t lookahead_len() {
        return window_len_;
    }

    virtual void process_history(Frame& hist_frame) {
    }

    virtual void process_loss(Frame& lost_frame, Frame* prev_frame, Frame* next_frame) {
        const size_t n_samples = lost_frame.num_bytes() / sizeof(T);
        T* samples = (T*)lost_frame.bytes();

        for (size_t ns = 0; ns < n_samples; ns++) {
            samples[ns] = fill_value_;
        }
    }

    void set_fill_value(T value) {
        fill_value_ = value;
    }

private:
    const SampleSpec sample_spec_;
    const packet::stream_timestamp_t window_len_;
    T fill_value_;
};

template <class T> struct IntReader : IFrameReader {
    IntReader(const SampleSpec& sample_spec)
        : sample_spec_(sample_spec)
        , rd_pos_(0)
        , wr_pos_(0) {
        memset(return_values_, 0, sizeof(return_values_));
        memset(return_flags_, 0, sizeof(return_flags_));
    }

    virtual status::StatusCode
    read(Frame& frame, packet::stream_timestamp_t duration, FrameReadMode mode) {
        CHECK(rd_pos_ < wr_pos_);

        CHECK(frame_factory.reallocate_frame(
            frame, sample_spec_.stream_timestamp_2_bytes(duration)));

        frame.set_raw(sample_spec_.is_raw());
        frame.set_duration(duration);
        frame.set_flags(return_flags_[rd_pos_]);

        size_t pos = 0;
        while (pos < frame.num_bytes()) {
            *reinterpret_cast<T*>(frame.bytes() + pos) = return_values_[rd_pos_];
            pos += sizeof(T);
        }

        rd_pos_++;

        return status::StatusOK;
    }

    void add_return_frame(T value, unsigned flags) {
        return_values_[wr_pos_] = value;
        return_flags_[wr_pos_] = flags;
        wr_pos_++;
    }

    size_t n_unread_frames() {
        return wr_pos_ - rd_pos_;
    }

private:
    enum { MaxSz = 100 };

    const SampleSpec sample_spec_;

    T return_values_[MaxSz];
    unsigned return_flags_[MaxSz];
    size_t rd_pos_;
    size_t wr_pos_;
};

} // namespace

TEST_GROUP(plc_reader) {
    PlcBackend supported_backends[PlcBackend_Max];
    size_t n_supported_backends;

    void setup() {
        n_supported_backends = 0;

        for (int n = 0; n < PlcBackend_Max; n++) {
            const PlcBackend backend = (PlcBackend)n;
            if (backend == PlcBackend_Default || backend == PlcBackend_None) {
                continue;
            }
            if (!processor_map.has_plc_backend(backend)) {
                continue;
            }

            supported_backends[n_supported_backends++] = backend;
        }
    }
};

// Read frame that fits maximum size.
TEST(plc_reader, small_read) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec sample_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);

    test::MockReader mock_reader(frame_factory, sample_spec);
    MockPlc mock_plc(sample_spec, arena);

    add_samples(mock_reader, FrameSz, 0.11f, Frame::HasSignal);

    PlcReader plc_reader(mock_reader, frame_factory, mock_plc, sample_spec);
    LONGS_EQUAL(status::StatusOK, plc_reader.init_status());

    FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                  FrameSz, ModeHard);

    LONGS_EQUAL(1, mock_reader.total_reads());
    LONGS_EQUAL(0, mock_reader.num_unread());

    LONGS_EQUAL(status::StatusOK, mock_reader.last_status());
    LONGS_EQUAL(ModeHard, mock_reader.last_mode());

    LONGS_EQUAL(Frame::HasSignal, frame->flags());
    expect_samples(*frame, FrameSz, 0.11f);
}

// Read frame that exceeds maximum size.
// Duration is capped and partial read is produced.
TEST(plc_reader, big_read) {
    enum { FrameSz = MaxSz * 2 };

    const SampleSpec sample_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);

    test::MockReader mock_reader(frame_factory, sample_spec);
    MockPlc mock_plc(sample_spec, arena);

    add_samples(mock_reader, FrameSz, 0.11f, Frame::HasSignal);

    PlcReader plc_reader(mock_reader, frame_factory, mock_plc, sample_spec);
    LONGS_EQUAL(status::StatusOK, plc_reader.init_status());

    FramePtr frame = expect_frame(status::StatusPart, plc_reader, sample_spec, FrameSz,
                                  MaxSz, ModeHard);

    LONGS_EQUAL(1, mock_reader.total_reads());
    LONGS_EQUAL(FrameSz - MaxSz, mock_reader.num_unread());

    LONGS_EQUAL(status::StatusOK, mock_reader.last_status());
    LONGS_EQUAL(ModeHard, mock_reader.last_mode());

    LONGS_EQUAL(Frame::HasSignal, frame->flags());
    expect_samples(*frame, MaxSz, 0.11f);
}

// PLC reader should ignore initial gap frame preceding first signal.
// It should not try to fill them with PLC.
TEST(plc_reader, initial_gap) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec sample_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);

    test::MockReader mock_reader(frame_factory, sample_spec);
    MockPlc mock_plc(sample_spec, arena);

    add_samples(mock_reader, FrameSz, 0.00f, Frame::HasGaps);
    add_samples(mock_reader, FrameSz, 0.00f, Frame::HasGaps);
    add_samples(mock_reader, FrameSz, 0.00f, Frame::HasGaps);

    add_samples(mock_reader, FrameSz, 0.11f, Frame::HasSignal);

    mock_plc.set_lookbehind(FrameSz);
    mock_plc.set_lookahead(FrameSz);

    PlcReader plc_reader(mock_reader, frame_factory, mock_plc, sample_spec);
    LONGS_EQUAL(status::StatusOK, plc_reader.init_status());

    for (size_t i = 0; i < 3; i++) {
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasGaps, frame->flags());
        expect_samples(*frame, FrameSz, 0.00f);

        LONGS_EQUAL(i + 1, mock_reader.total_reads());
        LONGS_EQUAL(status::StatusOK, mock_reader.last_status());

        LONGS_EQUAL(0, mock_plc.n_history_samples());
        LONGS_EQUAL(0, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }

    FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                  FrameSz, ModeHard);

    LONGS_EQUAL(Frame::HasSignal, frame->flags());
    expect_samples(*frame, FrameSz, 0.11f);

    LONGS_EQUAL(4, mock_reader.total_reads());
    LONGS_EQUAL(status::StatusOK, mock_reader.last_status());

    LONGS_EQUAL(FrameSz, mock_plc.n_history_samples());
    LONGS_EQUAL(0, mock_plc.n_lost_samples());
    LONGS_EQUAL(0, mock_plc.n_prev_samples());
    LONGS_EQUAL(0, mock_plc.n_next_samples());
}

// IPlc::window_len() returns zero, so PlcReader doesn't perform read-ahead.
TEST(plc_reader, readahead_disabled) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec sample_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);

    test::MockReader mock_reader(frame_factory, sample_spec);
    MockPlc mock_plc(sample_spec, arena);

    add_samples(mock_reader, FrameSz, 0.11f, Frame::HasSignal);
    add_samples(mock_reader, FrameSz, 0.00f, Frame::HasGaps);
    add_samples(mock_reader, FrameSz, 0.33f, Frame::HasSignal);

    mock_plc.set_lookbehind(0);
    mock_plc.set_lookahead(0);
    mock_plc.set_fill_value(0.22f);

    PlcReader plc_reader(mock_reader, frame_factory, mock_plc, sample_spec);
    LONGS_EQUAL(status::StatusOK, plc_reader.init_status());

    { // frame 1: signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_samples(*frame, FrameSz, 0.11f);

        LONGS_EQUAL(1, mock_reader.total_reads());
        LONGS_EQUAL(status::StatusOK, mock_reader.last_status());

        LONGS_EQUAL(FrameSz, mock_plc.n_history_samples());
        LONGS_EQUAL(0, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }

    { // frame 2: gap
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasGaps, frame->flags());
        expect_samples(*frame, FrameSz, 0.22f); // filled by PLC

        LONGS_EQUAL(2, mock_reader.total_reads());
        LONGS_EQUAL(status::StatusOK, mock_reader.last_status());

        LONGS_EQUAL(FrameSz, mock_plc.n_history_samples());
        LONGS_EQUAL(FrameSz, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }

    { // frame 3: signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_samples(*frame, FrameSz, 0.33f);

        LONGS_EQUAL(3, mock_reader.total_reads());
        LONGS_EQUAL(status::StatusOK, mock_reader.last_status());

        LONGS_EQUAL(FrameSz * 2, mock_plc.n_history_samples());
        LONGS_EQUAL(FrameSz, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }
}

// IPlc::window_len() returns non-zero, so PlcReader performs read-ahead
// (a soft read), and passes prev & next frames to IPlc. On next read, PlcReader
// will first return samples from that next frame and then switch to normal read.
TEST(plc_reader, readahead_enabled) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec sample_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);

    test::MockReader mock_reader(frame_factory, sample_spec);
    MockPlc mock_plc(sample_spec, arena);

    add_samples(mock_reader, FrameSz, 0.11f, Frame::HasSignal);
    add_samples(mock_reader, FrameSz, 0.00f, Frame::HasGaps);
    add_samples(mock_reader, FrameSz, 0.33f, Frame::HasSignal);
    add_samples(mock_reader, FrameSz, 0.44f, Frame::HasSignal);

    mock_plc.set_lookbehind(FrameSz);
    mock_plc.set_lookahead(FrameSz);
    mock_plc.set_fill_value(0.22f);
    mock_plc.set_expected_prev_value(0.11f);
    mock_plc.set_expected_next_value(0.33f);

    PlcReader plc_reader(mock_reader, frame_factory, mock_plc, sample_spec);
    LONGS_EQUAL(status::StatusOK, plc_reader.init_status());

    { // frame 1: signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_samples(*frame, FrameSz, 0.11f);

        LONGS_EQUAL(1, mock_reader.total_reads());
        LONGS_EQUAL(status::StatusOK, mock_reader.last_status());

        LONGS_EQUAL(FrameSz, mock_plc.n_history_samples());
        LONGS_EQUAL(0, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }

    { // frame 2: gap
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasGaps, frame->flags());
        expect_samples(*frame, FrameSz, 0.22f); // filled by PLC

        LONGS_EQUAL(3, mock_reader.total_reads());
        LONGS_EQUAL(status::StatusOK, mock_reader.last_status());

        LONGS_EQUAL(FrameSz, mock_plc.n_history_samples());
        LONGS_EQUAL(FrameSz, mock_plc.n_lost_samples());
        LONGS_EQUAL(FrameSz, mock_plc.n_prev_samples());
        LONGS_EQUAL(FrameSz, mock_plc.n_next_samples()); // read-ahead
    }

    { // frame 3 (first half): signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec,
                                      FrameSz / 2, FrameSz / 2, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_samples(*frame, FrameSz / 2, 0.33f); // filled from last read-ahead

        LONGS_EQUAL(3, mock_reader.total_reads());
        LONGS_EQUAL(status::NoStatus, mock_reader.last_status()); // not called

        LONGS_EQUAL(FrameSz + FrameSz / 2, mock_plc.n_history_samples());
        LONGS_EQUAL(FrameSz, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }

    { // frame 3 (second half): signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec,
                                      FrameSz / 2, FrameSz / 2, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_samples(*frame, FrameSz / 2, 0.33f); // filled from last read-ahead

        LONGS_EQUAL(3, mock_reader.total_reads());
        LONGS_EQUAL(status::NoStatus, mock_reader.last_status()); // not called

        LONGS_EQUAL(FrameSz * 2, mock_plc.n_history_samples());
        LONGS_EQUAL(FrameSz, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }

    { // frame 4: signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_samples(*frame, FrameSz, 0.44f);

        LONGS_EQUAL(4, mock_reader.total_reads());
        LONGS_EQUAL(status::StatusOK, mock_reader.last_status());

        LONGS_EQUAL(FrameSz * 3, mock_plc.n_history_samples());
        LONGS_EQUAL(FrameSz, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }
}

// IPlc::window_len() returns no-zero, so PlcReader performs read-ahead
// (a soft read), but it returns StatusDrain, indicating that next frame is
// not available yet.
TEST(plc_reader, readahead_drained) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec sample_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);

    test::MockReader mock_reader(frame_factory, sample_spec);
    MockPlc mock_plc(sample_spec, arena);

    add_samples(mock_reader, FrameSz, 0.11f, Frame::HasSignal);
    add_samples(mock_reader, FrameSz, 0.00f, Frame::HasGaps);

    mock_plc.set_lookbehind(FrameSz);
    mock_plc.set_lookahead(FrameSz);
    mock_plc.set_fill_value(0.22f);
    mock_plc.set_expected_prev_value(0.11f);
    mock_plc.set_expected_next_value(0.00f); // next not available

    PlcReader plc_reader(mock_reader, frame_factory, mock_plc, sample_spec);
    LONGS_EQUAL(status::StatusOK, plc_reader.init_status());

    { // frame 1: signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_samples(*frame, FrameSz, 0.11f);

        LONGS_EQUAL(1, mock_reader.total_reads());
        LONGS_EQUAL(status::StatusOK, mock_reader.last_status());

        LONGS_EQUAL(FrameSz, mock_plc.n_history_samples());
        LONGS_EQUAL(0, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }

    { // frame 2: gap, frame 3 not delivered yet
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasGaps, frame->flags());
        expect_samples(*frame, FrameSz, 0.22f);

        LONGS_EQUAL(3, mock_reader.total_reads());
        LONGS_EQUAL(status::StatusDrain, mock_reader.last_status());

        LONGS_EQUAL(FrameSz, mock_plc.n_history_samples());
        LONGS_EQUAL(FrameSz, mock_plc.n_lost_samples());
        LONGS_EQUAL(FrameSz, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }

    // deliver frame 3
    add_samples(mock_reader, FrameSz, 0.33f, Frame::HasSignal);

    { // frame 3: signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_samples(*frame, FrameSz, 0.33f);

        LONGS_EQUAL(4, mock_reader.total_reads());
        LONGS_EQUAL(status::StatusOK, mock_reader.last_status());

        LONGS_EQUAL(FrameSz * 2, mock_plc.n_history_samples());
        LONGS_EQUAL(FrameSz, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }
}

// When PLC reader is doing read-ahead and gets partial read, it should repeat.
TEST(plc_reader, readahead_partial) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec sample_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);

    test::MockReader mock_reader(frame_factory, sample_spec);
    MockPlc mock_plc(sample_spec, arena);

    add_samples(mock_reader, FrameSz, 0.11f, Frame::HasSignal);
    add_samples(mock_reader, FrameSz, 0.00f, Frame::HasGaps);
    add_samples(mock_reader, FrameSz, 0.33f, Frame::HasSignal);

    // read-ahead will trigger partial read
    mock_reader.set_limit_for_mode(FrameSz / 2, ModeSoft);

    mock_plc.set_lookbehind(FrameSz);
    mock_plc.set_lookahead(FrameSz);
    mock_plc.set_fill_value(0.22f);
    mock_plc.set_expected_prev_value(0.11f);
    mock_plc.set_expected_next_value(0.33f);

    PlcReader plc_reader(mock_reader, frame_factory, mock_plc, sample_spec);
    LONGS_EQUAL(status::StatusOK, plc_reader.init_status());

    { // frame 1: signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_samples(*frame, FrameSz, 0.11f);

        LONGS_EQUAL(1, mock_reader.total_reads());
        LONGS_EQUAL(status::StatusOK, mock_reader.last_status());

        LONGS_EQUAL(FrameSz, mock_plc.n_history_samples());
        LONGS_EQUAL(0, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }

    { // frame 2: gap
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasGaps, frame->flags());
        expect_samples(*frame, FrameSz, 0.22f); // filled by PLC

        // +1 call for gap, +2 calls for read-ahead
        // read-ahead should be concatenated from two parts
        LONGS_EQUAL(4, mock_reader.total_reads());
        LONGS_EQUAL(status::StatusOK, mock_reader.last_status());

        LONGS_EQUAL(FrameSz, mock_plc.n_history_samples());
        LONGS_EQUAL(FrameSz, mock_plc.n_lost_samples());
        LONGS_EQUAL(FrameSz, mock_plc.n_prev_samples());
        LONGS_EQUAL(FrameSz, mock_plc.n_next_samples());
    }

    { // frame 3: signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_samples(*frame, FrameSz, 0.33f); // filled from last read-ahead

        LONGS_EQUAL(4, mock_reader.total_reads());
        LONGS_EQUAL(status::NoStatus, mock_reader.last_status()); // not called

        LONGS_EQUAL(FrameSz * 2, mock_plc.n_history_samples());
        LONGS_EQUAL(FrameSz, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }
}

// Packet losses + soft reads.
TEST(plc_reader, soft_reads) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec sample_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);

    test::MockReader mock_reader(frame_factory, sample_spec);
    MockPlc mock_plc(sample_spec, arena);

    add_samples(mock_reader, FrameSz, 0.00f, Frame::HasGaps);
    add_samples(mock_reader, FrameSz, 0.22f, Frame::HasSignal);
    add_samples(mock_reader, FrameSz, 0.00f, Frame::HasGaps);
    add_samples(mock_reader, FrameSz, 0.44f, Frame::HasSignal);

    mock_plc.set_lookbehind(FrameSz);
    mock_plc.set_lookahead(FrameSz);
    mock_plc.set_fill_value(0.33f);
    mock_plc.set_expected_prev_value(0.22f);
    mock_plc.set_expected_next_value(0.44f);

    PlcReader plc_reader(mock_reader, frame_factory, mock_plc, sample_spec);
    LONGS_EQUAL(status::StatusOK, plc_reader.init_status());

    { // frame 1: initial gap (soft)
        mock_reader.set_status(status::StatusDrain);

        expect_frame(status::StatusDrain, plc_reader, sample_spec, FrameSz, 0, ModeSoft);

        LONGS_EQUAL(1, mock_reader.total_reads());
        LONGS_EQUAL(status::StatusDrain, mock_reader.last_status());
        LONGS_EQUAL(ModeSoft, mock_reader.last_mode());

        LONGS_EQUAL(0, mock_plc.n_history_samples());
        LONGS_EQUAL(0, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }

    { // frame 1: initial gap (hard)
        mock_reader.set_status(status::StatusOK);

        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasGaps, frame->flags());
        expect_samples(*frame, FrameSz, 0.00f);

        LONGS_EQUAL(2, mock_reader.total_reads());
        LONGS_EQUAL(status::StatusOK, mock_reader.last_status());
        LONGS_EQUAL(ModeHard, mock_reader.last_mode());

        LONGS_EQUAL(0, mock_plc.n_history_samples());
        LONGS_EQUAL(0, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }

    { // frame 2: signal (soft)
        mock_reader.set_status(status::StatusOK);

        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeSoft);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_samples(*frame, FrameSz, 0.22f);

        LONGS_EQUAL(3, mock_reader.total_reads());
        LONGS_EQUAL(status::StatusOK, mock_reader.last_status());
        LONGS_EQUAL(ModeSoft, mock_reader.last_mode());

        LONGS_EQUAL(FrameSz, mock_plc.n_history_samples());
        LONGS_EQUAL(0, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }

    { // frame 3: gap (soft)
        mock_reader.set_status(status::StatusDrain);

        expect_frame(status::StatusDrain, plc_reader, sample_spec, FrameSz, 0, ModeSoft);

        LONGS_EQUAL(4, mock_reader.total_reads());
        LONGS_EQUAL(status::StatusDrain, mock_reader.last_status());
        LONGS_EQUAL(ModeSoft, mock_reader.last_mode());

        LONGS_EQUAL(FrameSz, mock_plc.n_history_samples());
        LONGS_EQUAL(0, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }

    { // frame 3: gap (hard)
        mock_reader.set_status(status::StatusOK);

        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasGaps, frame->flags());
        expect_samples(*frame, FrameSz, 0.33f); // filled by PLC

        LONGS_EQUAL(6, mock_reader.total_reads());
        LONGS_EQUAL(status::StatusOK, mock_reader.last_status());
        LONGS_EQUAL(ModeSoft, mock_reader.last_mode()); // read-ahead

        LONGS_EQUAL(FrameSz, mock_plc.n_history_samples());
        LONGS_EQUAL(FrameSz, mock_plc.n_lost_samples());
        LONGS_EQUAL(FrameSz, mock_plc.n_prev_samples());
        LONGS_EQUAL(FrameSz, mock_plc.n_next_samples());
    }

    { // frame 4: signal (soft)
        mock_reader.set_status(status::StatusOK);

        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeSoft);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_samples(*frame, FrameSz, 0.44f);

        LONGS_EQUAL(6, mock_reader.total_reads());
        LONGS_EQUAL(status::NoStatus, mock_reader.last_status());

        LONGS_EQUAL(FrameSz * 2, mock_plc.n_history_samples());
        LONGS_EQUAL(FrameSz, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }
}

// Packet losses + each frame has different size + lookbehind and lookahead
// have different sizes.
TEST(plc_reader, variable_frame_sizes) {
    enum {
        FrameSz1 = 3,  // 0.11
        FrameSz2 = 10, // 0.11
        FrameSz3 = 5,  // 0.22 (lost)
        FrameSz4 = 10, // 0.22
        FrameSz5 = 5,  // 0.33 (lost)
        FrameSz6 = 20, // 0.44
        // window around lost frame to be passed to PLC
        LookbehindSz = 15,
        LookaheadSz = 13,
    };

    const SampleSpec sample_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);

    test::MockReader mock_reader(frame_factory, sample_spec);
    MockPlc mock_plc(sample_spec, arena);

    mock_plc.set_lookbehind(LookbehindSz);
    mock_plc.set_lookahead(LookaheadSz);

    PlcReader plc_reader(mock_reader, frame_factory, mock_plc, sample_spec);
    LONGS_EQUAL(status::StatusOK, plc_reader.init_status());

    // deliver frames 1 & 2
    add_samples(mock_reader, FrameSz1, 0.11f, Frame::HasSignal);
    add_samples(mock_reader, FrameSz2, 0.11f, Frame::HasSignal);

    { // frame 1: signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz1,
                                      FrameSz1, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_samples(*frame, FrameSz1, 0.11f);

        LONGS_EQUAL(1, mock_reader.total_reads());
        LONGS_EQUAL(FrameSz2, mock_reader.num_unread());

        LONGS_EQUAL(FrameSz1, mock_plc.n_history_samples());
        LONGS_EQUAL(0, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }

    { // frame 2: signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz2,
                                      FrameSz2, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_samples(*frame, FrameSz2, 0.11f);

        LONGS_EQUAL(2, mock_reader.total_reads());
        LONGS_EQUAL(0, mock_reader.num_unread());

        LONGS_EQUAL(FrameSz1 + FrameSz2, mock_plc.n_history_samples());
        LONGS_EQUAL(0, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }

    // lose frame 3, deliver frame 4
    add_samples(mock_reader, FrameSz3, 0.00f, Frame::HasGaps);
    add_samples(mock_reader, FrameSz4, 0.22f, Frame::HasSignal);

    mock_plc.set_fill_value(0.22f);
    mock_plc.set_expected_prev_value(0.11f);
    mock_plc.set_expected_next_value(0.22f);

    { // frame 3: gap
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz3,
                                      FrameSz3, ModeHard);

        LONGS_EQUAL(Frame::HasGaps, frame->flags());
        expect_samples(*frame, FrameSz3, 0.22f);

        LONGS_EQUAL(5, mock_reader.total_reads());
        LONGS_EQUAL(0, mock_reader.num_unread());

        LONGS_EQUAL(FrameSz1 + FrameSz2, mock_plc.n_history_samples());
        LONGS_EQUAL(FrameSz3, mock_plc.n_lost_samples());
        LONGS_EQUAL(FrameSz1 + FrameSz2, mock_plc.n_prev_samples());
        LONGS_EQUAL(FrameSz4, mock_plc.n_next_samples());

        // prev_frame was truncated from the left
        CHECK(FrameSz1 + FrameSz2 < LookbehindSz);
        // next_frame was truncated from the right
        CHECK(FrameSz4 < LookaheadSz);
    }

    { // frame 4: signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz4,
                                      FrameSz4, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_samples(*frame, FrameSz4, 0.22f);

        LONGS_EQUAL(5, mock_reader.total_reads());
        LONGS_EQUAL(0, mock_reader.num_unread());

        LONGS_EQUAL(FrameSz1 + FrameSz2 + FrameSz4, mock_plc.n_history_samples());
        LONGS_EQUAL(FrameSz3, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }

    // lose frame 5, deliver frame 6
    add_samples(mock_reader, FrameSz5, 0.00f, Frame::HasGaps);
    add_samples(mock_reader, FrameSz6, 0.44f, Frame::HasSignal);

    mock_plc.set_fill_value(0.33f);
    mock_plc.set_expected_prev_value(0.22f);
    mock_plc.set_expected_next_value(0.44f);

    { // frame 5: gap
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz5,
                                      FrameSz5, ModeHard);

        LONGS_EQUAL(Frame::HasGaps, frame->flags());
        expect_samples(*frame, FrameSz5, 0.33f);

        LONGS_EQUAL(7, mock_reader.total_reads());
        LONGS_EQUAL(FrameSz6 - LookaheadSz, mock_reader.num_unread());

        LONGS_EQUAL(FrameSz1 + FrameSz2 + FrameSz4, mock_plc.n_history_samples());
        LONGS_EQUAL(FrameSz3 + FrameSz5, mock_plc.n_lost_samples());
        LONGS_EQUAL(LookbehindSz, mock_plc.n_prev_samples());
        LONGS_EQUAL(LookaheadSz, mock_plc.n_next_samples());

        // prev_frame starts in the middle of frame 3
        CHECK(FrameSz3 + FrameSz4 >= LookbehindSz);
        CHECK(FrameSz4 < LookbehindSz);
        // next_frame ends in the middle of frame 6
        CHECK(FrameSz6 > LookaheadSz);
    }

    { // frame 6 (first half): signal
        FramePtr frame = expect_frame(status::StatusPart, plc_reader, sample_spec,
                                      FrameSz6, LookaheadSz, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_samples(*frame, LookaheadSz, 0.44f);

        LONGS_EQUAL(7, mock_reader.total_reads());
        LONGS_EQUAL(FrameSz6 - LookaheadSz, mock_reader.num_unread());

        LONGS_EQUAL(FrameSz1 + FrameSz2 + FrameSz4 + LookaheadSz,
                    mock_plc.n_history_samples());
        LONGS_EQUAL(FrameSz3 + FrameSz5, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }

    { // frame 6 (second half): signal
        FramePtr frame =
            expect_frame(status::StatusOK, plc_reader, sample_spec,
                         FrameSz6 - LookaheadSz, FrameSz6 - LookaheadSz, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_samples(*frame, FrameSz6 - LookaheadSz, 0.44f);

        LONGS_EQUAL(8, mock_reader.total_reads());
        LONGS_EQUAL(0, mock_reader.num_unread());

        LONGS_EQUAL(FrameSz1 + FrameSz2 + FrameSz4 + FrameSz6,
                    mock_plc.n_history_samples());
        LONGS_EQUAL(FrameSz3 + FrameSz5, mock_plc.n_lost_samples());
        LONGS_EQUAL(0, mock_plc.n_prev_samples());
        LONGS_EQUAL(0, mock_plc.n_next_samples());
    }
}

// Underlying reader doesn't fill capture timestamps.
TEST(plc_reader, without_cts) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec sample_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);

    test::MockReader mock_reader(frame_factory, sample_spec);
    MockPlc mock_plc(sample_spec, arena);

    add_samples(mock_reader, FrameSz, 0.11f, Frame::HasSignal);
    add_samples(mock_reader, FrameSz, 0.00f, Frame::HasGaps);
    add_samples(mock_reader, FrameSz, 0.33f, Frame::HasSignal);
    add_samples(mock_reader, FrameSz, 0.44f, Frame::HasSignal);

    mock_plc.set_lookbehind(FrameSz);
    mock_plc.set_lookahead(FrameSz);
    mock_plc.set_fill_value(0.22f);
    mock_plc.set_expected_prev_value(0.11f);
    mock_plc.set_expected_next_value(0.33f);

    PlcReader plc_reader(mock_reader, frame_factory, mock_plc, sample_spec);
    LONGS_EQUAL(status::StatusOK, plc_reader.init_status());

    { // frame 1: signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        LONGLONGS_EQUAL(0, frame->capture_timestamp());
    }

    { // frame 2: gap
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasGaps, frame->flags());
        LONGLONGS_EQUAL(0, frame->capture_timestamp());
    }

    { // frame 3 (first half): signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec,
                                      FrameSz / 2, FrameSz / 2, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        LONGLONGS_EQUAL(0, frame->capture_timestamp());
    }

    { // frame 3 (second half): signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec,
                                      FrameSz / 2, FrameSz / 2, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        LONGLONGS_EQUAL(0, frame->capture_timestamp());
    }

    { // frame 4: signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        LONGLONGS_EQUAL(0, frame->capture_timestamp());
    }

    LONGS_EQUAL(FrameSz * 3, mock_plc.n_history_samples());
    LONGS_EQUAL(FrameSz, mock_plc.n_lost_samples());
}

// Underlying reader fills capture timestamps.
TEST(plc_reader, with_cts) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec sample_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);

    test::MockReader mock_reader(frame_factory, sample_spec);
    MockPlc mock_plc(sample_spec, arena);

    add_samples(mock_reader, FrameSz, 0.11f, Frame::HasSignal);
    add_samples(mock_reader, FrameSz, 0.00f, Frame::HasGaps);
    add_samples(mock_reader, FrameSz, 0.33f, Frame::HasSignal);
    add_samples(mock_reader, FrameSz, 0.44f, Frame::HasSignal);

    mock_plc.set_lookbehind(FrameSz);
    mock_plc.set_lookahead(FrameSz);
    mock_plc.set_fill_value(0.22f);
    mock_plc.set_expected_prev_value(0.11f);
    mock_plc.set_expected_next_value(0.33f);

    PlcReader plc_reader(mock_reader, frame_factory, mock_plc, sample_spec);
    LONGS_EQUAL(status::StatusOK, plc_reader.init_status());

    const core::nanoseconds_t start_cts = 1000000;
    const core::nanoseconds_t frame_ns = sample_spec.samples_overall_2_ns(FrameSz);

    mock_reader.enable_timestamps(start_cts);

    { // frame 1: signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        LONGLONGS_EQUAL(start_cts, frame->capture_timestamp());
    }

    { // frame 2: gap
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasGaps, frame->flags());
        LONGLONGS_EQUAL(start_cts + frame_ns, frame->capture_timestamp());
    }

    { // frame 3 (first half): signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec,
                                      FrameSz / 2, FrameSz / 2, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        LONGLONGS_EQUAL(start_cts + frame_ns * 2, frame->capture_timestamp());
    }

    { // frame 3 (second half): signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec,
                                      FrameSz / 2, FrameSz / 2, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        LONGLONGS_EQUAL(start_cts + frame_ns * 2 + frame_ns / 2,
                        frame->capture_timestamp());
    }

    { // frame 4: signal
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        LONGLONGS_EQUAL(start_cts + frame_ns * 3, frame->capture_timestamp());
    }

    LONGS_EQUAL(FrameSz * 3, mock_plc.n_history_samples());
    LONGS_EQUAL(FrameSz, mock_plc.n_lost_samples());
}

// Non-raw PCM format.
TEST(plc_reader, non_raw_format) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec int_spec(MaxSz, PcmSubformat_SInt16, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    IntReader<int16_t> int_reader(int_spec);
    IntPlc<int16_t> int_plc(int_spec, FrameSz, arena);

    PlcReader plc_reader(int_reader, frame_factory, int_plc, int_spec);
    LONGS_EQUAL(status::StatusOK, plc_reader.init_status());

    { // frame 1: signal
        int_reader.add_return_frame(1111, Frame::HasSignal);
        LONGS_EQUAL(1, int_reader.n_unread_frames());

        FramePtr frame = expect_frame(status::StatusOK, plc_reader, int_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_int_samples<int16_t>(*frame, FrameSz, 1111);

        LONGS_EQUAL(0, int_reader.n_unread_frames());
    }

    { // frame 2: gap
        int_reader.add_return_frame(0, Frame::HasGaps);
        int_reader.add_return_frame(3333, Frame::HasSignal);
        int_plc.set_fill_value(2222);
        LONGS_EQUAL(2, int_reader.n_unread_frames());

        // will do read + read-ahead
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, int_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasGaps, frame->flags());
        expect_int_samples<int16_t>(*frame, FrameSz, 2222);

        LONGS_EQUAL(0, int_reader.n_unread_frames());
    }

    { // frame 3 (first half): signal
        LONGS_EQUAL(0, int_reader.n_unread_frames());

        // returns samples from last read-ahead
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, int_spec, FrameSz / 2,
                                      FrameSz / 2, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_int_samples<int16_t>(*frame, FrameSz / 2, 3333);

        LONGS_EQUAL(0, int_reader.n_unread_frames());
    }

    { // frame 3 (second half): signal
        LONGS_EQUAL(0, int_reader.n_unread_frames());

        // returns samples from last read-ahead
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, int_spec, FrameSz / 2,
                                      FrameSz / 2, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_int_samples<int16_t>(*frame, FrameSz / 2, 3333);

        LONGS_EQUAL(0, int_reader.n_unread_frames());
    }

    { // frame 4: signal
        int_reader.add_return_frame(4444, Frame::HasSignal);
        LONGS_EQUAL(1, int_reader.n_unread_frames());

        FramePtr frame = expect_frame(status::StatusOK, plc_reader, int_spec, FrameSz,
                                      FrameSz, ModeHard);

        LONGS_EQUAL(Frame::HasSignal, frame->flags());
        expect_int_samples<int16_t>(*frame, FrameSz, 4444);

        LONGS_EQUAL(0, int_reader.n_unread_frames());
    }
}

// Check every supported backend.
TEST(plc_reader, supported_backends) {
    enum { FrameSz = MaxSz / 2, NumFrames = 5, NumIters = 10 };

    const SampleSpec sample_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);

    for (size_t n_back = 0; n_back < n_supported_backends; n_back++) {
        test::MockReader mock_reader(frame_factory, sample_spec);

        PlcConfig plc_config;
        plc_config.backend = supported_backends[n_back];

        core::ScopedPtr<IPlc> plc(
            processor_map.new_plc(plc_config, sample_spec, frame_factory, arena));
        CHECK(plc);
        LONGS_EQUAL(status::StatusOK, plc->init_status());

        PlcReader plc_reader(mock_reader, frame_factory, *plc, sample_spec);
        LONGS_EQUAL(status::StatusOK, plc_reader.init_status());

        for (size_t i = 0; i < NumIters; i++) {
            add_samples(mock_reader, FrameSz, 0.00f, Frame::HasGaps);
            add_samples(mock_reader, FrameSz, 0.11f, Frame::HasSignal);
            add_samples(mock_reader, FrameSz, 0.00f, Frame::HasGaps);
            add_samples(mock_reader, FrameSz, 0.11f, Frame::HasSignal);
            add_samples(mock_reader, FrameSz, 0.11f, Frame::HasSignal);
        }

        for (size_t i = 0; i < NumFrames * NumIters; i++) {
            FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec,
                                          FrameSz, FrameSz, ModeHard);

            if (i == 0) {
                // Initial gap remains zero.
                expect_zero_samples(*frame, FrameSz);
            } else {
                // Every other frame has non-zero samples.
                expect_non_zero_samples(*frame, FrameSz);
            }
        }

        LONGS_EQUAL(NumFrames * NumIters, mock_reader.total_reads());
        LONGS_EQUAL(0, mock_reader.num_unread());
    }
}

// Forwarding mode to underlying reader.
TEST(plc_reader, forward_mode) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec sample_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);

    test::MockReader mock_reader(frame_factory, sample_spec);
    MockPlc mock_plc(sample_spec, arena);

    add_samples(mock_reader, FrameSz * 10, 0.00f, Frame::HasSignal);

    PlcReader plc_reader(mock_reader, frame_factory, mock_plc, sample_spec);
    LONGS_EQUAL(status::StatusOK, plc_reader.init_status());

    const FrameReadMode mode_list[] = {
        ModeHard,
        ModeSoft,
    };

    for (size_t md_n = 0; md_n < ROC_ARRAY_SIZE(mode_list); md_n++) {
        FramePtr frame = expect_frame(status::StatusOK, plc_reader, sample_spec, FrameSz,
                                      FrameSz, mode_list[md_n]);

        LONGS_EQUAL(mode_list[md_n], mock_reader.last_mode());
    }
}

// Forwarding error from underlying reader.
TEST(plc_reader, forward_error) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec sample_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);

    test::MockReader mock_reader(frame_factory, sample_spec);
    MockPlc mock_plc(sample_spec, arena);

    add_samples(mock_reader, FrameSz * 10, 0.00f, Frame::HasSignal);

    PlcReader plc_reader(mock_reader, frame_factory, mock_plc, sample_spec);
    LONGS_EQUAL(status::StatusOK, plc_reader.init_status());

    const status::StatusCode status_list[] = {
        status::StatusDrain,
        status::StatusAbort,
    };

    for (size_t st_n = 0; st_n < ROC_ARRAY_SIZE(status_list); st_n++) {
        mock_reader.set_status(status_list[st_n]);

        FramePtr frame = expect_frame(status_list[st_n], plc_reader, sample_spec, FrameSz,
                                      FrameSz, ModeHard);
    }
}

// Forwarding partial read from underlying reader.
TEST(plc_reader, forward_partial) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec sample_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);

    test::MockReader mock_reader(frame_factory, sample_spec);
    MockPlc mock_plc(sample_spec, arena);

    add_samples(mock_reader, FrameSz / 2, 0.00f, Frame::HasSignal);

    PlcReader plc_reader(mock_reader, frame_factory, mock_plc, sample_spec);
    LONGS_EQUAL(status::StatusOK, plc_reader.init_status());

    FramePtr frame = expect_frame(status::StatusPart, plc_reader, sample_spec, FrameSz,
                                  FrameSz / 2, ModeHard);

    LONGS_EQUAL(status::StatusPart, mock_reader.last_status());

    LONGS_EQUAL(1, mock_reader.total_reads());
    LONGS_EQUAL(0, mock_reader.num_unread());
}

// Attach to frame pre-allocated buffers of different sizes before reading.
TEST(plc_reader, preallocated_buffer) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec sample_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);

    const size_t buffer_list[] = {
        FrameSz * 50, // big size (reader should use it)
        FrameSz,      // exact size (reader should use it)
        FrameSz - 1,  // small size (reader should replace buffer)
        0,            // no buffer (reader should allocate buffer)
    };

    for (size_t bn = 0; bn < ROC_ARRAY_SIZE(buffer_list); bn++) {
        const size_t orig_buf_sz = buffer_list[bn];

        test::MockReader mock_reader(frame_factory, sample_spec);
        add_samples(mock_reader, FrameSz, 0.00f, Frame::HasSignal);

        MockPlc mock_plc(sample_spec, arena);
        PlcReader plc_reader(mock_reader, frame_factory, mock_plc, sample_spec);
        LONGS_EQUAL(status::StatusOK, plc_reader.init_status());

        FrameFactory mock_factory(arena, orig_buf_sz * sizeof(sample_t));
        FramePtr frame = orig_buf_sz > 0 ? mock_factory.allocate_frame(0)
                                         : mock_factory.allocate_frame_no_buffer();

        core::Slice<uint8_t> orig_buf = frame->buffer();

        LONGS_EQUAL(status::StatusOK, plc_reader.read(*frame, FrameSz, ModeHard));

        CHECK(frame->buffer());

        if (orig_buf_sz >= FrameSz) {
            CHECK(frame->buffer() == orig_buf);
        } else {
            CHECK(frame->buffer() != orig_buf);
        }

        LONGS_EQUAL(FrameSz, frame->duration());
        LONGS_EQUAL(FrameSz, frame->num_raw_samples());
        LONGS_EQUAL(FrameSz * sizeof(sample_t), frame->num_bytes());
    }
}

} // namespace audio
} // namespace roc
