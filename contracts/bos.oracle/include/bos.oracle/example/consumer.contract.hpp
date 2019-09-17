#pragma once
#include <eosio/eosio.hpp>
#include "bos.oracle/bos.types.hpp"


class [[eosio::contract("consumer.contract")]] consumer_contract : public eosio::contract
{

public:
  using contract::contract;

  consumer_contract(name s, name code, datastream<const char*> ds ) : contract(s,code,ds)
  {
  }

  void receivejson(name self, name code);

  [[eosio::action]] void fetchdata(name oracle,uint64_t service_id,uint64_t update_number,uint64_t request_id);
  

};

