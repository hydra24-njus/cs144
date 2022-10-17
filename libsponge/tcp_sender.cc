#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <iostream>
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
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const {
    uint64_t ret=0;
    for(auto seg:_retrans_buf){
        //cout<<seg.length_in_sequence_space()<<"\t"<<seg.payload().copy()<<endl;
        ret+=seg.length_in_sequence_space();
    }
    return ret;
}

void TCPSender::fill_window() {
    size_t remain_size=_window_size==0?1:_window_size;
    if(remain_size>bytes_in_flight())remain_size-=bytes_in_flight();
    else return;
    while(remain_size>0){
        TCPSegment seg;
        TCPHeader &header=seg.header();
        if(!_syn_sent){
            _syn_sent=true;
            header.syn=true;
            remain_size--;
        }
        header.seqno=wrap(_next_seqno,_isn);
        string buf=stream_in().read(min(remain_size,TCPConfig::MAX_PAYLOAD_SIZE));
        remain_size-=buf.size();
        seg.payload()=move(buf);
        if(!_fin_sent&&remain_size>0&&stream_in().eof()){
            remain_size--;
            _fin_sent=true;
            header.fin=true;
        }
        if(seg.length_in_sequence_space()==0)break;
        //cout<<seg.header().syn<<" "<<seg.header().fin<<"\t"<<seg.payload().copy()<<endl;
        _segments_out.emplace(seg);
        _retrans_buf.emplace_back(seg);
        _next_seqno+=seg.length_in_sequence_space();
        if(!_timer.statu()){
            _timer.start(_initial_retransmission_timeout);
        }
    }
    
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    _window_size=window_size;
    uint64_t ackno_seq=unwrap(ackno,_isn,_next_seqno);
    if(ackno_seq>_next_seqno){
        //debug.
        return;
    }
    bool flag=false;
    for(auto it=_retrans_buf.begin();it!=_retrans_buf.end();){
        TCPSegment seg=*it;
        if(unwrap(seg.header().seqno,_isn,_next_seqno)+seg.length_in_sequence_space()<=ackno_seq){
            it=_retrans_buf.erase(it);
            flag=true;
        }
        else break;
    }
    if(_retrans_buf.empty()){
        _timer.reset();
        _retrans_count=0;
        return;
    }
    if(flag){
        _timer.start(_initial_retransmission_timeout);
        _retrans_count=0;
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    //cout<<"tick"<<endl;
    _timer.update(ms_since_last_tick);
    if(_timer.trip()){
        _segments_out.emplace(_retrans_buf.front());
        if(_window_size>0)_retrans_count++;
        _timer.start(_initial_retransmission_timeout<<_retrans_count);
    }
}

unsigned int TCPSender::consecutive_retransmissions() const {
    return _retrans_count;
}

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno=wrap(_next_seqno,_isn);
    _segments_out.push(seg);
}
