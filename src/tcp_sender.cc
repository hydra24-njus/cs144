#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  uint64_t ret = 0;
  for ( auto& seg : _retrans_buf ) {
    ret += seg.sequence_length();
  }
  return ret;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return _retrans_cnt;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.
  size_t remain_size = (_window_size == 0) ? 1 : _window_size;
    uint64_t tmp_size = sequence_numbers_in_flight();
    if (remain_size <= tmp_size)
        return;
    remain_size -= tmp_size;
    while (remain_size > 0) {
        TCPSenderMessage seg;
        seg.RST=input_.has_error();
        if (!_syn_sent) {
            _syn_sent = true;
            seg.SYN = true;
            remain_size--;
        }
        seg.seqno = seg.seqno.wrap(_next_seqno, isn_);
        string buf;
        read(input_.reader(),min(remain_size, TCPConfig::MAX_PAYLOAD_SIZE),buf);
        remain_size -= buf.size();
        seg.payload = move(buf);
        if (!_fin_sent && remain_size > 0 && reader().is_finished()) {
            remain_size--;
            _fin_sent = true;
            seg.FIN = true;
        }
        if (seg.sequence_length() == 0)
            break;
        _retrans_buf.emplace_back(seg);
        transmit(seg);
        _next_seqno += seg.sequence_length();
    }
    if (!_timer.state() && sequence_numbers_in_flight() != 0)
        _timer.start(initial_RTO_ms_);
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  TCPSenderMessage msg = { .seqno = Wrap32::wrap( _next_seqno, isn_ ),
                           .SYN = false,
                           .payload = string(),
                           .FIN = false,
                           .RST = input_.has_error() };
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  _window_size = msg.window_size;
  if ( msg.RST ) {
    input_.set_error();
  }
  if ( msg.ackno.has_value() ) {
  uint64_t ackno_seq = msg.ackno.value().unwrap( isn_, _next_seqno );
  if ( ackno_seq > _next_seqno )
    return; // TODO: Debug;
  bool flag = false;
  for ( auto it = _retrans_buf.begin(); it != _retrans_buf.end(); ) {
    auto& seg = *it;
    if ( seg.seqno.unwrap( isn_, _next_seqno ) + seg.sequence_length() <= ackno_seq ) {
      it = _retrans_buf.erase( it );
      flag = true;
    } else
      break;
  }
  if ( flag ) {
    _timer.shutdown();
    _retrans_cnt = 0;
  }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  _timer.update( ms_since_last_tick );
  if ( _timer.trip() ) {
    transmit( _retrans_buf.front() );
    if ( _window_size > 0 )
      _retrans_cnt++; // why not back off? Another question: should the _retrans_cnt++ if win_size==0?
    _timer.start( initial_RTO_ms_ << _retrans_cnt );
  }
}
