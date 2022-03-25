#include "tcp_sender.hh"

#include "buffer.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , rto_(retx_timeout) {}

uint64_t TCPSender::bytes_in_flight() const { return count_bytes_in_flight_; }

void TCPSender::fill_window() {
    if (_next_seqno == 0) {
        TCPSegment seg;
        syn_ = true;
        seg.header().syn = true;
        send_segment(seg);
        return;
    } else if (_next_seqno == count_bytes_in_flight_) {
        return;
    }

    size_t window_size = receiver_windows_size_ == 0 ? 1 : receiver_windows_size_;
    while (window_size > 0 /* && !stream_in().buffer_empty() */) {
        TCPSegment seg;
        size_t payload_size =
            min(TCPConfig::MAX_PAYLOAD_SIZE, min(stream_in().buffer_size(), window_size - bytes_in_flight()));
        if (payload_size > 0) {
            seg.payload() = stream_in().read(payload_size);
        }
        if (!syn_) {
            seg.header().syn = true;
            syn_ = true;
        }
        if (!fin_ && stream_in().eof() && count_bytes_in_flight_ + seg.length_in_sequence_space() < window_size) {
            seg.header().fin = true;
            fin_ = true;
        }

        if (seg.length_in_sequence_space() == 0) {
            break;
        }
        send_segment(seg);
        if (fin_) {
            break;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    DUMMY_CODE(ackno, window_size);
    uint64_t abs_ackno = unwrap(ackno, _isn, _next_seqno);
    // failed tests: send_extra(FIN flag occupies space in window (part II)
    // When abs_ackno == recv_ackno_, windows_size changed
    if (abs_ackno > _next_seqno || abs_ackno < recv_ackno_) {
        return;
    }
    TCPSegment seg;
    while (!segments_outstanding_.empty()) {
        seg = segments_outstanding_.front();
        uint64_t seqno = unwrap(seg.header().seqno, _isn, _next_seqno) + seg.length_in_sequence_space();

        if (abs_ackno >= seqno) {
            recv_ackno_ = seqno;
            count_bytes_in_flight_ -= seg.length_in_sequence_space();
            segments_outstanding_.pop();
            // When the receiver gives the sender an ackno that acknowledges the successful receipt
            // of *new* data:
            // 7.a
            rto_ = _initial_retransmission_timeout;
            // 7.b
            if (!segments_outstanding_.empty()) {
                time_elapsed_ = 0;
            }
            // 7.c
            count_consecutive_retransmissions_ = 0;
        } else {
            break;
        }
    }
    /*
    // failed tests: send_extra(Timer doesn't restart without ACK of new data)
    // 7.a
    rto_ = _initial_retransmission_timeout;
    // 7.b
    if (!segments_outstanding_.empty()) {
        time_elapsed_ = 0;
    }
    // 7.c
    count_consecutive_retransmissions_ = 0;
    */
    receiver_windows_size_ = window_size;
    recv_ackno_ = abs_ackno;
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    DUMMY_CODE(ms_since_last_tick);
    if (!timer_running_) {
        return;
    }
    time_elapsed_ += ms_since_last_tick;
    if (time_elapsed_ >= rto_ && !segments_outstanding_.empty()) {
        _segments_out.push(segments_outstanding_.front());
        if (receiver_windows_size_ /* || segments_outstanding_.front().header().syn */) {
            rto_ <<= 1;
            count_consecutive_retransmissions_++;
        }
        time_elapsed_ = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return count_consecutive_retransmissions_; }

void TCPSender::send_empty_segment() {
    // sengments dont occupy seqno
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    _segments_out.push(seg);
}

void TCPSender::send_segment(TCPSegment &seg) {
    // any segments occupy seqno
    seg.header().seqno = next_seqno();
    _next_seqno += seg.length_in_sequence_space();
    count_bytes_in_flight_ += seg.length_in_sequence_space();

    _segments_out.push(seg);
    segments_outstanding_.push(seg);
    // Every time a segment containing data (nonzero length in sequence space) is sent
    // (whether it’s the first time or a retransmission), if the timer is not running, start it
    // running so that it will expire after RTO milliseconds (for the current value of RTO).
    if (!timer_running_) {
        timer_running_ = true;
        time_elapsed_ = 0;
    }
}
