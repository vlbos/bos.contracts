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
#include "bos.oracle/bos.types.hpp"
#include "bos.oracle/bos.constants.hpp"
#include "bos.oracle/bos.functions.hpp"

using namespace eosio;

// namespace eosio {

using eosio::asset;
using eosio::public_key;
using std::string;

struct [[ eosio::table, eosio::contract("bos.oracle") ]] data_service_subscription
{
   uint64_t subscription_id;
   uint64_t service_id;
   name contract_account;
   name action_name;
   name account;
   asset balance;
   asset payment;
   asset consumption;
   asset month_consumption;
   uint8_t payment_method;
   time_point_sec last_payment_time;
   time_point_sec subscription_time;
   uint64_t status; /// unsubscribe 0 subscribe 1
   uint64_t primary_key() const { return subscription_id; }
   uint64_t by_account()const { return account.value; }
};

struct [[ eosio::table, eosio::contract("bos.oracle") ]] data_consumer
{
   name account;
   public_key pubkey;
   uint8_t status;
   time_point_sec create_time;

   uint64_t primary_key() const { return account.value; }
};

struct [[ eosio::table, eosio::contract("bos.oracle") ]] data_service_request
{
   uint64_t request_id;
   uint64_t service_id;
   name contract_account;
   name action_name;
   name requester;
   time_point_sec request_time;
   std::string request_content;

   uint64_t primary_key() const { return request_id; }
};

struct [[ eosio::table, eosio::contract("bos.oracle") ]] data_service_usage_record
{
   uint64_t usage_id;
   uint64_t request_id;
   asset fee;
   uint8_t usage_type;/// request once  0,subscribe 1
   time_point_sec usage_time;

   uint64_t primary_key() const { return usage_id; }
};



typedef eosio::multi_index<"dataconsumer"_n, data_consumer> data_consumers;
typedef eosio::multi_index<"subscription"_n, data_service_subscription,indexed_by<"byaccount"_n, const_mem_fun<data_service_subscription, uint64_t, &data_service_subscription::by_account>> > data_service_subscriptions;

typedef eosio::multi_index<"request"_n, data_service_request> data_service_requests;

typedef eosio::multi_index<"usagerecords"_n, data_service_usage_record> data_service_usage_records;

// };

// } /// namespace eosio
