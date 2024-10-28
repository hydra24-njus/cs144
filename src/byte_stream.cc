#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  // Your code here.
  return _eof;
}

void Writer::push( string data )
{
  // Your code here.
  if ( is_closed() || data.empty() )
    return;
  size_t bytes_to_write = min( data.length(), available_capacity() );
  _buf.append( data.substr( 0, bytes_to_write ) );
  _bytes_written += bytes_to_write;
  return;
}

void Writer::close()
{
  // Your code here.
  _eof = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - _buf.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return _bytes_written;
}

bool Reader::is_finished() const
{
  // Your code here.
  return _eof && _buf.empty();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return _bytes_read;
}

string_view Reader::peek() const
{
  return _buf;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  if ( len == 0 || _buf.empty() ) {
    return;
  }
  size_t bytes_to_pop = min( len, _buf.size() );
  _buf.erase( _buf.begin(), _buf.begin() + bytes_to_pop );
  _bytes_read += bytes_to_pop;
  return;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return _buf.length();
}
