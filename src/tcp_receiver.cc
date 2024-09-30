#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if ( message.RST ) {
    reader().set_error();
  }
  if ( !( _syn ^ message.SYN ) ) {
    // 即未接收到syn或重复接收syn，直接忽略
    return;
  }
  if ( !_syn ) {
    // 即接收到syn
    _syn = true;
    _seqno = message.seqno;
  }
  _fin = message.FIN;
  uint64_t abs_seqno = message.seqno.unwrap( _seqno.value(), writer().bytes_pushed() );
  uint64_t index = abs_seqno - ( !message.SYN );
  reassembler_.insert( index, message.payload, _fin ); // 判断是否真正结束交由reassembler实现
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  TCPReceiverMessage msg;
  if ( writer().has_error() ) {
    msg.RST = true;
  }
  auto window_size = writer().available_capacity();
  msg.window_size = window_size > UINT16_MAX ? UINT16_MAX : window_size;
  if ( _seqno.has_value() ) {
    const auto abs_seqno = writer().bytes_pushed() + 1 + writer().is_closed();
    msg.ackno = Wrap32::wrap( abs_seqno, _seqno.value() );
  } else {
    msg.ackno = nullopt;
  }
  return msg;
}
