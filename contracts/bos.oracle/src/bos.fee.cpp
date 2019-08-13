#pragma once

#include "bos.oracle/bos.oracle.hpp"
#include <cmath>
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
using namespace eosio;
using std::string;

/**
 * @brief
 *
 * @param service_id
 * @return uint8_t
 */
uint8_t bos_oracle::get_service_status(uint64_t service_id) {
  data_services svctable(_self, _self.value);
  auto service_itr = svctable.find(service_id);
  check(service_itr != svctable.end(), "service does not exist");

  return service_itr->status;
}

/**
 * @brief
 *
 * @param service_id
 * @param provider
 * @return uint64_t
 */
uint64_t bos_oracle::get_request_by_last_push(uint64_t service_id,
                                              name provider) {

  data_service_provision_logs logtable(_self, service_id);
  auto update_time_idx = logtable.get_index<"bytime"_n>();

  uint64_t request_id = 0;
  for (const auto &u : update_time_idx) {
    if (provider == u.account && 0 != u.request_id) {
      request_id = u.request_id;
      break;
    }
  }

  return request_id;
}


/**
 * @brief
 *
 * @param service_id
 * @param account
 * @param contract_account
 * @param action_name
 * @param is_request
 */
void bos_oracle::add_times(uint64_t service_id, name account,
                           name contract_account, name action_name,
                           bool is_request) {

  const uint64_t one_time = 1;
  auto add_time = [is_request](uint64_t &times, uint64_t &month_times,
                               bool is_new = false) {
    if (is_new) {
      times = 0;
      month_times = 0;
    }

    if (is_request) {
      times += one_time;
    } else {
      month_times += one_time;
    }
  };

  push_records pushtable(_self, service_id);
  auto push_itr = pushtable.find(service_id);
  if (push_itr == pushtable.end()) {
    pushtable.emplace(_self, [&](auto &p) {
      p.service_id = service_id;
      add_time(p.times, p.month_times, true);
      // print(p.times);
      // print("==new==times");
      // print(p.month_times);
      // print("====month times");
    });
  } else {
    pushtable.modify(push_itr, same_payer, [&](auto &p) {
      add_time(p.times, p.month_times);
      // print(p.times);
      // print("====times=====");
      // print(p.month_times);
      // print("====month times=====");
    });
  }

  provider_push_records providetable(_self, service_id);
  auto provide_itr = providetable.find(account.value);
  if (provide_itr == providetable.end()) {
    providetable.emplace(_self, [&](auto &p) {
      p.service_id = service_id;
      p.account = account;
      add_time(p.times, p.month_times, true);
    });
  } else {
    providetable.modify(provide_itr, same_payer,
                        [&](auto &p) { add_time(p.times, p.month_times); });
  }

  action_push_records actiontable(_self, service_id);
  uint64_t a_id = get_hash_key(get_nn_hash(contract_account, action_name));
  auto action_itr = actiontable.find(a_id);
  if (action_itr == actiontable.end()) {
    actiontable.emplace(_self, [&](auto &a) {
      a.service_id = service_id;
      a.contract_account = contract_account;
      a.action_name = action_name;
      add_time(a.times, a.month_times, true);
    });
  } else {
    actiontable.modify(action_itr, same_payer,
                       [&](auto &a) { add_time(a.times, a.month_times); });
  }

  provider_action_push_records provideractiontable(_self, service_id);
  uint64_t pa_id =
      get_hash_key(get_nnn_hash(account, contract_account, action_name));
  auto provider_action_itr = provideractiontable.find(pa_id);
  if (provider_action_itr == provideractiontable.end()) {
    provideractiontable.emplace(_self, [&](auto &pa) {
      pa.service_id = service_id;
      pa.account = account;
      pa.contract_account = contract_account;
      pa.action_name = action_name;
      add_time(pa.times, pa.month_times, true);
    });
  } else {
    provideractiontable.modify(provider_action_itr, same_payer, [&](auto &pa) {
      add_time(pa.times, pa.month_times);
    });
  }
}

std::tuple<uint64_t, uint64_t, uint64_t, uint64_t>
bos_oracle::get_times(uint64_t service_id, name account) {

  push_records pushtable(_self, service_id);
  provider_push_records providetable(_self, service_id);

  uint64_t service_times = 0;
  uint64_t provide_times = 0;
  uint64_t service_month_times = 0;
  uint64_t provide_month_times = 0;
  auto push_itr = pushtable.find(service_id);
  if (push_itr != pushtable.end()) {
    service_times = push_itr->times;
    service_month_times = push_itr->month_times;
  }

  auto provide_itr = providetable.find(account.value);
  if (provide_itr != providetable.end()) {
    provide_times = provide_itr->times;
    provide_month_times = provide_itr->month_times;
  }

  return std::make_tuple(service_times, service_month_times, provide_times,
                         provide_month_times);
}

/**
 * @brief
 *
 * @param service_id
 * @param fee_type
 * @return asset
 */
