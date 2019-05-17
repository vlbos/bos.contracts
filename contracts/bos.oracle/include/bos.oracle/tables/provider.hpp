#pragma once
#include "bos.oracle/bos.constants.hpp"
#include "bos.oracle/bos.functions.hpp"
#include "bos.oracle/bos.types.hpp"
#include <eosiolib/eosio.hpp>

using namespace eosio;
// using std::string;

struct [[eosio::table, eosio::contract("bos.oracle")]] data_service {
  uint64_t service_id;
  uint8_t fee_type;
  uint8_t data_type;
  uint8_t status;
  uint8_t injection_method;
  uint64_t acceptance;
  time_point_sec duration;
  uint64_t provider_limit;
  uint64_t update_cycle;
  time_point_sec update_start_time;
  uint64_t appeal_freeze_period;
  uint64_t exceeded_risk_control_freeze_period;
  uint64_t guarantee_id;
  asset service_price;
  asset stake_amount;
  asset risk_control_amount;
  asset pause_service_stake_amount;
  bool freeze_flag;
  bool emergency_flag;
  std::string data_format;
  std::string criteria;
  std::string declaration;

  uint64_t primary_key() const { return service_id; }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] data_service_fee {
  uint64_t id;
  uint64_t service_id;
  uint8_t fee_type;
  asset service_price;

  uint64_t primary_key() const { return id; }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] data_provider {
  name account;
  public_key pubkey;
  asset total_stake_amount;
  asset unconfirmed_amount;

  uint64_t primary_key() const { return account.value; }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] data_service_provision {
  uint64_t provision_id;
  uint64_t service_id;
  name account;
  public_key pubkey;
  asset stake_amount;
  asset service_income;
  asset claim_amount;
  std::string public_information;
  bool stop_service;

  uint64_t primary_key() const { return provision_id; }
  uint64_t by_account() const { return account.value; }
  uint64_t by_svcid() const { return service_id; }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] svc_provision_cancel_apply {
  uint64_t apply_id;
  uint64_t service_id;
  name provider;
  uint64_t status;
  time_point_sec cancel_time;
  time_point_sec finish_time;

  uint64_t primary_key() const { return apply_id; }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] data_service_provision_log {
  uint64_t log_id;
  uint64_t service_id;
  uint64_t status;
  std::string data_json;
  name account;
  name contract_account;
  name action_name;
  uint64_t request_id;
  time_point_sec update_time;

  uint64_t primary_key() const { return log_id; }
  uint64_t by_time() const { return static_cast<uint64_t>(-update_time.sec_since_epoch()); }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] push_record {
  uint64_t service_id;
  uint64_t times;

  uint64_t primary_key() const { return service_id; }

};

struct [[eosio::table, eosio::contract("bos.oracle")]] provider_push_record {
  uint64_t service_id;
  name account;
  uint64_t times;

  uint64_t primary_key() const { return account.value; }

};

struct [[eosio::table, eosio::contract("bos.oracle")]] action_push_record {
  uint64_t service_id;
  name contract_account;
  name action_name;
  uint64_t times;

  uint64_t primary_key() const { return get_hash_key(get_nn_hash(contract_account,action_name)); }

};

struct [[eosio::table, eosio::contract("bos.oracle")]] provider_action_push_record {
  uint64_t service_id;
  name account;
  name contract_account;
  name action_name;
  uint64_t times;

  uint64_t primary_key() const { return get_hash_key(get_nnn_hash(account,contract_account,action_name)); }

};

typedef eosio::multi_index<"dataservices"_n, data_service> data_services;

typedef eosio::multi_index<"servicefees"_n, data_service_fee> data_service_fees;
typedef eosio::multi_index<"providers"_n, data_provider> data_providers;
typedef eosio::multi_index<
    "svcprovision"_n, data_service_provision,
    indexed_by<"byaccount"_n,
               const_mem_fun<data_service_provision, uint64_t,
                             &data_service_provision::by_account>>, 
    indexed_by<"bysvcid"_n,
               const_mem_fun<data_service_provision, uint64_t,
                             &data_service_provision::by_svcid>>>
    data_service_provisions;

typedef eosio::multi_index<"cancelapplys"_n, svc_provision_cancel_apply>
    svc_provision_cancel_applys;

typedef eosio::multi_index<"provisionlog"_n, data_service_provision_log,
indexed_by<"bytime"_n,
               const_mem_fun<data_service_provision_log, uint64_t,
                             &data_service_provision_log::by_time>>>
    data_service_provision_logs;


typedef eosio::multi_index<"pushrecords"_n, push_record> push_records;

typedef eosio::multi_index<"ppushrecords"_n, provider_push_record> provider_push_records;

typedef eosio::multi_index<"apushrecords"_n, action_push_record> action_push_records;

typedef eosio::multi_index<"papushrecord"_n, provider_action_push_record> provider_action_push_records;



// //   ///bos.oraclize end
// };

// } // namespace bosoracle