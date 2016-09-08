#pragma once

#include <cstdint>
#include <boost/multiprecision/integer.hpp>

namespace f2f { namespace util 
{

inline uint32_t HashFNV1a_32(const char * it, const char * end)
{
  uint32_t hash = 2166136261ui32;
  for (; it != end; ++it)
  {
    hash ^= *it;
    hash *= 16777619ui32;
  }

  return hash;
}

}}