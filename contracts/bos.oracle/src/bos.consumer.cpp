/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */

#include "bos.oracle/bos.oracle.hpp"
#include <eosiolib/asset.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/transaction.hpp>
#include <string>
using namespace eosio;
// namespace eosio {

using eosio::asset;
using eosio::public_key;
using std::string;

void bos_oracle::subscribe(uint64_t service_id, name contract_account,
                           name action_name, std::string publickey,
                           name account, asset amount, std::string memo) {

  // //   token::transfer_action transfer_act{ token_account, { account,
  // active_permission } };
  // //          transfer_act.send( account, consumer_account, amount, memo );

  //       INLINE_ACTION_SENDER(eosio::token, transfer)(
  //          token_account, { {account, active_permission} },
  //          { account, consumer_account, amount, memo }
  //       );
  require_auth(account);
  require_auth(contract_account);

  asset price_by_month =
      get_price_by_fee_type(service_id, data_service_fee_type::fee_month);
  check(price_by_month.amount > 0 && amount >= price_by_month,
        "amount must greater than price by month");

  transfer(account, consumer_account, amount, memo);

  // add consumer
  data_consumers consumertable(_self, _self.value);
  auto consumer_itr = consumertable.find(account.value);
  if (consumer_itr == consumertable.end()) {
    consumertable.emplace(_self, [&](auto &c) {
      c.account = account;
      // c.pubkey = publickey;
      c.status = data_consumer_status::consumer_on;
      c.create_time = time_point_sec(now());
    });
  }

  // add consumer service subscription relation
  data_service_subscriptions substable(_self, _self.value);

  auto id =
      get_hash_key(get_uuu_hash(service_id, contract_account, action_name));
  auto subs_itr = substable.find(id);
  check(subs_itr == substable.end(), "contract_account exist");

  substable.emplace(_self, [&](auto &subs) {
    subs.subscription_id = id;
    subs.service_id = service_id;
    subs.contract_account = contract_account;
    subs.action_name = action_name;
    subs.payment = amount;
    subs.consumption = asset(0, core_symbol());
    subs.subscription_time = time_point_sec(now());
  });
}

void bos_oracle::payservice(uint64_t service_id, name contract_account,
                            name action_name, name account, asset amount,
                            std::string memo) {

  require_auth(account);
  require_auth(contract_account);
  check(amount.amount > 0, "amount must be greater than zero");
  transfer(account, consumer_account, amount, memo);

  data_service_subscriptions substable(_self, _self.value);

  auto id =
      get_hash_key(get_uuu_hash(service_id, contract_account, action_name));
  auto subs_itr = substable.find(id);
  check(subs_itr != substable.end(), "contract_account does not exist");

  substable.modify(subs_itr, _self,
                   [&](auto &subs) { subs.payment += amount; });

  transaction t;
  t.actions.emplace_back(
      permission_level{_self, active_permission}, _self, "confirmpay"_n,
      std::make_tuple(service_id, contract_account, action_name, amount));
  t.delay_sec = 120;//seconds
  uint128_t deferred_id =
      (uint128_t(contract_account.value) << 64) | action_name.value;
  cancel_deferred(deferred_id);
  t.send(deferred_id, _self);
}

void bos_oracle::confirmpay(uint64_t service_id, name contract_account,
                            name action_name, asset amount) {
  check(amount.amount > 0, "amount must be greater than zero");
  data_service_subscriptions substable(_self, _self.value);

  auto id =
      get_hash_key(get_uuu_hash(service_id, contract_account, action_name));
  auto subs_itr = substable.find(id);
  check(subs_itr != substable.end(), "contract_account does not exist");
  check(subs_itr->payment > amount, "payment must be greater than amount");
  substable.modify(subs_itr, _self, [&](auto &subs) {
    subs.payment -= amount;
    subs.balance += amount;
  });
}

