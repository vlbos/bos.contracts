#pragma once
#include <eosiolib/eosio.hpp>

/// provider
enum data_service_status : uint8_t { service_in, service_cancel, service_pause };

enum data_consumer_status : uint8_t { consumer_on, consumer_stop };

enum data_service_subscription_status : uint8_t { service_unsubscribe,service_subscribe };

enum data_service_usage_type : uint8_t { usage_request,usage_subscribe };

enum data_request_status : uint8_t { reqeust_valid, request_cancel };

enum data_service_fee_type : uint8_t { fee_times, fee_month, fee_type_count };

enum data_service_data_type : uint8_t {
  data_deterministic,
  data_non_deterministic
};

enum data_service_injection_method : uint8_t {
  chain_direct,
  chain_indirect,
  chain_outside
};



enum transfer_type : uint8_t { transfer_freeze , transfer_delay };