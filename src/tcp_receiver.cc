#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  
  if (message.SYN)
  {
    zero_point = message.seqno;
    init = true;
  }
  if (!init)
  {
    return;
  }
  
  rst = message.RST;
  
  uint64_t absolute_seqno = message.seqno.unwrap(zero_point, writer().bytes_pushed());
  uint64_t sqe_index = absolute_seqno - 1;
  if (message.SYN)
  {
    sqe_index++;
  }
  reassembler_.insert(sqe_index, message.payload, message.FIN);
  
  
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  TCPReceiverMessage message;
  if (init)
  {
    if (reader().is_finished())
    {
      message.ackno = Wrap32::wrap(writer().bytes_pushed()+2 , zero_point);
    }
    else{
      message.ackno = Wrap32::wrap(writer().bytes_pushed()+1 , zero_point);
    }
    
  }

  message.RST = rst;
  message.window_size = UINT16_MAX < writer().available_capacity() ? UINT16_MAX : writer().available_capacity();
  return message;
}
