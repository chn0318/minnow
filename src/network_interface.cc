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
  uint32_t ipaddr = next_hop.ipv4_numeric();
  EthernetFrame frame;
  Serializer serializer;
  auto iter = IP2Mac.find(ipaddr);
  if (iter!=IP2Mac.end())
  {
    frame.header.dst = iter->second.first;
    frame.header.src = ethernet_address_;
    frame.header.type = EthernetHeader::TYPE_IPv4;
    dgram.serialize(serializer);
    frame.payload = serializer.output();
    transmit(frame);
  }else{
    IP2DatagramBuffer[ipaddr].push_back(dgram);
    auto ptr = IP2Request.find(ipaddr);
    if (ptr!=IP2Request.end())
    {
      return;
    }
    frame.header.dst = ETHERNET_BROADCAST;
    frame.header.src = ethernet_address_;
    frame.header.type = EthernetHeader::TYPE_ARP;   
    ARPMessage msg;
    msg.opcode = ARPMessage::OPCODE_REQUEST;
    msg.sender_ethernet_address = ethernet_address_;
    msg.sender_ip_address = ip_address_.ipv4_numeric();
    msg.target_ip_address = ipaddr;
    msg.serialize(serializer);
    frame.payload = serializer.output();
    transmit(frame);
    IP2Request[ipaddr] = 5000;
  }
  
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if (frame.header.dst!=ETHERNET_BROADCAST && frame.header.dst!=ethernet_address_)
  {
    return;
  }
  Parser parser(frame.payload);
  if (frame.header.type == EthernetHeader::TYPE_IPv4)
  {
    InternetDatagram msg;
    msg.parse(parser);
    datagrams_received_.push(msg);
  }
  if (frame.header.type == EthernetHeader::TYPE_ARP)
  {
    ARPMessage msg;
    msg.parse(parser);
    //update IP2MAC table
    IP2Mac[msg.sender_ip_address] = make_pair(msg.sender_ethernet_address, 30000);
    // auto iter_ = IP2Request.find(msg.sender_ip_address);
    // if (iter_!=IP2Request.end())
    // {
    //   IP2Request.erase(iter_);
    // }
    //send pending InternetDatagram
    if (!IP2DatagramBuffer[msg.sender_ip_address].empty())
    {
      for (auto& elem : IP2DatagramBuffer[msg.sender_ip_address])
      {
          EthernetFrame sned_frame;
          Serializer serializer;
          sned_frame.header.dst = msg.sender_ethernet_address;
          sned_frame.header.src = ethernet_address_;
          sned_frame.header.type = EthernetHeader::TYPE_IPv4;
          elem.serialize(serializer);
          sned_frame.payload = serializer.output();
          transmit(sned_frame);
      }
      IP2DatagramBuffer[msg.sender_ip_address].clear();
    }
    //send APR_Reply Message
    if (msg.opcode == ARPMessage::OPCODE_REQUEST && msg.target_ip_address == ip_address_.ipv4_numeric())
    {
        EthernetFrame sned_frame;
        Serializer serializer;
        sned_frame.header.dst = msg.sender_ethernet_address;
        sned_frame.header.src = ethernet_address_;
        sned_frame.header.type = EthernetHeader::TYPE_ARP;
        ARPMessage reply_msg;
        reply_msg.opcode = ARPMessage::OPCODE_REPLY;
        reply_msg.sender_ethernet_address = ethernet_address_;
        reply_msg.sender_ip_address = ip_address_.ipv4_numeric();
        reply_msg.target_ethernet_address = msg.sender_ethernet_address;
        reply_msg.target_ip_address = msg.sender_ip_address;
        reply_msg.serialize(serializer);
        sned_frame.payload = serializer.output();
        transmit(sned_frame);
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  //traverse IP2MAC to update time counter
  for (auto iter = IP2Mac.begin(); iter!=IP2Mac.end();)
  {
      iter->second.second-= ms_since_last_tick;
      if (iter->second.second <= 0)
      {
        iter = IP2Mac.erase(iter);
      }else{
        iter++;
      }
  }
  //traverse IP2Request to update time counter
  for (auto iter = IP2Request.begin(); iter!=IP2Request.end();)
  {
      iter->second-= ms_since_last_tick;
      if (iter->second <= 0)
      {
        // iter->second = 5000;
        // //retransmit BroadCast request
        // EthernetFrame frame;
        // Serializer serializer;
        // frame.header.dst = ETHERNET_BROADCAST;
        // frame.header.src = ethernet_address_;
        // frame.header.type = EthernetHeader::TYPE_ARP;   
        // ARPMessage msg;
        // msg.opcode = ARPMessage::OPCODE_REQUEST;
        // msg.sender_ethernet_address = ethernet_address_;
        // msg.sender_ip_address = ip_address_.ipv4_numeric();
        // msg.target_ip_address = iter->first;
        // msg.serialize(serializer);
        // frame.payload = serializer.output();
        // transmit(frame);
        iter = IP2Request.erase(iter);
      }else{
        iter++;
      }
  }
  
}
