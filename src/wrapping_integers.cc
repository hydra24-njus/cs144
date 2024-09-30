#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return Wrap32 { static_cast<uint32_t>( n + zero_point.raw_value_ ) };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  int32_t tmp = raw_value_ - wrap( checkpoint, zero_point ).raw_value_;
  int64_t ans = checkpoint + tmp;
  return ans >= 0 ? ans : ( ans + ( 1ul << 32 ) );
}
