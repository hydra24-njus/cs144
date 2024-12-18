#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  // Your code here.
 uint32_t dst = route_prefix & (prefix_length ? 0xffffffff << (32 - prefix_length) : 0);
    auto it = _route_table.find(dst);
    if (it == _route_table.end() || it->second.prefix_len <= prefix_length) {
        _route_table[dst] = {prefix_length, interface_num, next_hop};
    }
}

void Router::route_one_datagram(InternetDatagram &dgram) {
    // Your code here.
    const auto dst_ip = dgram.header.dst;
  auto result_it = _route_table.end();
  for ( auto it = _route_table.begin(); it != _route_table.end(); ++it ) {
    if ( it->second.prefix_len == 0 || ( ( it->first ^ dst_ip ) >> ( 32 - it->second.prefix_len ) ) == 0 ) {
      if ( result_it == _route_table.end() || it->second.prefix_len > result_it->second.prefix_len )
        result_it = it;
    }
  }

  if ( result_it != _route_table.end() && dgram.header.ttl > 1 ) {
    --dgram.header.ttl;
    dgram.header.compute_checksum();
    auto next_interface = interface(result_it->second.interface_num);
    next_interface->send_datagram( dgram, result_it->second.next_hop.value_or( Address::from_ipv4_numeric( dst_ip ) ) );
  }
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // Your code here.
  for (auto &interface : _interfaces) {
        auto &queue = interface->datagrams_received();
        while (! queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
