#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check`.


using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const {
    return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const {
    return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const {
    return _receiver.unassembled_bytes();
}

size_t TCPConnection::time_since_last_segment_received() const {
    return _time_received;
}

void TCPConnection::segment_received(const TCPSegment &seg) {
    _time_received=0;
    const TCPHeader &header=seg.header();
    if(header.rst){
        //do something
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _active=false;
        return;
    }
    _receiver.segment_received(seg);
    if(header.ack){
        _sender.ack_received(header.ackno,header.win);
    }
    if(seg.length_in_sequence_space()>0&&_sender.segments_out().size()==0){
        _sender.fill_window();
        if(_sender.segments_out().empty()){
            _sender.send_empty_segment();
        }
    }
    seg_send();
    if(_receiver.stream_out().input_ended()&&!_sender.stream_in().input_ended()){
        _linger_after_streams_finish=false;
    }
}
void TCPConnection::seg_send(){
    auto &seg_queue=_sender.segments_out();
    while(!seg_queue.empty()){
        auto &seg=seg_queue.front();
        auto &header=seg.header();
        header.win=_receiver.window_size()>UINT16_MAX?UINT16_MAX:_receiver.window_size();
        if(_receiver.ackno().has_value()){
            header.ack=true;
            header.ackno=_receiver.ackno().value();
        }
        _segments_out.push(seg);
        seg_queue.pop();
    }
}
bool TCPConnection::should_shutdown(){
    return (_receiver.stream_out().input_ended()&&_receiver.unassembled_bytes()==0)
        && (_sender.stream_in().input_ended()&&_sender.stream_in().buffer_empty()&&_sender.bytes_in_flight()==0)
        && (_sender.next_seqno_absolute()==_sender.stream_in().bytes_written()+2);
}
bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    size_t size=_sender.stream_in().write(data);
    _sender.fill_window();
    seg_send();
    return size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _time_received+=ms_since_last_tick;
    if(_sender.consecutive_retransmissions()>=_cfg.MAX_RETX_ATTEMPTS){
        _sender.send_empty_segment(true);
        seg_send();
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _active=false;
        return;
    }
    _sender.tick(ms_since_last_tick);
    if(should_shutdown()){
        if(_linger_after_streams_finish){
            if(_time_received>=10*_cfg.rt_timeout)
                _active=false;
        }
        else _active=false;
    }
    seg_send();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    seg_send();
}

void TCPConnection::connect() {
    _sender.fill_window();
    seg_send();
    _active=true;
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            _sender.send_empty_segment(true);
            seg_send();
            _sender.stream_in().set_error();
            _receiver.stream_out().set_error();
            _active = false;
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
