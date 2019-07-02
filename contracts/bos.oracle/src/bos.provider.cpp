#pragma once

#include "bos.oracle/bos.oracle.hpp"
#include <cmath>
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>
using namespace eosio;
using std::string;

/**
 * @brief
 *
 * @param service_id
 * @param account
 * @param stake_amount
 * @param service_price
 * @param fee_type
 * @param data_format
 * @param data_type
 * @param criteria
 * @param acceptance
 * @param declaration
 * @param injection_method
 * @param duration
 * @param provider_limit
 * @param update_cycle
 * @param update_start_time
 */
void bos_oracle::regservice(uint64_t service_id, name account,
                            asset stake_amount, asset service_price,
                            uint64_t fee_type, std::string data_format,
                            uint64_t data_type, std::string criteria,
                            uint64_t acceptance, std::string declaration,
                            uint64_t injection_method, uint64_t duration,
                            uint64_t provider_limit, uint64_t update_cycle,
                            time_point_sec update_start_time) {
                              print("=====1");
  require_auth(account);
  uint64_t new_service_id = service_id;
  data_services svctable(_self, _self.value);
 auto service_itr = svctable.find(service_id);
  if (service_itr == svctable.end()) { 
    //  if (0 == service_id) {
     print("=====2");
    // add service
    svctable.emplace(_self, [&](auto &s) {
      s.service_id = svctable.available_primary_key();
      print("$$$$$$$$$$$$$");
      print(s.service_id);
      print("**************");
      s.service_price = service_price;
      s.fee_type = fee_type;
      s.data_format = data_format;
      s.data_type = data_type;
      s.criteria = criteria;
      s.acceptance = acceptance;
      s.declaration = declaration;
      s.injection_method = injection_method;
      s.stake_amount = stake_amount;
      s.duration = duration;
      s.provider_limit = provider_limit;
      s.update_cycle = update_cycle;
      s.update_start_time = update_start_time;
      s.appeal_freeze_period = 0;
      s.exceeded_risk_control_freeze_period = 0;
      s.guarantee_id = 0;
      s.risk_control_amount = asset(0, core_symbol());
      s.pause_service_stake_amount = asset(0, core_symbol());
      s.freeze_flag = false;
      s.emergency_flag = false;
      s.status = service_status::service_in;
      new_service_id = s.service_id;
      
    });
  }
    print("=====1");
  transfer(account, provider_account, stake_amount, "");
    print("=====1");
  // add provider
  data_providers providertable(_self, _self.value);
  auto provider_itr = providertable.find(account.value);
  if (provider_itr == providertable.end()) {
    providertable.emplace(_self, [&](auto &p) {
      p.account = account;
      p.total_stake_amount = stake_amount;
      // p.pubkey = "";
      p.total_freeze_amount = asset(0, core_symbol());
      p.unconfirmed_amount = asset(0, core_symbol());
      p.claim_amount = asset(0, core_symbol());
      p.last_claim_time = time_point_sec(now());
    });
  } else {
    providertable.modify(provider_itr, same_payer, [&](auto &p) {
      p.total_stake_amount += stake_amount;
    });
  }
    print("=====1");
  data_service_provisions provisionstable(_self, new_service_id);

  auto provision_itr = provisionstable.find(account.value);
  check(provision_itr == provisionstable.end(), "the account has subscribed service   ");

  provisionstable.emplace(_self, [&](auto &p) {
    p.service_id = new_service_id;
    p.account = account;
    p.stake_amount = stake_amount;
    p.freeze_amount = asset(0, core_symbol());
    p.service_income = asset(0, core_symbol());
    p.status = 0;
    p.public_information = "";
    p.stop_service =false;
  });
    print("=====1");
  provider_services provservicestable(_self, account.value);
  uint64_t create_time_sec =
      static_cast<uint64_t>(update_start_time.sec_since_epoch());
  auto provservice_itr = provservicestable.find(create_time_sec);
  check(provservice_itr == provservicestable.end(),
        "same account register service interval too short ");
  provservicestable.emplace(_self, [&](auto &p) {
    p.service_id = new_service_id;
    p.create_time = update_start_time;
  });
    print("=====1");
    data_service_stakes svcstaketable(_self, _self.value);
  auto svcstake_itr = svcstaketable.find(new_service_id);
  if(svcstake_itr == svcstaketable.end())
  {
 svcstaketable.emplace(_self, [&](auto &ss) {
      ss.service_id = new_service_id;
      ss.stake_amount = stake_amount;
      ss.freeze_amount = asset(0, core_symbol());
      ss.unconfirmed_amount = asset(0, core_symbol());
  });
  }
  else
  {
    svcstaketable.modify(svcstake_itr, same_payer,
                         [&](auto &ss) { ss.stake_amount += stake_amount; });
  }


}

