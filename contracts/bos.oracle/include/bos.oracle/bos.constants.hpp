#pragma once
#include <eosio/eosio.hpp>

/// provider
enum service_status : uint8_t {
  service_in,
  service_cancel,
  service_pause,
  service_freeze,
  service_emergency
};

enum consumer_status : uint8_t { consumer_on, consumer_stop };

enum apply_status : uint8_t { apply_init, apply_cancel };

enum subscription_status : uint8_t {
  subscription_subscribe,
  subscription_unsubscribe
};
enum provision_status : uint8_t {
  provision_reg,
  provision_unreg,
  provision_suspend
};

enum transfer_status : uint8_t {
  transfer_start,
  transfer_finish,
  transfer_failed
};

enum usage_type : uint8_t { usage_request, usage_subscribe };

enum request_status : uint8_t { reqeust_valid, request_cancel, reqeust_finish };

enum fee_type : uint8_t { fee_times, fee_month, fee_type_count };

enum data_type : uint8_t { data_deterministic, data_non_deterministic };

enum injection_method : uint8_t { chain_indirect, chain_direct, chain_outside };

enum transfer_type : uint8_t { tt_freeze, tt_delay };
enum transfer_category : uint8_t {
  tc_service_stake,
  tc_pay_service,
  tc_deposit,
  tc_arbitration_stake_appeal,
  tc_arbitration_stake_arbitrator,
  tc_arbitration_stake_resp_case,
  ts_arbitration_stake_reappeal,
  tc_arbitration_stake_reresp_case,
  tc_risk_guarantee
};

const uint8_t index_category = 0;
const uint8_t index_from = 1;
const uint8_t index_to = 2;
const uint8_t index_notify = 3;
const uint8_t deposit_count = 4;

const uint8_t index_id = 1;
const uint8_t index_evidence = 2;
const uint8_t index_info = 3;
const uint8_t index_provider = 4;
const uint8_t index_reason = 5;
const uint8_t appeal_count = 6;

const uint8_t index_type = 1;
const uint8_t arbitrator_count = 2;

const uint8_t resp_case_count = 3;

const uint8_t index_duration = 2;
const uint8_t risk_guarantee_case_count = 4;

// index_category,index_id 
// deposit_category,deposit_from,deposit_to,deposit_notify 
// appeal_category,index_id ,index_evidence,index_info,index_reason,index_provider
// arbitrator_category,index_type 
// resp_case_category,index_id ,index_evidence
// risk_guarantee_category,index_id,index_duration
enum arbitration_timer_type : uint8_t {
  appeal_timeout,
  reappeal_timeout,
  resp_appeal_timeout,
  resp_arbitrate_timeout,
  upload_result_timeout,
  resp_reappeal_timeout,

  public_appeal_timeout,
  public_reappeal_timeout,
  public_resp_appeal_timeout,
  public_resp_arbitrate_timeout,
  public_upload_result_timeout,
  public_resp_reappeal_timeout
};



enum arbitrator_status : uint8_t { stoped = 0, serving = 1 };

enum arbitrator_type : uint8_t { profession = 1, amateur = 2 };


enum arbi_method_type : uint8_t { multiple_rounds = 1, public_arbitration = 2 };

enum arbi_step_type : uint64_t {
  arbi_init = 1,
  arbi_responded,
  arbi_choosing_arbitrator,
  arbi_started,
  arbi_end,
  arbi_appeal_timeout_end,
  arbi_reappeal,
  arbi_reappeal_responded,
  arbi_reappeal_timeout_end,
  arbi_public_init,
  arbi_public_responded,
  arbi_public_choosing_arbitrator,
  arbi_public_started,
  arbi_public_end,
  arbi_public_timeout
};

enum final_result_type : uint64_t {
  consumer,
  provider

};