void bos_oracle::fee_service(uint64_t service_id, name contract_account,
                             name action_name, uint8_t fee_type) {
  static constexpr uint32_t month_seconds = 30 * 24 * 60 * 60;
  // //   token::transfer_action transfer_act{ token_account, { account,
  // active_permission } };
  // //          transfer_act.send( account, consumer_account, amount, memo );

  //       INLINE_ACTION_SENDER(eosio::token, transfer)(
  //          token_account, { {account, active_permission} },
  //          { account, consumer_account, amount, memo }
  //       );
  // require_auth(account);
  require_auth(contract_account);
  // transfer(account, consumer_account, amount, memo);
  asset price_by_times = get_price_by_fee_type(service_id, fee_type);

  data_service_subscriptions substable(_self, _self.value);
  auto id =
      get_hash_key(get_uuu_hash(service_id, contract_account, action_name));
  auto subs_itr = substable.find(id);
  check(subs_itr != substable.end(), "contract_account does not exist");

  check(price_by_times.amount > 0 and subs_itr->balance >= price_by_times,
        "balance must greater than price by times");

  substable.modify(subs_itr, _self, [&](auto &subs) {
    subs.balance -= price_by_times;
    if (data_service_fee_type::fee_times == fee_type) {
      subs.consumption += price_by_times;
    } else {

      subs.month_consumption += price_by_times;
      subs.last_payment_time += month_seconds;
    }
  });
}

uint8_t bos_oracle::get_subscription_status(uint64_t service_id,
                                            name contract_account,
                                            name action_name) {

  data_service_subscriptions substable(_self, _self.value);
  auto id =
      get_hash_key(get_uuu_hash(service_id, contract_account, action_name));
  auto subs_itr = substable.find(id);
  check(subs_itr != substable.end(), "contract_account does not exist");

  return subs_itr->status;
}

time_point_sec bos_oracle::get_payment_time(uint64_t service_id,
                                            name contract_account,
                                            name action_name) {

  data_service_subscriptions substable(_self, _self.value);
  auto id =
      get_hash_key(get_uuu_hash(service_id, contract_account, action_name));
  auto subs_itr = substable.find(id);
  check(subs_itr != substable.end(), "contract_account does not exist");

  return subs_itr->last_payment_time;
}

std::vector<std::tuple<name,name>> bos_oracle::get_subscription_list(uint64_t service_id) {

  static constexpr uint64_t subscribe_time_deadline = 2*60*60;//2 hours
  data_service_subscriptions substable(_self, service_id);
 auto subscription_time_idx = substable.get_index<"bytime"_n>();
std::vector<std::tuple<name,name>> receive_contracts;

for(const auto& s :subscription_time_idx)
{
   if (s.status == data_service_subscription_status::service_subscribe ) {
        receive_contracts.push_back(
            std::make_tuple(s.contract_account, s.action_name));
      }
}

  return receive_contracts;
}


std::vector<std::tuple<name,name,uint64_t>> bos_oracle::get_request_list(uint64_t service_id,
                                            uint64_t request_id) {

  static constexpr int64_t request_time_deadline_us = 2 * 3600 * int64_t(1000000); // 2 hours
  std::vector<std::tuple<name, name, uint64_t>> receive_contracts;
  data_service_requests reqtable(_self, service_id);
  auto request_time_idx = reqtable.get_index<"bytime"_n>();
  auto lower = request_time_idx.begin();
  auto upper = request_time_idx.end();
  if (0 != request_id) {
    auto req_itr = reqtable.find(request_id);
    check(req_itr != reqtable.end(), "request id could not be found");

    lower = request_time_idx.lower_bound(static_cast<uint64_t>(req_itr->request_time.sec_since_epoch()));
  }

  while (lower != upper) {
    auto req = lower++;
    if (req->status == data_request_status::reqeust_valid &&
        time_point_sec(now()) - req->request_time < microseconds(request_time_deadline_us)) {
      receive_contracts.push_back(std::make_tuple(
          req->contract_account, req->action_name, req->request_id));
    }
  }

  return receive_contracts;
}


void bos_oracle::requestdata(uint64_t service_id, name contract_account,
                             name action_name, name requester,
                             std::string request_content) {
  require_auth(requester);

  /// check service available subsrciption status subscribe
  check(data_service_status::service_in == get_service_status(service_id) &&
            data_service_subscription_status::service_subscribe ==
                get_subscription_status(service_id, contract_account,
                                        action_name),
        "service and subscription must be available");

  fee_service(service_id, contract_account, action_name,
              data_service_fee_type::fee_times);

  data_service_requests reqtable(_self, _self.value);

  reqtable.emplace(_self, [&](auto &r) {
    r.request_id = reqtable.available_primary_key();
    r.service_id = service_id;
    r.contract_account = contract_account;
    r.action_name = action_name;
    r.requester = requester;
    r.request_time = time_point_sec(now());
    r.request_content = request_content;
  });
}

// } /// namespace eosio
