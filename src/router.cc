#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

static uint32_t setLeadingBits(uint8_t n) {
    if (n <= 0) {
        return 0;
    }
    if (n >= 32) {
        return ~0U;
    }
    return (~0U << (32 - n));
}
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

  element item(route_prefix, prefix_length, next_hop, interface_num);
  router_table.push_back(item);
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for (size_t i = 0; i < _interfaces.size(); i++)
  {
    while (!interface(i)->datagrams_received().empty())
    {
      InternetDatagram msg = interface(i)->datagrams_received().front();
      interface(i)->datagrams_received().pop();
      // get the dst ip addr
      uint32_t dst_ip_addr = msg.header.dst;
      uint32_t max_length = 0;
      bool flag = false;
      uint32_t forward_addr = dst_ip_addr;
      size_t interface_num;
      //traverse router_table
      for (auto item : router_table)
      {
        uint32_t mask = setLeadingBits(item.prefix_length);
        if ((mask & item.route_prefix) == (mask & dst_ip_addr))
        {
          
          if (max_length < item.prefix_length || flag == false)
          {
            max_length = item.prefix_length;
            forward_addr = item.next_hop.has_value()?item.next_hop.value().ipv4_numeric():dst_ip_addr;
            interface_num = item.interface_num;
          }
          flag = true;
        }  
      }
      //drop package if router table do not has match item or ttl <= 0
      if (!flag)
      {
        continue;
      }
      if (msg.header.ttl <= 1)
      {
        continue;
      }
      msg.header.ttl--;
      interface(interface_num)->send_datagram(msg, Address::from_ipv4_numeric(forward_addr));
    }
    
  }
  
}
