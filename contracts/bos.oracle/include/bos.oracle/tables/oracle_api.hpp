#pragma once
/*

  bos_oracle



*/

#include <eosio/eosio.hpp>
#include <string>
//

using namespace eosio;

struct push_json {
   uint64_t service_id;
   name provider;
   name contract_account;
   uint64_t request_id;
   std::string data;

   EOSLIB_SERIALIZE(push_json, (service_id)(provider)(contract_account)(request_id)(data))
};


struct [[eosio::table, eosio::contract("bos.oracle")]] oracle_data_record {
   uint64_t record_id;
   uint64_t request_id;
   uint64_t cycle_number;
   std::string data;
   uint32_t timestamp;

   uint64_t primary_key() const { return record_id; }
   uint128_t by_number() const { return (uint128_t(request_id) << 64) | cycle_number; }

   EOSLIB_SERIALIZE(oracle_data_record, (record_id)(request_id)(cycle_number)(data)(timestamp))
};

typedef eosio::multi_index<"oracledata"_n, oracle_data_record, indexed_by<"bynumber"_n, const_mem_fun<oracle_data_record, uint128_t, &oracle_data_record::by_number>>> oracle_data;