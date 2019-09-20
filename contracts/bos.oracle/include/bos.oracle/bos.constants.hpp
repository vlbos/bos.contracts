#pragma once
#include <eosio/eosio.hpp>

/// provider
enum service_status : uint8_t { service_init, service_in, service_cancel, service_pause, service_pause_insufficient_providers, service_freeze, service_emergency };

enum apply_status : uint8_t { apply_init, apply_cancel };

enum subscription_status : uint8_t { subscription_subscribe, subscription_unsubscribe };

enum provision_status : uint8_t { provision_reg, provision_unreg, provision_suspend, provision_freeze_suspend };

enum transfer_status : uint8_t { transfer_start, transfer_finish, transfer_failed };

enum request_status : uint8_t { reqeust_valid, request_cancel, reqeust_finish };

enum log_status : uint8_t { log_init, log_fail, log_sent, log_paid, log_finish };

enum fee_type : uint8_t { fee_times, fee_month, fee_type_count };

enum data_type : uint8_t { data_deterministic, data_non_deterministic };

enum injection_method : uint8_t { chain_indirect, chain_direct, chain_outside };

enum transfer_type : uint8_t { tt_freeze, tt_delay };
enum transfer_category : uint8_t { tc_service_stake, tc_pay_service, tc_deposit, tc_arbitration_stake_appeal, tc_arbitration_stake_arbitrator, tc_arbitration_stake_resp_case, tc_risk_guarantee };

enum deposit_index : uint8_t { index_category, index_from, index_to, index_notify, deposit_count };

enum appeal_index : uint8_t { index_id = 1, index_evidence, index_info, index_reason, index_provider, appeal_count };

enum arbitrator_index : uint8_t { index_type = 1, arbitrator_count };

const uint8_t resp_case_count = 3;

enum risk_guarantee_index : uint8_t { index_duration = 2, risk_guarantee_case_count };

// index_category,index_id
// deposit_category,deposit_from,deposit_to,deposit_notify
// appeal_category,index_id ,index_evidence,index_info,index_reason,index_provider
// arbitrator_category,index_type
// resp_case_category,index_id ,index_evidence
// risk_guarantee_category,index_id,index_duration
enum arbitration_timer_type : uint8_t { reappeal_timeout = 1, resp_appeal_timeout, accept_arbitrate_invitation_timeout, upload_result_timeout };

enum arbitrator_type : uint8_t { fulltime = 1, crowd = 2, wps = 5 };
enum arbi_method_type : uint8_t { multiple_rounds = 1, public_arbitration = 2 };
enum arbi_step_type : uint8_t {
   arbi_init = 1,
   arbi_choosing_arbitrator,
   arbi_wait_for_resp_appeal,
   arbi_wait_for_accept_arbitrate_invitation,
   arbi_wait_for_upload_result,
   arbi_wait_for_reappeal,
   arbi_resp_appeal_timeout_end,
   arbi_reappeal_timeout_end,
   arbi_public_end
};

enum arbitration_role_type : uint8_t { pending = 0, consumer = 1, provider = 2, sponsor = 4, respodent = 8 };

static int default_arbitration_correct_rate = 6000;
static int arbitration_correct_rate_base = 100;
static int percent_100 = 10000;

static const uint64_t arbi_process_time_limit = 3600;
static const uint8_t round_limit = 3;
static const uint8_t arbi_resp_appeal_timeout_value = 1;    // hours
static const uint8_t arbi_upload_result_timeout_value = 1;  // hours
static const uint8_t arbi_reappeal_timeout_value = 1;       /// hours
static const uint8_t arbi_resp_arbitrate_timeout_value = 1; // hours
static const uint8_t arbi_freeze_stake_duration = 1;        // days
static const uint8_t arbi_case_deadline = 1;                // hours
static const int8_t request_time_deadline = 1;          // 1 hours
static const uint8_t update_time_deadline = 1;          // 1 hours
static const uint32_t service_stake_limit = 1000;
static const uint64_t one_time = 1;
static const uint8_t first_round = 1;
static const uint8_t first_respondent = 1;

