#include "tcp_receiver.hh"

#include "stream_reassembler.hh"
#include "tcp_header.hh"
#include "wrapping_integers.hh"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <sys/types.h>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    DUMMY_CODE(seg);
    TCPHeader tcp_header = seg.header();
    string data = seg.payload().copy();
    WrappingInt32 payload_first_seqno = tcp_header.seqno;

    // syn hadn't arrived
    if (!tcp_header.syn && !syn_) {
        return;
    }

    // first segment with syn
    if (tcp_header.syn) {
        syn_ = true;
        isn_ = tcp_header.seqno;
        payload_first_seqno = tcp_header.seqno + 1;
    }

    if (tcp_header.fin) {
        fin_ = true;
    }

    uint64_t checkpoint = stream_out().bytes_written();
    uint64_t absolute_seqno = unwrap(payload_first_seqno, isn_, checkpoint);
    uint64_t stream_index = absolute_seqno - 1;

    _reassembler.push_substring(data, stream_index, tcp_header.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (syn_ == false) {
        // If the ISN hasn’t been set yet, return an empty optional.
        return {};
    }

    // absolute sequence number = stream index + 1
    uint64_t n = stream_out().bytes_written() + 1;

    // tests: recv_connect.cc -> 81
    // https://serverfault.com/a/981472
    // RFC793 FIN: A control bit (finis) occupying one sequence number, which
    // indicates that the sender will send no more data or control
    // occupying sequence space.
    if (fin_ && _reassembler.empty()) {
        n = n + 1;
    }
    return wrap(n, isn_);
}

size_t TCPReceiver::window_size() const { return _capacity - stream_out().buffer_size(); }
