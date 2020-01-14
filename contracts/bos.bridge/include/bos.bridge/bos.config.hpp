#pragma once
#include <eosio/eosio.hpp>
#include <eosio/crypto.hpp>
#include <eosio/asset.hpp>
#include <eosio/symbol.hpp>
#include <eosio/system.hpp>
#include <cmath>

using bytes = std::vector<uint8_t>;


static const std::string default_core_symbol =  "BOS";
static const uint8_t default_precision =  4;
static const std::string chain_token =  "eth";
static const std::string address_zero =  "0";

static const uint8_t current_bridge_version = 1;

template<typename CharT>
static std::string to_hex(const CharT* d, uint32_t s) {
  std::string r;
  const char* to_hex="0123456789abcdef";
  uint8_t* c = (uint8_t*)d;
  for( uint32_t i = 0; i < s; ++i ) {
    (r += to_hex[(c[i] >> 4)]) += to_hex[(c[i] & 0x0f)];
  }
  return r;
}

bool operator<(const eosio::checksum256& lhs, const eosio::checksum256& rhs)
{
    return to_hex(&lhs, sizeof(lhs)) < to_hex(&rhs, sizeof(rhs));
}

// std::string data = "hello";
// checksum256 digest;
// sha256(&data[0], data.size(), &digest);

// std::string hexstr = to_hex(&digest, sizeof(digest));