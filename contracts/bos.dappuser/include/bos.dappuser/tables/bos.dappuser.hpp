#pragma once
#include <eosio/eosio.hpp>
#include "bos.dappuser/bos.types.hpp"
#include "bos.oraclized.hpp"


// // @abi table args i64
struct request_args
{
  bytes schema;
  bytes args;
};

// carbon-copy call structure
struct  push_data
{
  name oracle;
  name contract;
  string task;
  string memo;
  bytes data;

  EOSLIB_SERIALIZE(push_data, (oracle)(contract)(task)(memo)(data))
};

struct  push_json
{
  uint64_t service_id;
  name provider;
  name contract_account;
  name action_name;
  uint64_t request_id;
  string data_json;
 
  EOSLIB_SERIALIZE(push_json, (service_id)(provider)(contract_account)(action_name)(request_id)(data_json))
};

struct price
{
  uint64_t value;
  uint8_t decimals;

  EOSLIB_SERIALIZE(price, (value)(decimals))
};

// @abi table ethbtc i64
struct [[eosio::table, eosio::contract("bos.dappuser")]] ethbtc
{
  uint32_t best_before;
  uint32_t update_after;
  price value;

  EOSLIB_SERIALIZE(ethbtc, (best_before)(update_after)(value))
};

typedef oraclized<("ethbtc"_n).value, 11, 10, price> ethbtc_data;

typedef singleton<"master"_n, name> account_master;
