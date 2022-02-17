#include "byte_stream.hh"

#include <cstddef>
#include <string>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : buffer_(), capacity_(capacity), is_input_ended_(false), total_bytes_written_(0), total_bytes_read_(0) {}

size_t ByteStream::write(const string &data) {
    // DUMMY_CODE(data);
    size_t remaining_capacity_ = remaining_capacity();
    size_t bytes_write_ = remaining_capacity_ > data.size() ? data.size() : remaining_capacity_;
    buffer_ += data.substr(0, bytes_write_);
    total_bytes_written_ += bytes_write_;
    return bytes_write_;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    // DUMMY_CODE(len);
    return buffer_.substr(0, min(buffer_size(), len));
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    // DUMMY_CODE(len);
    size_t bytes_read = buffer_size() > len ? len : buffer_size();
    total_bytes_read_ += bytes_read;
    buffer_.erase(0, bytes_read);
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    // DUMMY_CODE(len);
    size_t bytes_read = buffer_size() > len ? len : buffer_size();
    string ret = peek_output(bytes_read);
    pop_output(bytes_read);
    return ret;
}

void ByteStream::end_input() { is_input_ended_ = true; }

bool ByteStream::input_ended() const { return is_input_ended_; }

size_t ByteStream::buffer_size() const { return buffer_.size(); }

bool ByteStream::buffer_empty() const { return buffer_.empty(); }

bool ByteStream::eof() const { return is_input_ended_ && buffer_empty(); }

size_t ByteStream::bytes_written() const { return total_bytes_written_; }

size_t ByteStream::bytes_read() const { return total_bytes_read_; }

size_t ByteStream::remaining_capacity() const { return capacity_ - buffer_size(); }
