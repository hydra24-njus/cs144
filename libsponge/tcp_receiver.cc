#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    TCPHeader _header = seg.header();
    if (!_syn ^ _header.syn)
        return;
    if (!_syn) {
        _syn = true;
        _seqno = _header.seqno;
    }
    _fin = _header.fin;
    uint64_t abs_seqno = unwrap(_header.seqno, _seqno, _reassembler.stream_out().bytes_written());
    uint64_t index = abs_seqno - (!_header.syn);
    _reassembler.push_substring(seg.payload().copy(), index, _fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_syn)
        return {};
    return wrap(stream_out().bytes_written() + _syn + _reassembler.stream_out().input_ended(), _seqno);
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
