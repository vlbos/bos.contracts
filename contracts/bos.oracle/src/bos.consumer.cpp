/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */

#include "bos.oracle/bos.oracle.hpp"
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include <eosio/transaction.hpp>
#include <string>
using namespace eosio;
// namespace eosio {

using eosio::asset;
using eosio::public_key;
using std::string;

/**
 * @brief
 *
 * @param service_id
 * @param contract_account
 * @param publickey
 * @param account
 * @param amount
 * @param memo
 */
void bos_oracle::subscribe(uint64_t service_id, name contract_account, name account, std::string memo) {
   check(memo.size() <= 256, "memo could not be greater than 256");
   require_auth(account);

   // add consumer service subscription relation
   data_service_subscriptions subscriptionstable(_self, service_id);

   auto subscriptions_itr = subscriptionstable.find(account.value);
   check(subscriptions_itr == subscriptionstable.end(), "account exist");
   subscriptionstable.emplace(_self, [&](auto& subs) {
      subs.service_id = service_id;
      subs.account = account;
      subs.contract_account = contract_account;
      subs.payment = asset(0, core_symbol());
      subs.consumption = asset(0, core_symbol());
      subs.month_consumption = asset(0, core_symbol());
      subs.balance = subs.payment - subs.consumption - subs.month_consumption;
      subs.subscription_time = bos_oracle::current_time_point_sec();
      subs.last_payment_time = time_point_sec();
      subs.status = subscription_status::subscription_subscribe;
   });
}

/**
 * @brief
 *
 * @param service_id
 * @param contract_account
 * @param requester
 * @param request_content
 */
void bos_oracle::requestdata(uint64_t service_id, name contract_account, name requester, std::string request_content) {
   check(request_content.size() <= 256, "request_content could not be greater than 256");
   require_auth(requester);

   /// check service available subscription status subscribe
   check(service_status::service_in == get_service_status(service_id) && subscription_status::subscription_subscribe == get_subscription_status(service_id, requester),
         "service and subscription must be available");

   fee_service(service_id, requester, fee_type::fee_times);

   data_service_requests reqtable(_self, service_id);
   uint64_t id = reqtable.available_primary_key();
   if (0 == id) {
      id++;
   }

   reqtable.emplace(_self, [&](auto& r) {
      r.request_id = id;
      r.service_id = service_id;
      r.contract_account = contract_account;
      r.requester = requester;
      r.request_time = bos_oracle::current_time_point_sec();
      r.request_content = request_content;
      r.status = request_status::reqeust_valid;
   });
}
/**
 * @brief
 *
 * @param service_id
 * @param contract_account
 * @param account
 * @param amount
 * @param memo
 */
void bos_oracle::pay_service(uint64_t service_id, name contract_account, asset amount) {
   data_service_subscriptions subscriptionstable(_self, service_id);

   auto subscriptions_itr = subscriptionstable.find(contract_account.value);
   check(subscriptions_itr != subscriptionstable.end(), "contract_account does not exist");

   subscriptionstable.modify(subscriptions_itr, _self, [&](auto& subs) {
      subs.payment += amount;
      subs.balance = subs.payment - subs.consumption - subs.month_consumption;
   });
}

// } /// namespace eosio