asset bos_oracle::get_price_by_fee_type(uint64_t service_id, uint8_t fee_type) {
  //  check(fee_types.size() > 0 && fee_types.size() ==
  //  service_prices.size(),"fee_types size have to equal service prices size");
  data_service_fees feetable(_self, service_id);
  // for(int i = 0;i < fee_types.size();i++)
  // {
  // auto type = fee_type; // s[i];
  // auto price = service_prices[i];
  check(fee_type >= fee_type::fee_times && fee_type < fee_type::fee_type_count,
        "unknown fee type");

  auto fee_itr = feetable.find(fee_type);
  check(fee_itr != feetable.end(), " service's fee type does not found");

  return fee_itr->service_price;
}

/**
 * @brief
 *
 * @param service_id
 * @param contract_account
 * @param action_name
 * @param fee_type
 */
void bos_oracle::fee_service(uint64_t service_id, name contract_account,
                             name action_name, uint8_t fee_type) {
  // static constexpr uint32_t month_seconds = 30 * 24 * 60 * 60;
  // //   token::transfer_action transfer_act{ token_account, { account,
  // active_permission } };
  // //          transfer_act.send( account, consumer_account, amount, memo );

  //       INLINE_ACTION_SENDER(eosio::token, transfer)(
  //          token_account, { {account, active_permission} },
  //          { account, consumer_account, amount, memo }
  //       );
  // require_auth(account);
  // require_auth(contract_account);
  // transfer(account, consumer_account, amount, memo);
  asset price_by_times = get_price_by_fee_type(service_id, fee_type);

  check(price_by_times.amount > 0,
        " get price by times cound not be greater than zero");

  data_service_subscriptions substable(_self, service_id);
  // auto id =
  //     get_hash_key(get_uuu_hash(service_id, contract_account, action_name));
  auto subs_itr = substable.find(contract_account.value);
  check(subs_itr != substable.end(), "contract_account does not exist");

  check(subs_itr->balance >= price_by_times,
        "balance cound not be  greater than price by times");

  substable.modify(subs_itr, _self, [&](auto &subs) {
    if (fee_type::fee_times == fee_type) {
      subs.consumption += price_by_times;
    } else {
      subs.month_consumption += price_by_times;
      subs.last_payment_time = time_point_sec(eosio::current_time_point());
    }
    subs.balance = subs.payment - subs.consumption - subs.month_consumption;
  });

  service_consumptions consumptionstable(_self, service_id);
  auto consumptions_itr = consumptionstable.find(service_id);
  if (consumptions_itr == consumptionstable.end()) {
    consumptionstable.emplace(_self, [&](auto &c) {
      c.service_id = service_id;
      if (fee_type::fee_times == fee_type) {
        c.consumption = price_by_times;
        c.month_consumption = asset(0, core_symbol());
      } else {
        c.consumption = asset(0, core_symbol());
        c.month_consumption = price_by_times;
      }
      c.update_time = time_point_sec(eosio::current_time_point());
    });
  } else {
    consumptionstable.modify(consumptions_itr, same_payer, [&](auto &c) {
      if (fee_type::fee_times == fee_type) {
        print("%%%%%%%%%%consumption");
        c.consumption.print();
        price_by_times.print();
        print("%%%%%%%00000%%%consumption");

        c.consumption += price_by_times;
      } else {
        c.month_consumption += price_by_times;
      }
      c.update_time = time_point_sec(eosio::current_time_point());
    });
  }
}

/**
 * @brief
 *
 * @param service_id
 * @param contract_account
 * @param action_name
 * @return uint8_t
 */
uint8_t bos_oracle::get_subscription_status(uint64_t service_id,
                                            name contract_account,
                                            name action_name) {

  data_service_subscriptions substable(_self, service_id);
  // auto id =
  //     get_hash_key(get_uuu_hash(service_id, contract_account, action_name));
  auto subs_itr = substable.find(contract_account.value);
  check(subs_itr != substable.end(), "contract_account does not exist");

  return subs_itr->status;
}

/**
 * @brief
 *
 * @param service_id
 * @param contract_account
 * @param action_name
 * @return time_point_sec
 */
time_point_sec bos_oracle::get_payment_time(uint64_t service_id,
                                            name contract_account,
                                            name action_name) {

  data_service_subscriptions substable(_self, service_id);
  // auto id =
  //     get_hash_key(get_uuu_hash(service_id, contract_account, action_name));
  auto subs_itr = substable.find(contract_account.value);
  check(subs_itr != substable.end(), "contract_account does not exist");

  return subs_itr->last_payment_time;
}

/**
 * @brief
 *
 * @param service_id
 * @return std::vector<std::tuple<name, name>>
 */
std::vector<std::tuple<name, name>>
bos_oracle::get_subscription_list(uint64_t service_id) {

  data_service_subscriptions substable(_self, service_id);
  auto subscription_time_idx = substable.get_index<"bytime"_n>();
  std::vector<std::tuple<name, name>> receive_contracts;

  for (const auto &s : subscription_time_idx) {
    if (s.status == subscription_status::subscription_subscribe) {
      receive_contracts.push_back(
          std::make_tuple(s.contract_account, s.action_name));
    }
  }

  return receive_contracts;
}

