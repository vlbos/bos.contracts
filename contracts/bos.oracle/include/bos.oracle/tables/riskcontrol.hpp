/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/time.hpp>
#include <string>

// namespace eosio {

using eosio::asset;
using eosio::public_key;
using std::string;

struct [[eosio::table, eosio::contract("bos.oracle")]] data_service_stake{
  uint64_t          service_id;
  asset total_stake_amount;
  asset freeze_amount;
  asset unconfirmed_amount;

  uint64_t primary_key() const { return service_id; }
};

struct [[ eosio::table, eosio::contract("bos.oracle") ]] transfer_freeze_delay
{
   uint64_t transfer_id;
   uint64_t service_id;
   name account;
   time_point_sec start_time;
   time_point_sec duration;
   asset amount;
   uint64_t status;
   uint64_t type;

   uint64_t primary_key() const { return transfer_id; }
};

// struct [[ eosio::table, eosio::contract("bos.oracle") ]] transfer_delay
// {
//    uint64_t delay_id;
//    uint64_t service_id;
//    name account;
//    time_point_sec start_time;
//    time_point_sec duration;
//    asset amount;
//    uint64_t status;

//    uint64_t primary_key() const { return delay_id; }
// };

struct [[eosio::table]] risk_guarantee
{
   uint64_t risk_id;
   name account;
   asset amount;
   time_point_sec start_time;
   time_point_sec duration;
   signature sig;
   uint64_t status;
   uint64_t primary_key() const { return risk_id; }
};

struct [[eosio::table]] riskcontrol_account {

  asset balance;
  asset unconfirmed_balance;

  uint64_t primary_key() const { return balance.symbol.code().raw(); }
};

typedef eosio::multi_index<"accounts"_n, riskcontrol_account> accounts;
// typedef eosio::multi_index<"users"_n, account_user> users;

typedef eosio::multi_index<"servicestake"_n, data_service_stake> data_service_stakes;
typedef eosio::multi_index<"freezedelays"_n, transfer_freeze_delay> transfer_freeze_delays;
// typedef eosio::multi_index<"tfdelays"_n, transfer_delay> transfer_delays;
typedef eosio::multi_index<"riskguarant"_n, risk_guarantee> risk_guarantees;
// };

// } /// namespace eosio
