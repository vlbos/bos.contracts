#pragma once
#include <eosiolib/eosio.hpp>

/// provider
enum service_status : uint8_t { service_in, service_cancel, service_pause };

enum consumer_status : uint8_t { consumer_on, consumer_stop };

enum apply_status : uint8_t { apply_init, apply_cancel};

enum subscription_status : uint8_t { subscription_subscribe,subscription_unsubscribe };
enum provision_status : uint8_t { provision_reg,provision_unreg,provision_suspend };

enum transfer_status : uint8_t { transfer_start,transfer_finish,transfer_failed };

enum usage_type : uint8_t { usage_request,usage_subscribe };

enum request_status : uint8_t { reqeust_valid, request_cancel };

enum fee_type : uint8_t { fee_times, fee_month, fee_type_count };

enum data_type : uint8_t {
  data_deterministic,
  data_non_deterministic
};

enum injection_method : uint8_t {
  chain_direct,
  chain_indirect,
  chain_outside
};

enum transfer_type : uint8_t { tt_freeze , tt_delay };
enum transfer_category : uint8_t { tc_service_stake , tc_pay_service,tc_deposit,tc_arbitration_stake };
enum memo_index : uint8_t { index_category,index_service , index_count};
enum memo_index_deposit : uint8_t { deposit_category,deposit_from ,deposit_to,deposit_notify , deposit_count};
enum arbitration_timer_type: uint8_t {
  appeal_timeout,
  resp_appeal_timeout,
  resp_arbitrate_timeout,
  upload_result_timeout
};