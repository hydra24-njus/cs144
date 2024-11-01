#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>

class Timer
{
private:
  size_t _current_time { 0 };
  size_t _current_tout { 0 };
  bool status { false };

public:
  void shutdown()
  {
    _current_time = 0;
    _current_tout = 0;
    status = false;
  }
  void start( unsigned int timeout )
  {
    status = true;
    _current_time = 0;
    _current_tout = timeout;
  }
  void update( size_t uptime )
  {
    if ( !status )
      return;
    _current_time += uptime;
  }
  bool trip() { return status && ( _current_time >= _current_tout ); }
  bool state() { return status; }
};

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms )
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  std::vector<TCPSenderMessage> _retrans_buf {};

  unsigned int _retrans_cnt { 0 };

  uint64_t _next_seqno { 0 };
  uint32_t _window_size { 1 };

  bool _syn_sent { false };
  bool _fin_sent { false };

  Timer _timer {};
};
