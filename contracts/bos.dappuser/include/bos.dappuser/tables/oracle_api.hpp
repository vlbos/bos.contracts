#pragma once
/*

  bos_oracle

  Author: Guillaume "Gnome" Babin-Tremblay - EOS Titan
  
  Website: https://eostitan.com
  Email: guillaume@eostitan.com

  Github: https://github.com/eostitan/bos_oracle/
  
  Published under MIT License

*/

#include <eosiolib/eosio.hpp>
#include <eosiolib/fixedpoint.hpp>
#include <eosiolib/chain.h>


using namespace eosio;



  struct [[eosio::table,eosio::contract("bos.oracle")]] oracle_data_record {
    uint64_t update_number;
    string value;
    uint64_t timestamp;

    uint64_t primary_key() const {return update_number;}

    EOSLIB_SERIALIZE( oracle_data_record, (update_number)(value)(timestamp))

  };

  //Multi index types definition
  typedef eosio::multi_index<"oracledata"_n, oracle_data_record> oracle_data;