/**
 * @brief
 *
 * @param service_id
 * @param request_id
 * @return std::vector<std::tuple<name, name, uint64_t>>
 */
std::vector<std::tuple<name, name, uint64_t>>
bos_oracle::get_request_list(uint64_t service_id, uint64_t request_id) {
  // print("=-=============get_request_list in===========");
  static constexpr int64_t request_time_deadline = 2; // 2 hours
  std::vector<std::tuple<name, name, uint64_t>> receive_contracts;
  std::vector<uint64_t> ids;
  data_service_requests reqtable(_self, service_id);
  auto request_time_idx = reqtable.get_index<"bytime"_n>();
  auto lower = request_time_idx.begin();
  auto upper = request_time_idx.end();
  // if (0 != request_id) {
  //   auto req_itr = reqtable.find(request_id);
  //   check(req_itr != reqtable.end(), "request id could not be found");

  //   lower = request_time_idx.lower_bound(
  //       static_cast<uint64_t>(req_itr->request_time.sec_since_epoch()));
  //   upper = request_time_idx.upper_bound(
  //       static_cast<uint64_t>(req_itr->request_time.sec_since_epoch()));
  // }

  // print("=-=============get_request_list while before===========");
  while (lower != upper) {
    // print("=-=============get_request_list while (lower !=
    // upper)===========");
    auto req = lower++;
    if (req->status == request_status::reqeust_valid &&
        time_point_sec(eosio::current_time_point()) - req->request_time >
            eosio::hours(request_time_deadline)) {
      // print("=-=============get_request_list while  if===========");
      receive_contracts.push_back(std::make_tuple(
          req->contract_account, req->action_name, req->request_id));

          ids.push_back(req->request_id);
    }
  }

  for(auto& id:ids)
  {
      data_service_requests reqtable(_self, service_id);
    auto req_itr = reqtable.find(request_id);
    check(req_itr != reqtable.end(), "request id could not be found");
       reqtable.modify(req_itr, _self, [&](auto &r) {
      r.status = request_status::reqeust_finish;

    });
  }

  return receive_contracts;
}

/**
 * @brief
 *
 * @param service_id
 * @return std::tuple<uint64_t, uint64_t>
 */
std::tuple<uint64_t, uint64_t>
bos_oracle::get_consumption(uint64_t service_id) {

  static constexpr uint64_t update_time_deadline = 1; // 24 hours
  data_service_subscriptions substable(_self, service_id);

  service_consumptions consumptionstable(_self, service_id);
  auto service_itr = consumptionstable.find(service_id);
  check(service_itr != consumptionstable.end(), "service could not be found");
  uint64_t consumptions = service_itr->consumption.amount;
  uint64_t month_consumptions = service_itr->month_consumption.amount;

  if (service_itr->update_time - time_point_sec(eosio::current_time_point()) >
      eosio::days(update_time_deadline)) {
    consumptions = 0;
    month_consumptions = 0;
    for (const auto &s : substable) {
      consumptions += s.consumption.amount;
      month_consumptions += s.month_consumption.amount;
    }

    consumptionstable.modify(service_itr, _self, [&](auto &consump) {
      consump.consumption = asset(consumptions, core_symbol());
      consump.month_consumption = asset(month_consumptions, core_symbol());
      consump.update_time = time_point_sec(eosio::current_time_point());
    });
  }

  return std::make_tuple(consumptions, month_consumptions);
}

std::vector<std::tuple<name, asset>>
bos_oracle::get_provider_list(uint64_t service_id) {

  data_service_provisions provisionstable(_self, service_id);

  std::vector<std::tuple<name, asset>> providers;

  for (const auto &p : provisionstable) {

    // print("p.amount.amount");
    // print(p.amount.amount);
    // print("p.freeze_amount.amount");
    // print(p.freeze_amount.amount);
    if (p.status == provision_status::provision_reg &&
        p.amount.amount - p.freeze_amount.amount > 0) {
      providers.push_back(
          std::make_tuple(p.account, p.amount - p.freeze_amount));
    }
  }

  return providers;
}

// std::tuple<uint64_t, uint64_t, uint64_t, uint64_t>
// bos_oracle::stat_freeze_amounts(uint64_t service_id, name account) {
// }

std::tuple<asset, asset> bos_oracle::stat_freeze_amounts(uint64_t service_id,
                                                         name account) {

  account_freeze_logs freezelogtable(_self, service_id);

  //  std::map<name,uint64_t> provider2pushtimes;
  //   auto account_idx = account_freeze_logs.get_index<"byaccount"_n>();
  // asset total_amount(0,core_symbole());
  // auto lower =action_idx.lower_bound(account.value);
  // auto upper =action_idx.upper_bound(account.value);
  asset total_amount(0, core_symbol());
  asset account_total_amount(0, core_symbol());
  for (const auto &f : freezelogtable) {

    if (f.account == account) {
      account_total_amount += f.amount;
    }
    total_amount += f.amount;
  }

  return std::make_tuple(total_amount, account_total_amount);
}

// } // namespace bosoracle