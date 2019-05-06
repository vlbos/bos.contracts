#pragma once
#include <eosiolib/eosio.hpp>
using std::vector;

typedef std::vector<char> bytes;

uint64_t get_hash_key(checksum256 hash)
{
  const uint64_t *p64 = reinterpret_cast<const uint64_t *>(&hash);
  return p64[0] ^ p64[1] ^ p64[2] ^ p64[3];
}
