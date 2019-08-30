#pragma once
/*

  bos_oracle



*/

#include <eosio/eosio.hpp>
//

using namespace eosio;

/**
 * @brief 推送数据
 * 
 */
struct [[eosio::table, eosio::contract("bos.oracle")]] oracle_data_record {
  uint64_t update_number;
  string value;
  uint64_t timestamp;

  uint64_t primary_key() const { return update_number; }

  EOSLIB_SERIALIZE(oracle_data_record, (update_number)(value)(timestamp))
};


struct [[eosio::table, eosio::contract("bos.oracle")]] oracle_request_data_record {
  uint64_t request_id;
  string value;
  uint64_t timestamp;

  uint64_t primary_key() const { return request_id; }

  EOSLIB_SERIALIZE(oracle_request_data_record, (request_id)(value)(timestamp))
};


// Multi index types definition
typedef eosio::multi_index<"oracledata"_n, oracle_data_record>    oracle_data;
typedef eosio::multi_index<"oraclereq"_n, oracle_request_data_record>    oracle_request_data;