void bos_oracle::unstakeasset(uint64_t service_id, name account,
                              asset stake_amount) {
  stakeasset(service_id, account, -stake_amount);
}

/**
 * @brief
 *
 * @param service_id
 * @param account
 * @param stake_amount
 */
void bos_oracle::stakeasset(uint64_t service_id, name account,
                             asset stake_amount) {
  require_auth(account);
  if (stake_amount.amount > 0) {
    transfer(account, provider_account, stake_amount, "");
  }

  data_providers providertable(_self, _self.value);
  auto provider_itr = providertable.find(account.value);
  check(provider_itr != providertable.end(), "");

  data_service_provisions provisionstable(_self, service_id);

  auto provision_itr = provisionstable.find(account.value);
  check(provision_itr != provisionstable.end(),
        "account does not subscribe services");

  if (stake_amount.amount < 0) {
    check(provider_itr->total_stake_amount >=
              asset(std::abs(stake_amount.amount), core_symbol()),
          "");
    check(provision_itr->stake_amount >=
              asset(std::abs(stake_amount.amount), core_symbol()),
          "");
  }

  providertable.modify(provider_itr, same_payer,
                       [&](auto &p) { p.total_stake_amount += stake_amount; });

  provisionstable.modify(provision_itr, same_payer,
                         [&](auto &p) { p.stake_amount += stake_amount; });

  if (stake_amount.amount < 0) {
    transfer(provider_account, account,
             asset(std::abs(stake_amount.amount), core_symbol()), "");
  }


   data_service_stakes svcstaketable(_self, _self.value);
  auto svcstake_itr = svcstaketable.find(service_id);
  if(svcstake_itr == svcstaketable.end())
  {
 svcstaketable.emplace(_self, [&](auto &ss) {
      ss.stake_amount = stake_amount;
      ss.freeze_amount = asset(0, core_symbol());
  });
  }
  else
  {
    svcstaketable.modify(svcstake_itr, same_payer,
                         [&](auto &ss) { ss.stake_amount += stake_amount; });
  }


}

/**
 * @brief
 *
 * @param service_id
 * @param fee_types
 * @param service_prices
 */
void bos_oracle::addfeetypes(uint64_t service_id,
                             std::vector<uint8_t> fee_types,
                             std::vector<asset> service_prices) {
  require_auth(_self);
  check(fee_types.size() > 0 && fee_types.size() == service_prices.size(),
        "fee_types size have to equal service prices size");
  data_service_fees feetable(_self, service_id);
  for (int i = 0; i < fee_types.size(); i++) {
    auto fee_type = fee_types[i];
    auto service_price = service_prices[i];
    addfeetype(service_id, fee_type, service_price);
  }
}

void bos_oracle::addfeetype(uint64_t service_id, uint8_t fee_type,
                            asset service_price) {
  require_auth(_self);
  data_service_fees feetable(_self, service_id);
  check(fee_type >= fee_type::fee_times && fee_type < fee_type::fee_type_count,
        "unknown fee type");

  auto fee_itr = feetable.find(static_cast<uint64_t>(fee_type));
  if (fee_itr == feetable.end()) {
    feetable.emplace(_self, [&](auto &f) {
      f.service_id = service_id;
      f.fee_type = fee_type;
      f.service_price = service_price;
    });
  } else {
    feetable.modify(fee_itr, same_payer, [&](auto &f) {
      f.service_id = service_id;
      f.fee_type = fee_type;
      f.service_price = service_price;
    });
  }
}

