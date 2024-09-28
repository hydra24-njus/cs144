#include "reassembler.hh"
#include <cassert>

using namespace std;

#define first_unassembled_index ( output_.writer().bytes_pushed() )
#define first_unacceptable_index ( output_.writer().bytes_pushed() + output_.writer().available_capacity() )

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // mark the _eof_index
  if ( is_last_substring ) {
    _eof_index = first_index + data.length();
    if ( _eof_index == first_unassembled_index ) {
      output_.writer().close();
      return;
    }
  }
  // ignore the unvalid input.
  if ( data.empty() || first_index >= first_unacceptable_index ) {
    return;
  }
  // the string should not be buffed by Reassembler.
  if ( first_index <= first_unassembled_index ) {
    // the string Reassembler received is already pushed into ByteStream.
    // so just ignore it.
    if ( first_index + data.length() < first_unassembled_index ) {
      return;
    }
    // the string received has half pushed,
    // so push the rest part of string first.
    output_.writer().push( data.substr( first_unassembled_index - first_index ) );

    // pending if any buffered string has being covered by the newest string.
    auto it = _buf.begin();
    while ( it != _buf.end() && it->first + it->second.length() <= first_unassembled_index ) {
      it = _buf.erase( it );
    }
    // pending if any buffered string can be pushed into ByteStream.
    if ( !_buf.empty() && _buf.begin()->first <= first_unassembled_index ) {
      output_.writer().push( _buf.begin()->second.substr( first_unassembled_index - _buf.begin()->first ) );
      _buf.erase( _buf.begin() );
    }
    // pending if output_ can be closed
    if ( _buf.empty() && first_unassembled_index == _eof_index ) {
      output_.writer().close();
    }
  }
  // push the string to Reassembler's buf.
  else {

    // if the buf is empty, just add it in and return.
    data = data.substr( 0, first_unacceptable_index - first_index );

    auto it = _buf.lower_bound( first_index );
    while ( it != _buf.end() ) {
      // 判断是否可合并
      if ( first_index + data.length() < it->first )
        break;
      // 判断是否完全覆盖
      if ( first_index + data.length() <= it->first + it->second.length() ) {
        data += it->second.substr( first_index + data.length() - it->first );
      }
      it = _buf.erase( it );
    }
    auto result = _buf.insert( { first_index, data } );
    it = result.first;
    // 可能向前合并
    if ( it != _buf.begin() ) {
      auto prev = std::prev( it );
      // 判断是否重叠
      if ( prev->first + prev->second.length() >= it->first ) {
        // 判断是否完全覆盖
        if ( prev->first + prev->second.length() <= it->first + it->second.length() ) {
          prev->second += it->second.substr( prev->first + prev->second.length() - it->first );
        }
        _buf.erase( it );
      }
    }
  }
}
uint64_t Reassembler::bytes_pending() const
{
  size_t ans = 0;
  for ( const auto& entry : _buf ) {
    ans += entry.second.length();
  }
  return ans;
}
