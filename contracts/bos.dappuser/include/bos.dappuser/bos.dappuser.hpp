#pragma once
#include <eosiolib/eosio.hpp>
#include "bos.types.hpp"
#include "tables/bos.dappuser.hpp"


class [[eosio::contract("bos.dappuser")]] YOUR_CONTRACT_NAME : public eosio::contract
{
private:
  ethbtc_data ethbtc;

  name known_master;

public:
  using contract::contract;

  YOUR_CONTRACT_NAME(name s, name code, datastream<const char*> ds ) : contract(s,code,ds), ethbtc(_self, _self)
  {
    known_master = account_master(_self, _self.value).get_or_create(_self, "undefined"_n);
  }

  void receive(name self, name code);
  void receivejson(name self, name code);

  // @abi action
  [[eosio::action]]
  void setup(name oracle);

  void ask_data(name administrator,
                name registry,
                string data,
                uint32_t update_each,
                string memo,
                bytes args);
};
