/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include <string>

#include "bos.oracle/bos.constants.hpp"
#include "bos.oracle/bos.functions.hpp"

using namespace eosio;
// namespace eosio {

using eosio::asset;
using eosio::public_key;
using std::string;
/**
 * @brief 服务抵押表
 * 
 */
struct [[eosio::table, eosio::contract("bos.oracle")]] data_service_stake {
  uint64_t service_id;
  asset amount;
  asset freeze_amount;
  asset unconfirmed_amount;

  uint64_t primary_key() const { return service_id; }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] transfer_freeze_delay {
  uint64_t transfer_id;
  uint64_t service_id;
  name account;
  time_point_sec start_time;
  uint32_t duration;
  asset amount;
  uint8_t status;
  uint64_t type;

  uint64_t primary_key() const { return transfer_id; }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] risk_guarantee {
  uint64_t risk_id;
  name account;
  asset amount;
  time_point_sec start_time;
  uint32_t duration;
  uint8_t status;
  uint64_t primary_key() const { return risk_id; }
};


/**
 * @brief 服务提供者账户 冻结余额    scope 服务ID
 * 
 */
struct [[eosio::table, eosio::contract("bos.oracle")]] account_freeze_stat {
  name account;
  asset amount;

  uint64_t primary_key() const { return account.value; }
};

/**
 * @brief 服务 冻结余额    scope 服务ID
 * 
 */
struct [[eosio::table, eosio::contract("bos.oracle")]] service_freeze_stat {
  uint64_t service_id;
  asset amount;

  uint64_t primary_key() const { return service_id; }
};

/**
 * @brief 风控账户余额   scope  数据使用者账户account.value
 * 
 */
struct [[eosio::table, eosio::contract("bos.oracle")]] riskcontrol_account {

  asset balance;

  uint64_t primary_key() const { return balance.symbol.code().raw(); }
};

typedef eosio::multi_index<"riskaccounts"_n, riskcontrol_account> riskcontrol_accounts;

typedef eosio::multi_index<"servicestake"_n, data_service_stake> data_service_stakes;
typedef eosio::multi_index<"freezedelays"_n, transfer_freeze_delay> transfer_freeze_delays;
typedef eosio::multi_index<"riskguarante"_n, risk_guarantee> risk_guarantees;

typedef eosio::multi_index<"freezestats"_n, account_freeze_stat> account_freeze_stats;
typedef eosio::multi_index<"svcfrozestat"_n, service_freeze_stat> service_freeze_stats;
// };

// } /// namespace eosio
