#include "byte_stream.hh"
using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{

  return closed;
}

void Writer::push( string data )
{
  // Your code here.
  uint64_t L = min( data.length(), capacity_ - length );
  data = data.substr( 0, L );
  if ( L > 0 ) {
    buffer += data;
    length += L;
    total_push += L;
  }

  return;
}

void Writer::close()
{
  closed = true;
  return;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - length;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return total_push;
}

bool Reader::is_finished() const
{
  // Your code here.
  return closed && length == 0;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return total_pop;
}

string_view Reader::peek() const
{
  // Your code here.
  return string_view( buffer );
}

void Reader::pop( uint64_t len )
{
  buffer.erase( 0, len );
  length -= len;
  total_pop += len;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return length;
}
