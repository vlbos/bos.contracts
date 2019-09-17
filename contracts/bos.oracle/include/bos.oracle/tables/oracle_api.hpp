#pragma once
/*

  bos_oracle



*/

#include <eosio/eosio.hpp>
#include <string>
//

using namespace eosio;

struct  push_json
{
  uint64_t service_id;
  name provider;
  name contract_account;
  uint64_t request_id;
  std::string data_json;
 
  EOSLIB_SERIALIZE(push_json, (service_id)(provider)(contract_account)(request_id)(data_json))
};

/**
 * @brief 推送数据
 * 
 */
struct [[eosio::table, eosio::contract("bos.oracle")]] oracle_data_record {
  uint64_t update_number;
  std::string value;
  uint64_t timestamp;

  uint64_t primary_key() const { return update_number; }

  EOSLIB_SERIALIZE(oracle_data_record, (update_number)(value)(timestamp))
};


struct [[eosio::table, eosio::contract("bos.oracle")]] oracle_request_data_record {
  uint64_t request_id;
  std::string value;
  uint64_t timestamp;

  uint64_t primary_key() const { return request_id; }

  EOSLIB_SERIALIZE(oracle_request_data_record, (request_id)(value)(timestamp))
};


// Multi index types definition
typedef eosio::multi_index<"oracledata"_n, oracle_data_record>    oracle_data;
typedef eosio::multi_index<"oraclereq"_n, oracle_request_data_record>    oracle_request_data;
