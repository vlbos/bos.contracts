#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

using namespace eosio;
class [[eosio::contract("consumer.contract")]] consumer_contract : public eosio::contract {

 public:
   using contract::contract;

   consumer_contract(eosio::name s, eosio::name code, eosio::datastream<const char*> ds) : contract(s, code, ds) {}

   void receivejson(eosio::name self, eosio::name code);

   [[eosio::action]] void fetchdata(eosio::name oracle, uint64_t service_id, uint64_t update_number, uint64_t request_id);
   [[eosio::action]] void transfer(eosio::name from, eosio::name to, asset quantity, std::string memo);
   [[eosio::action]] void dream(eosio::name who, asset value, std::string memo);
   [[eosio::action]] void reality(asset data);
};
