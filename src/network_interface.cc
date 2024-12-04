#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  // Your code here.
  const uint32_t next_hop_ip = next_hop.ipv4_numeric();
  auto it = _arp_table.find( next_hop_ip );
  if ( it != _arp_table.end() ) {
    EthernetFrame frame;
    frame.header = { it->second.addr, ethernet_address_, EthernetHeader::TYPE_IPv4 };
    frame.payload = serialize( dgram );
    transmit( frame );
  } else {
    _frame_buffer.push_back( { next_hop, dgram } );
    if ( _waiting_list.find( next_hop_ip ) == _waiting_list.end() ) {
      _waiting_list[next_hop_ip] = _timer;
      EthernetFrame frame = ARP_Request( next_hop_ip );
      transmit( frame );
    }
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Your code here.

  if ( frame.header.dst != ETHERNET_BROADCAST && frame.header.dst != ethernet_address_ )
    return;
  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram dtm;
    if ( parse( dtm, frame.payload ) ) {
      datagrams_received_.push( dtm );
    } else
      return;
  } else if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage msg;
    if ( parse( msg, frame.payload ) ) {
      auto sender_mac = msg.sender_ethernet_address;
      auto sender_ip = msg.sender_ip_address;
      if ( msg.opcode == ARPMessage::OPCODE_REQUEST && msg.target_ip_address == ip_address_.ipv4_numeric() ) {
        ARPMessage reply_msg;
        reply_msg.opcode = ARPMessage::OPCODE_REPLY;
        reply_msg.sender_ethernet_address = ethernet_address_;
        reply_msg.sender_ip_address = ip_address_.ipv4_numeric();
        reply_msg.target_ethernet_address = sender_mac;
        reply_msg.target_ip_address = sender_ip;
        EthernetFrame reply_frame;
        reply_frame.header = { sender_mac, ethernet_address_, EthernetHeader::TYPE_ARP };
        reply_frame.payload = serialize( reply_msg );
        transmit( reply_frame );
        _arp_table[sender_ip] = { sender_mac, _timer };
      } else if ( msg.opcode == ARPMessage::OPCODE_REPLY ) {
        _arp_table[sender_ip] = { sender_mac, _timer };
        _waiting_list.erase( sender_ip );
        for ( auto it = _frame_buffer.begin(); it != _frame_buffer.end(); ) {
          if ( sender_ip == it->first.ipv4_numeric() ) {
            EthernetFrame reply_frame;
            reply_frame.header = { sender_mac, ethernet_address_, EthernetHeader::TYPE_IPv4 };
            reply_frame.payload = serialize( it->second );
            transmit( reply_frame );
            it = _frame_buffer.erase( it );
          } else
            it++;
        }
      }
    } else
      return;
  }
  return;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  _timer += ms_since_last_tick;
  for ( auto it = _arp_table.begin(); it != _arp_table.end(); ) {
    if ( _timer - it->second.life >= 30000 ) {
      it = _arp_table.erase( it );
    } else
      it++;
  }
  for ( auto it = _waiting_list.begin(); it != _waiting_list.end(); it++ ) {
    if ( _timer - it->second >= 5000 ) {
      auto frame = ARP_Request( it->first );
      transmit( frame );
      it->second = _timer;
    }
  }
}

inline EthernetFrame NetworkInterface::ARP_Request( uint32_t ip )
{
  EthernetFrame frame;
  frame.header = { ETHERNET_BROADCAST, ethernet_address_, EthernetHeader::TYPE_ARP };
  ARPMessage msg;
  msg.opcode = ARPMessage::OPCODE_REQUEST;
  msg.sender_ethernet_address = ethernet_address_;
  msg.sender_ip_address = ip_address_.ipv4_numeric();
  msg.target_ethernet_address = { 0, 0, 0, 0, 0, 0 };
  msg.target_ip_address = ip;
  frame.payload = serialize( msg );
  return frame;
}