/**
 * @brief
 *
 * @param service_id
 * @param provider
 * @param data_json
 * @param is_request
 */
void bos_oracle::multipush(uint64_t service_id, name provider,
                           const string &data_json, bool is_request) {
                             print("==========multipush");
  require_auth(provider);
  check(service_status::service_in == get_service_status(service_id),
        "service and subscription must be available");

  auto push_data = [](uint64_t service_id, name provider, name contract_account,
                      name action_name, uint64_t request_id,
                      const string &data_json) {
    transaction t;
    t.actions.emplace_back(
        permission_level{provider, active_permission}, provider, "pushdata"_n,
        std::make_tuple(service_id, provider, contract_account, action_name,
                        request_id, data_json));
    t.delay_sec = 0;
    uint128_t deferred_id =
        (uint128_t(service_id) << 64) | contract_account.value;
    cancel_deferred(deferred_id);
    t.send(deferred_id, provider);
  };

  if (is_request) {
    // request
    uint64_t request_id = get_request_by_last_push(service_id, provider);
    std::vector<std::tuple<name, name, uint64_t>> receive_contracts =
        get_request_list(service_id, request_id);

    for (const auto &rc : receive_contracts) {
      push_data(service_id, provider, std::get<0>(rc), std::get<1>(rc),
               std::get<2>(rc), data_json);
    }
  } else {

    // subscription
    std::vector<std::tuple<name, name>> subscription_receive_contracts =
        get_subscription_list(service_id);

    for (const auto &src : subscription_receive_contracts) {
      push_data(service_id, provider, std::get<0>(src), std::get<1>(src), 0,
               data_json);
    }
  }
}

/**
 * @brief
 *
 * @param service_id
 * @param provider
 * @param contract_account
 * @param action_name
 * @param data_json
 * @param request_id
 */
void bos_oracle::pushdata(uint64_t service_id, name provider,
                          name contract_account, name action_name,
                          uint64_t request_id, const string &data_json) {
  require_auth(provider);
  check(service_status::service_in == get_service_status(service_id) &&
            subscription_status::subscription_subscribe ==
                get_subscription_status(service_id, contract_account,
                                        action_name),
        "service and subscription must be available");

  if (0 == request_id) {
    time_point_sec pay_time =
        get_payment_time(service_id, contract_account, action_name);

    if (pay_time < time_point_sec(now())) {
      fee_service(service_id, contract_account, action_name,
                  fee_type::fee_month);
    }
  }

  data_service_provision_logs logtable(_self, _self.value);
  logtable.emplace(_self, [&](auto &l) {
    l.log_id = logtable.available_primary_key();
    l.service_id = service_id;
    // l.update_number = update_number;
    l.data_json = data_json;
    // l.status = 0;
    // l.provider_sig = provider_signature;
    l.contract_account = contract_account;
    l.action_name = action_name;
    l.request_id = request_id;
    l.update_time = time_point_sec(now());
  });

  add_times(service_id, provider, contract_account, action_name,
            0 != request_id);

  require_recipient(contract_account);
}

/**
 * @brief
 *
 * @param account
 * @param receive_account
 */
