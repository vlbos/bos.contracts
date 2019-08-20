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
void bos_oracle::subscribe(uint64_t service_id, name contract_account,
                           name account, asset amount, std::string memo) {

  // //   token::transfer_action transfer_act{ token_account, { account,
  // active_permission } };
  // //          transfer_act.send( account, consumer_account, amount, memo );

  //       INLINE_ACTION_SENDER(eosio::token, transfer)(
  //          token_account, { {account, active_permission} },
  //          { account, consumer_account, amount, memo }
  //       );
  require_auth(account);
  // require_auth(contract_account);

  // asset price_by_month =
  //     get_price_by_fee_type(service_id, fee_type::fee_month);
  // check(price_by_month.amount > 0 && amount >= price_by_month,
  //       "amount must greater than price by month");

  // transfer(account, consumer_account, amount, memo);

  
  // add consumer service subscription relation
  data_service_subscriptions substable(_self, service_id);


  auto subs_itr = substable.find(contract_account.value);
  check(subs_itr == substable.end(), "contract_account exist");

  substable.emplace(_self, [&](auto &subs) {
    // subs.subscription_id = id;
    subs.service_id = service_id;
    subs.account = account;
    subs.contract_account = contract_account;
    subs.payment = asset(0, core_symbol()); // amount;
    subs.consumption = asset(0, core_symbol());
    subs.month_consumption = asset(0, core_symbol());
    subs.balance = subs.payment - subs.consumption - subs.month_consumption;
    subs.subscription_time = time_point_sec(eosio::current_time_point());
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
void bos_oracle::requestdata(uint64_t service_id, name contract_account,
                             name requester,
                             std::string request_content) {
  // print("======requestdata");
  require_auth(requester);

  /// check service available subscription status subscribe
  check(service_status::service_in == get_service_status(service_id) &&
            subscription_status::subscription_subscribe ==
                get_subscription_status(service_id, contract_account),
        "service and subscription must be available");

  fee_service(service_id, contract_account,  fee_type::fee_times);

  data_service_requests reqtable(_self, service_id);

  reqtable.emplace(_self, [&](auto &r) {
    r.request_id = reqtable.available_primary_key() +
                   (0 == reqtable.available_primary_key() ? 1 : 0);
    r.service_id = service_id;
    r.contract_account = contract_account;
    r.requester = requester;
    r.request_time = time_point_sec(eosio::current_time_point());
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
void bos_oracle::payservice(uint64_t service_id, name contract_account,
                            asset amount, std::string memo) {
  require_auth(contract_account);
  check(amount.amount > 0, "amount must be greater than zero");
  transfer(contract_account, consumer_account, amount, "");
  pay_service(service_id, contract_account, amount);
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
void bos_oracle::pay_service(uint64_t service_id, name contract_account,
                             asset amount) {

  // require_auth(contract_account);

  data_service_subscriptions substable(_self, service_id);

  auto subs_itr = substable.find(contract_account.value);
  check(subs_itr != substable.end(), "contract_account does not exist");

  substable.modify(subs_itr, _self, [&](auto &subs) {
    subs.payment += amount;
    subs.balance = subs.payment - subs.consumption - subs.month_consumption;
  });

}

// } /// namespace eosio
