#pragma once
#include "murmurhash.hpp"
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>



uint64_t get_hash_key(checksum256 hash) {
  const uint64_t *p64 = reinterpret_cast<const uint64_t *>(&hash);
  return p64[0] ^ p64[1] ^ p64[2] ^ p64[3];
}

uint32_t random(void *seed, size_t len) {
  checksum256 rand256;
  rand256 = sha256(static_cast<const char *>(seed), len);
  auto res = murmur_hash2(reinterpret_cast<const char *>(&rand256),
                          sizeof(rand256) / sizeof(char));
  return res;
}
