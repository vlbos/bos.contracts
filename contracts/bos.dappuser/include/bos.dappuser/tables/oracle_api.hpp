#pragma once
/*

  bos_oracle



*/

#include <eosio/eosio.hpp>
// 

using namespace eosio;



  struct [[eosio::table,eosio::contract("bos.oracle")]] oracle_data_record {
    uint64_t update_number;
    uint64_t request_id;
    string value;
    uint64_t timestamp;
    name provider;

    uint64_t primary_key() const {return timestamp;}

    EOSLIB_SERIALIZE(oracle_data_record, (update_number)(request_id)(value)(timestamp)(provider))
  };

  
  //Multi index types definition
  typedef eosio::multi_index<"oracledata"_n, oracle_data_record> oracle_data;




