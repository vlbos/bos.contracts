#pragma once
#include <eosiolib/eosio.hpp>
using std::vector;

typedef std::vector<char> bytes;

enum data_service_fee_type : uint8_t { fee_count = 0, fee_month = 1 };

enum data_service_data_type : uint8_t {
  data_deterministic = 0,
  data_non_deterministic = 1
};

enum data_service_injection_method : uint8_t {
  chain_direct = 0,
  chain_indirect = 1,
  outside_chain = 2
};