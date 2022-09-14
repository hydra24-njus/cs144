#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity):_capacity(capacity),_buf() { }

size_t ByteStream::write(const string &data) {
    size_t bytes_to_write=min(data.length(),remaining_capacity());
    _buf.insert(_buf.end(),data.begin(),data.begin()+bytes_to_write);
    _bytes_written+=bytes_to_write;
    return bytes_to_write;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string ret=string(_buf.begin(),_buf.begin()+min(len,_buf.size()));
    return ret;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t bytes_to_read=min(len,_buf.size());
    _buf.erase(_buf.begin(),_buf.begin()+bytes_to_read);
    _bytes_read+=bytes_to_read;
    return;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string ret=peek_output(len);
    pop_output(ret.length());
    return ret;
}

void ByteStream::end_input() {_eof=true;}

bool ByteStream::input_ended() const { return _eof; }

size_t ByteStream::buffer_size() const { return _buf.size(); }

bool ByteStream::buffer_empty() const { return _buf.empty(); }

bool ByteStream::eof() const { return _eof&&_buf.empty(); }

size_t ByteStream::bytes_written() const { return _bytes_written; }

size_t ByteStream::bytes_read() const { return _bytes_read; }

size_t ByteStream::remaining_capacity() const { return _capacity-_buf.size(); }
