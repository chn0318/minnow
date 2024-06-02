#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <vector>
using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{

  return seq_number_in_flight;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  if (Impossible_ack || Send_FIN){
    return;
  }
  if (Connecting && !Connected)
  {
    return;
  }
  
  // caculate the range of data which need to be sent
  bool end = false;
  vector<TCPSenderMessage> Q;
  uint64_t abs_start_index = reader().bytes_popped() + 1;
  //if connection doesn't set up yet
  if (!Connecting)
  {
    RTO = initial_RTO_ms_;
    TCPSenderMessage message;
    message.SYN = true;
    message.seqno = Wrap32::wrap(0, isn_);
    end = true;
    Q.push_back(message);
  }
  else{
    string_view data = reader().peek();
    if (data.length() == 0)
    {
      TCPSenderMessage message;
      message.seqno = Wrap32::wrap(abs_start_index, isn_);
      end = true;
      Q.push_back(message);
    }
    else{
      //construct payload
      uint64_t abs_end_index = Curr + Window_size -1;
      uint64_t L = abs_end_index - abs_start_index +1 > 0 ?  abs_end_index - abs_start_index +1 : 0;
      string payload(data.substr(0, L));
      //special case: window size = 0, send 1 byte to get ack
      if (Window_size == 0)
      {
        TCPSenderMessage message;
        string temp(data.substr(0,1));
        if (!stuck_in_zero_window)
        {
          message.payload = temp;
          stuck_in_zero_window = true;
        }
        message.seqno = Wrap32::wrap(abs_start_index, isn_);
        Q.push_back(message);
        end = true;
      }else{
        if (payload.length() == data.length())
        {
          end = true;
        }
        uint64_t payload_length = min(payload.length(), TCPConfig::MAX_PAYLOAD_SIZE);
        uint64_t i = 0;
        do
        {
          TCPSenderMessage message;
          message.seqno = Wrap32::wrap(i+abs_start_index, isn_);
          message.payload = payload.substr(i, min(payload_length, payload.length() - i));
          Q.push_back(message);
          i+=payload_length;
        } while (i < payload.length());
      }
    }
  }
  if (writer().is_closed() && end)
  {
      if (!Q.empty())
      {
          TCPSenderMessage message = Q.back();
          Q.pop_back();
          uint64_t abs_end_index = Curr + Window_size -1;
          if (Window_size == 0)
          {
            abs_end_index++;
          }
          
          uint64_t abs_end_message = (message.seqno + (message.sequence_length() -1)).unwrap(isn_, Curr);
          if (Connecting)
          {
            if (abs_end_message >= abs_end_index){
                message.FIN = false;
            }
            else{
                message.FIN = true;
                Send_FIN = true;
            }
          }
          else{
            message.FIN = true;
            Send_FIN = true;
          }
          Q.push_back(message);
      }
  }

  if (!Q.empty() && Q.back().sequence_length() == 0)
  {
      Q.pop_back();
  }
  if (reader().has_error())
  {
    TCPSenderMessage message;
    message.seqno = Wrap32::wrap(abs_start_index, isn_);
    message.RST = true;
    transmit(message);
    if (!Timer.alive)
    {
        Timer.alive = true;
        Timer.acc = 0;
    }
    Outstanding.push(message);
    seq_number_in_flight += message.sequence_length();
  }else{
    for (auto &message : Q)
    {
        transmit(message);
        if (!Timer.alive)
        {
          Timer.alive = true;
          Timer.acc = 0;
        }
        Outstanding.push(message);
        seq_number_in_flight += message.sequence_length();
        input_.reader().pop(message.payload.length());
    }
  }
  

  Connecting = true;
  return;
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage message;
  uint64_t abs_start_index = reader().bytes_popped() + 1 + Send_FIN;
  message.seqno = Wrap32::wrap(abs_start_index, isn_);
  if (reader().has_error())
  {
    message.RST = true;
  }
  
  return message;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  //update window range
  //translate ack seqno to absolute seqno
  if (msg.RST)
  {
    writer().set_error();
  }
  
  if (!msg.ackno.has_value())
  {
    Window_size = msg.window_size;
    return;
  }
  
  uint64_t abs_ack = msg.ackno.value().unwrap(isn_, Curr);  
  uint64_t abs_start_index = reader().bytes_popped() + 1;
  if (Send_FIN)
  {
    abs_start_index++;
  }
  
  if (abs_ack >= Curr && abs_ack <= abs_start_index)
  {
    Impossible_ack = false;
    if (!Connected)
    {
      Connected = true;
    }
    
    Curr = abs_ack;
    Window_size = msg.window_size;
    stuck_in_zero_window = false;
    
    consecutive = 0;
    RTO = initial_RTO_ms_;
    //update the outstanding segment
    while (!Outstanding.empty())
    {
      Wrap32 seq_end = Outstanding.front().seqno + Outstanding.front().sequence_length();
      uint64_t abs_segment_end = seq_end.unwrap(isn_, Curr);
      if (Curr >= abs_segment_end)
      {
        seq_number_in_flight -= Outstanding.front().sequence_length();
        Outstanding.pop();
        Timer.acc = 0;
      }else
      {
        break;
      }
    }
    if (Outstanding.empty())
    {
      Timer.alive = false;
      Timer.acc = 0;
    }
  }else{
    if (abs_ack > abs_start_index)
    {
          Impossible_ack = true;
    }
    

  }

  
  return;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  if (!Timer.alive)
  {
    return;
  }
  
  Timer.acc += ms_since_last_tick;
  if (Timer.acc >= RTO)
  {
    transmit(Outstanding.front());
    if (Window_size!=0 || !Connected)
    {
      RTO = RTO* 2;
      consecutive++;
    }
    Timer.acc = 0;
  }
  return;
}
