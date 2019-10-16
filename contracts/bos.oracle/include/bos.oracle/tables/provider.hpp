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
   uint64_t last_cycle_number;
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
   uint64_t by_time() const { return static_cast<uint64_t>(-update_time.sec_since_epoch()); }
   uint128_t by_number() const { return (uint128_t(request_id) << 64) | cycle_number; }
};

/**
 * @brief 按服务分组 数据次数
 *
 */
struct [[eosio::table, eosio::contract("bos.oracle")]] push_record {
   uint64_t service_id;
   uint64_t times;       ///提供数据次数
   uint64_t month_times; ///提供数据次数 按月
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

typedef eosio::multi_index<"provisionlog"_n, data_service_provision_log, indexed_by<"bytime"_n, const_mem_fun<data_service_provision_log, uint64_t, &data_service_provision_log::by_time>>,
                           indexed_by<"bynumber"_n, const_mem_fun<data_service_provision_log, uint128_t, &data_service_provision_log::by_number>>>
    data_service_provision_logs;

typedef eosio::multi_index<"pushrecords"_n, push_record> push_records;

typedef eosio::multi_index<"ppushrecords"_n, provider_push_record> provider_push_records;

// //   ///bos.oraclize end
// };

// } // namespace bosoracle

struct oracle_parameters {
   std::string core_symbol = default_core_symbol;
   uint8_t precision = default_precision;
   uint64_t min_service_stake_limit = default_min_service_stake_limit;
   uint64_t min_appeal_stake_limit = default_min_appeal_stake_limit;
   uint64_t min_reg_arbitrator_stake_limit = default_min_reg_arbitrator_stake_limit;
   uint16_t arbitration_correct_rate = default_arbitration_correct_rate;
   uint8_t round_limit = default_round_limit;
   uint32_t arbi_timeout_value = default_arbi_timeout_value;                 
   uint32_t arbi_freeze_stake_duration = default_arbi_freeze_stake_duration; 
   uint32_t time_deadline = default_time_deadline;                           
   uint32_t clear_data_time_length = default_clear_data_time_length;        
   uint16_t max_data_size = default_max_data_size;
   uint16_t min_provider_limit = default_min_provider_limit;
   uint16_t max_provider_limit = default_max_provider_limit;
   uint32_t min_update_cycle = default_min_update_cycle;
   uint32_t max_update_cycle = default_max_update_cycle;
   uint32_t min_duration = default_min_duration;
   uint32_t max_duration = default_max_duration;
   uint16_t min_acceptance = default_min_acceptance;
   uint16_t max_acceptance = default_max_acceptance;

   // explicit serialization macro is not necessary, used here only to improve compilation time
   EOSLIB_SERIALIZE(oracle_parameters, (core_symbol)(precision)(min_service_stake_limit)(min_appeal_stake_limit)(min_reg_arbitrator_stake_limit)(arbitration_correct_rate)(round_limit)(
                                           arbi_timeout_value)(arbi_freeze_stake_duration)(time_deadline)(clear_data_time_length)(max_data_size)(min_provider_limit)(max_provider_limit)(
                                           min_update_cycle)(max_update_cycle)(min_duration)(max_duration)(min_acceptance)(max_acceptance))
};

struct [[eosio::table("metaparams"), eosio::contract("bos.oracle")]] oracle_meta_parameters {
   oracle_meta_parameters() {parameters_data = pack(oracle_parameters{}); }
   uint8_t version;
   std::vector<char> parameters_data;

   EOSLIB_SERIALIZE(oracle_meta_parameters, (version)(parameters_data))
};

typedef eosio::singleton<"metaparams"_n, oracle_meta_parameters> oracle_meta_parameters_singleton;