void bos_oracle::claim(name account, name receive_account) {
  require_auth(account);
  provider_services proviservicestable(_self, account.value);
  uint64_t consumption = 0;
  uint64_t month_consumption = 0;
  uint64_t service_times = 0;
  uint64_t provide_times = 0;
  uint64_t service_month_times = 0;
  uint64_t provide_month_times = 0;
  uint64_t claimreward = 0;
  uint64_t income = 0;
  uint64_t month_income = 0;

  asset stake_freeze_amount = asset(0, core_symbol());
  asset service_stake_freeze_amount = asset(0, core_symbol());
  asset total_stake_freeze_amount = asset(0, core_symbol());
  uint64_t stake_freeze_income = 0;

  data_providers providertable(_self, _self.value);
  auto provider_itr = providertable.find(account.value);
  check(provider_itr != providertable.end(), "");

  check(time_point_sec(now()) - provider_itr->last_claim_time >= eosio::days(1),
        "claim span must be greater than one day");

  auto calc_income = [](uint64_t service_times, uint64_t provide_times,
                        uint64_t consumption) -> uint64_t {
    check(provide_times > 0 && service_times > provide_times,
          "provider times and service_times must greater than zero");
    uint64_t income = 0;
    // if (provide_times > 0 && service_times > provide_times) {
    income = consumption * provide_times / static_cast<double>(service_times);
    // }

    return income;
  };

  for (const auto &p : proviservicestable) {
    std::tie(consumption, month_consumption) = get_consumption(p.service_id);
    std::tie(service_times, service_month_times, provide_times,
             provide_month_times) = get_times(p.service_id, account);

    std::tie(stake_freeze_amount, service_stake_freeze_amount) =
        get_freeze_stat(p.service_id, account);

    income += calc_income(service_times, provide_times, consumption * 0.8);

    month_income += calc_income(service_month_times, provide_month_times,
                                month_consumption * 0.8);

    uint64_t stake_income = (consumption + month_consumption) * 0.8;
    check(stake_freeze_amount.amount > 0 &&
              service_stake_freeze_amount.amount > stake_freeze_amount.amount,
          "provider freeze_amount and service_times must greater than zero");

    stake_freeze_income =
        stake_income * stake_freeze_amount.amount /
        static_cast<double>(service_stake_freeze_amount.amount);
  }
  asset new_income =
      asset(income + month_income + stake_freeze_income, core_symbol()) -
      provider_itr->claim_amount;

  check(new_income.amount > 0, "no income ");

  providertable.modify(provider_itr, same_payer,
                       [&](auto &p) { p.claim_amount += new_income; });

  transfer(consumer_account, account, new_income, "claim");
}

/**
 * @brief
 *
 * @param service_id
 * @param action_type
 */
void bos_oracle::execaction(uint64_t service_id, uint64_t action_type) {
  require_auth(_self);
  data_services svctable(_self, _self.value);
  auto service_itr = svctable.find(service_id);
  check(service_itr != svctable.end(), "service does not exist");
  svctable.modify(service_itr, _self, [&](auto &s) {
    if (0 == action_type) {
      s.freeze_flag = true;
    } else {
      s.emergency_flag = true;
    }
  });
}

/**
 * @brief
 *
 * @param service_id
 * @param signature
 * @param account
 * @param status
 */
void bos_oracle::unregservice(uint64_t service_id, name account,
                              uint64_t status) {
  require_auth(account);
  // data_services svctable(_self, _self.value);
  // auto service_itr = svctable.find(service_id);
  // check(service_itr != svctable.end(), "service does not exist");
  // svctable.modify(service_itr, _self, [&](auto &s) { s.status = status; });

  data_service_provisions provisionstable(_self, service_id);
  auto provision_itr = provisionstable.find(account.value);
  check(provision_itr != provisionstable.end(),
        "provider does not subscribe service ");

  provisionstable.modify(provision_itr, same_payer,
                         [&](auto &p) { p.status = status; });

  if (static_cast<uint64_t>(service_status::service_cancel) == status) {
    svc_provision_cancel_applys applytable(_self, _self.value);
    applytable.emplace(_self, [&](auto &a) {
      a.apply_id = applytable.available_primary_key();
      a.service_id = service_id;
      a.provider = account;
      a.status = apply_status::apply_init;
      a.cancel_time = time_point_sec(now());
      a.finish_time = time_point_sec(now());
    });

    provider_services provservicestable(_self, account.value);

    auto provservice_itr = provservicestable.find(service_id);
    check(provservice_itr != provservicestable.end(),
          "provider does not subscribe service");
    provservicestable.erase(provservice_itr);

    asset freeze_amount = asset(0, core_symbol()); /// calculates  unfinish code
    asset stake_amount = asset(0, core_symbol());
    check(asset(0, core_symbol()) == freeze_amount,
          "freeze amount must equal zero");

    transfer(provider_account, account, stake_amount, "");
  }
}

// } // namespace bosoracle