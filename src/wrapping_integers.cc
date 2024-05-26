#include "wrapping_integers.hh"
#include<cmath>
using namespace std;

static uint64_t dis(uint64_t a, uint64_t b){
  if (a < b)
  {
    return b-a;
  }
  return a-b;
  
}
Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  uint32_t n_uint32 = static_cast<uint32_t>(n % (static_cast<uint64_t>(1) << 32));
  return zero_point + n_uint32;
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  uint64_t zero_point_uint64 = static_cast<uint64_t>(zero_point.raw_value_);
  uint64_t raw_value_uint64 = static_cast<uint64_t>(raw_value_);
  if (raw_value_uint64 < zero_point_uint64)
  {
    raw_value_uint64 += static_cast<uint64_t>(1)<<32;
  }
  uint64_t offset = raw_value_uint64 - zero_point_uint64;
  uint64_t N = checkpoint / (static_cast<uint64_t>(1)<<32);
  uint64_t ans = N * (static_cast<uint64_t>(1)<<32) + offset;
  if (N > 0)
  {
    uint64_t temp = ans - (static_cast<uint64_t>(1)<<32);
    if (dis(temp, checkpoint)<dis(ans,checkpoint))
    {
      ans = temp;
    }
  }
  uint64_t temp = (N+1) * (static_cast<uint64_t>(1)<<32) + offset;
  if (dis(temp, checkpoint) < dis(ans, checkpoint))
  {
    ans = temp;
  }
  return ans;
}
