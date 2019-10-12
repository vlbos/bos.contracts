#pragma once
#include "bos.oracle/bos.constants.hpp"
#include "bos.oracle/bos.functions.hpp"
#include <eosio/eosio.hpp>

using namespace eosio;
// using std::string;

struct [[eosio::table, eosio::contract("bos.oracle")]] data_service {
  uint64_t service_id;
  uint8_t data_type;
  uint8_t status;
  uint8_t injection_method;
  uint8_t acceptance;
  uint32_t duration;
  uint8_t provider_limit;
  uint32_t update_cycle;
  uint64_t last_update_number;
  uint64_t appeal_freeze_period;
  uint64_t exceeded_risk_control_freeze_period;
  uint64_t guarantee_id;
  asset base_stake_amount;
  asset risk_control_amount;
  asset pause_service_stake_amount;
  std::string data_format;
  std::string criteria;
  time_point_sec update_start_time;
  uint64_t primary_key() const { return service_id; }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] data_service_fee {
  uint64_t service_id;
  uint8_t fee_type;
  asset service_price;

  uint64_t primary_key() const { return static_cast<uint64_t>(fee_type); }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] data_provider {
  name account;
  asset total_stake_amount;
  asset total_freeze_amount;
  asset unconfirmed_amount;
  asset claim_amount;
  time_point_sec last_claim_time;
  std::vector<uint64_t> services;
  uint64_t primary_key() const { return account.value; }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] data_service_provision {
  uint64_t service_id;
  name account;
  asset amount;
  asset freeze_amount;
  asset service_income;
  uint8_t status;
  std::string public_information;
  time_point_sec create_time;

  uint64_t primary_key() const { return account.value; }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] svc_provision_cancel_apply {
  uint64_t apply_id;
  uint64_t service_id;
  name provider;
  uint8_t status;
  time_point_sec update_time;

  uint64_t primary_key() const { return apply_id; }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] data_service_provision_log {
  uint64_t log_id;
  uint64_t service_id;
  std::string data;
  name account;
  name contract_account;
  uint64_t request_id;
  time_point_sec update_time;
  uint64_t cycle_number;
  uint8_t status;

  uint64_t primary_key() const { return log_id; }
  uint64_t by_time() const {
    return static_cast<uint64_t>(-update_time.sec_since_epoch());
  }
  uint128_t by_number() const { return (uint128_t(request_id)<<64)|cycle_number; }
};

/**
 * @brief 按服务分组 数据次数  
 * 
 */
struct [[eosio::table, eosio::contract("bos.oracle")]] push_record {
  uint64_t service_id;
  uint64_t times;   ///提供数据次数
  uint64_t month_times;  ///提供数据次数 按月
  uint64_t primary_key() const { return service_id; }
};

/**
 * @brief 按数据提供者分组 数据提供次数  scope service_id
 * 
 */
struct [[eosio::table, eosio::contract("bos.oracle")]] provider_push_record {
  name account;
  uint64_t times;
  uint64_t month_times;

  uint64_t primary_key() const { return account.value; }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] oracle_id {
  uint8_t id_type;
  uint64_t id;   
  uint64_t primary_key() const { return static_cast<uint64_t>(id_type); }
};

typedef eosio::multi_index<"dataservices"_n, data_service> data_services;

typedef eosio::multi_index<"servicefees"_n, data_service_fee> data_service_fees;
typedef eosio::multi_index<"providers"_n, data_provider> data_providers;
typedef eosio::multi_index<"oracleids"_n, oracle_id> oracle_ids;
typedef eosio::multi_index<"svcprovision"_n, data_service_provision> data_service_provisions;

typedef eosio::multi_index<"cancelapplys"_n, svc_provision_cancel_apply> svc_provision_cancel_applys;

typedef eosio::multi_index<"provisionlog"_n, data_service_provision_log,
                           indexed_by<"bytime"_n, const_mem_fun<data_service_provision_log, uint64_t, &data_service_provision_log::by_time>>,
                           indexed_by<"bynumber"_n, const_mem_fun<data_service_provision_log, uint128_t, &data_service_provision_log::by_number>>>
    data_service_provision_logs;

typedef eosio::multi_index<"pushrecords"_n, push_record> push_records;

typedef eosio::multi_index<"ppushrecords"_n, provider_push_record> provider_push_records;

// //   ///bos.oraclize end
// };

// } // namespace bosoracle