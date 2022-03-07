#include "stream_reassembler.hh"

#include <cstddef>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : buffer_(capacity, '\0')
    , flag_(capacity, false)
    , count_unassembled_bytes_(0)
    , eof_(false)
    , _output(capacity)
    , _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    DUMMY_CODE(data, index, eof);

    size_t first_unassembled_ = _output.bytes_written();
    size_t first_unacceptable_ = first_unassembled_ + _capacity;
    size_t len = data.length();

    if (index + len < first_unassembled_) {
        return;
    }

    if (index >= first_unacceptable_) {
        return;
    }

    if (eof) {
        eof_ = true;
    }

    size_t begin_ = index < first_unassembled_ ? first_unassembled_ : index;
    size_t end_ = (index + len) >= first_unacceptable_ ? first_unacceptable_ : index + len;

    for (size_t i = begin_; i < end_; i++) {
        if (!flag_[i - first_unassembled_]) {
            buffer_[i - first_unassembled_] = data[i - index];
            count_unassembled_bytes_++;
            flag_[i - first_unassembled_] = true;
        }
    }

    string str = "";
    size_t remain = _output.remaining_capacity();
    while (flag_.front() && remain > 0) {
        str += buffer_.front();
        buffer_.pop_front();
        flag_.pop_front();
        buffer_.push_back('\0');
        flag_.push_back(false);
        remain--;
    }
    if (str.length() > 0) {
        _output.write(str);
        count_unassembled_bytes_ -= str.length();
    }

    if (eof_ && count_unassembled_bytes_ == 0) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return count_unassembled_bytes_; }

bool StreamReassembler::empty() const { return count_unassembled_bytes_ == 0; }
