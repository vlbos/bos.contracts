#pragma once

#include "bos.oracle/bos.oracle.hpp"
#include <cmath>
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
using namespace eosio;
using std::string;

void bos_oracle::regservice(uint64_t service_id, name account,
                            asset stake_amount, asset service_price,
                            uint64_t fee_type, std::string data_format,
                            uint64_t data_type, std::string criteria,
                            uint64_t acceptance, std::string declaration,
                            uint64_t injection_method, time_point_sec duration,
                            uint64_t provider_limit, uint64_t update_cycle,
                            time_point_sec update_start_time) {
  uint64_t new_service_id = service_id;
  data_services svctable(_self, _self.value);
  if (0 == service_id) {
    // add service

    svctable.emplace(_self, [&](auto &s) {
      s.service_id = svctable.available_primary_key();
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
      s.freeze_flag = 0;
      s.emergency_flag = 0;
      s.status = 0;
      new_service_id = s.service_id;
    });
  }

  transfer(account, provider_account, stake_amount, "");

  // add provider
  data_providers providertable(_self, _self.value);
  auto provider_itr = providertable.find(account.value);
  if (provider_itr == providertable.end()) {
    providertable.emplace(_self, [&](auto &p) {
      p.account = account;
      // p.pubkey = publickey;
      p.total_stake_amount = stake_amount;
    });
  } else {
    providertable.modify(provider_itr, same_payer, [&](auto &p) {
      p.total_stake_amount += stake_amount;
    });
  }

  data_service_provisions provisionstable(_self, _self.value);

  provisionstable.emplace(_self, [&](auto &p) {
    p.provision_id = provisionstable.available_primary_key();
    p.service_id = new_service_id;
    p.account = account;
    p.stake_amount = stake_amount;
  });
}

void bos_oracle::unregservice(uint64_t service_id, std::string signature,
                              name account, uint64_t status) {

  data_services svctable(_self, _self.value);
  auto service_itr = svctable.find(service_id);
  check(service_itr == svctable.end(), "service does not exist");
  svctable.modify(service_itr, _self, [&](auto &s) { s.status = status; });

  if (static_cast<uint64_t>(data_service_status::service_cancel) == status) {
    svc_provision_cancel_applys applytable(_self, _self.value);
    applytable.emplace(_self, [&](auto &a) {
      a.apply_id = applytable.available_primary_key();
      a.service_id = service_id;
      a.provider = account;
      a.status = 0;
      a.cancel_time = time_point_sec(now());
      a.finish_time = time_point_sec(now());
    });

    asset freeze_amount = asset(0, core_symbol()); /// calculates
    asset stake_amount = asset(0, core_symbol());
    check(asset(0, core_symbol()) == freeze_amount,
          "freeze amount must equal zero");

    transfer(provider_account, account, stake_amount, "");
  }
}

void bos_oracle::execaction(uint64_t service_id, uint64_t action_type) {
  data_services svctable(_self, _self.value);
  auto service_itr = svctable.find(service_id);
  check(service_itr == svctable.end(), "service does not exist");
  svctable.modify(service_itr, _self, [&](auto &s) {
    if (0 == action_type) {
      s.freeze_flag = true;
    } else {
      s.emergency_flag = true;
    }
  });
}

uint8_t bos_oracle::get_service_status(uint64_t service_id) {
  data_services svctable(_self, _self.value);
  auto service_itr = svctable.find(service_id);
  check(service_itr == svctable.end(), "service does not exist");

  return service_itr->status;
}

void bos_oracle::stakeamount(uint64_t service_id, uint64_t provider_id,
                             name account, std::string signature,
                             asset stake_amount) {

  if (stake_amount.amount > 0) {
    transfer(account, provider_account, stake_amount, "");
  }

  data_providers providertable(_self, _self.value);
  auto provider_itr = providertable.find(account.value);
  check(provider_itr != providertable.end(), "");

  data_service_provisions provisionstable(_self, _self.value);
  auto provision_idx = provisionstable.get_index<"byaccount"_n>();
  auto acc_itr = provision_idx.lower_bound(account.value);
  check(acc_itr != provision_idx.end(), "account does not subscribe services");

  while (acc_itr != provision_idx.end() && acc_itr->service_id != service_id) {
    acc_itr++;
  }

  check(acc_itr != provision_idx.end() && acc_itr->service_id == service_id,
        "account does not subscribe services");

  auto provision_itr = provisionstable.find(acc_itr->provision_id);
  check(provision_itr != provisionstable.end(),
        "account does not subscribe services");

  if (stake_amount.amount < 0) {
    check(provider_itr->total_stake_amount >=
              asset(fabs(stake_amount.amount), core_symbol()),
          "");
    check(provision_itr->stake_amount >=
              asset(fabs(stake_amount.amount), core_symbol()),
          "");
  }

  providertable.modify(provider_itr, same_payer,
                       [&](auto &p) { p.total_stake_amount += stake_amount; });

  provisionstable.modify(provision_itr, same_payer,
                         [&](auto &p) { p.stake_amount += stake_amount; });

  if (stake_amount.amount < 0) {
    transfer(provider_account, account,
             asset(fabs(stake_amount.amount), core_symbol()), "");
  }
}

void bos_oracle::pushdata(uint64_t service_id,name provider, 
                          name contract_account, name action_name,
                          uint64_t data_json, uint64_t request_id) {
  require_auth(provider);
  check(data_service_status::service_in == get_service_status(service_id) &&
            data_service_subscription_status::service_subscribe ==
                get_subscription_status(service_id, contract_account,
                                        action_name),
        "service and subscription must be available");

  if (0 == request_id) {
    time_point_sec pay_time =
        get_payment_time(service_id, contract_account, action_name);

    if (pay_time < time_point_sec(now())) {
      fee_service(service_id, contract_account, action_name,
                  data_service_fee_type::fee_month);
    }
  }

  data_service_provision_logs logtable(_self, _self.value);
  logtable.emplace(_self, [&](auto &l) {
    l.log_id = logtable.available_primary_key();
    l.service_id = service_id;
    // l.update_number = update_number;
    l.data_json = data_json;
    l.status = 0;
    // l.provider_sig = provider_signature;
    l.contract_account = contract_account;
    l.action_name = action_name;
    l.request_id = request_id;
    l.update_time = time_point_sec(now());
  });

  addtimes(service_id,provider,contract_account,  action_name);
}


void bos_oracle::multipush(uint64_t service_id,name provider, 
                          uint64_t data_json,bool is_request) {
  require_auth(provider);
  check(data_service_status::service_in == get_service_status(service_id),
        "service and subscription must be available");

  if (is_request) {
    // request
    uint64_t request_id = get_request_by_last_push( service_id,provider);
    std::vector<std::tuple<name, name,uint64_t>> receive_contracts =
        get_request_list(service_id, request_id);

    for (const auto &rc : receive_contracts) {
      pushdata( service_id,provider, std::get<0>(rc), std::get<1>(rc),
               data_json, std::get<2>(rc));
    }
  } 
  else {

    // subscription
    std::vector<std::tuple<name, name>> subscription_receive_contracts =
        get_subscription_list(service_id);

    for (const auto &src : subscription_receive_contracts) {
      pushdata( service_id, provider,std::get<0>(src), std::get<1>(src),
               data_json, 0);
    }
  }
}



uint64_t bos_oracle::get_request_by_last_push(uint64_t service_id,name provider ) {

  data_service_provision_logs logtable(_self, service_id);
  auto update_time_idx = logtable.get_index<"bytime"_n>();

  uint64_t request_id = 0;
  for (const auto &u : update_time_idx) {
    if (provider==u.account && 0 != u.request_id) {
      request_id = u.request_id;
      break;
    }
  }

  return request_id;
}


// std::map<name,uint64_t> bos_oracle::get_push_times_by_action(uint64_t service_id,
//                           name contract_account, name action_name,bool is_request) {

//   data_service_provision_logs logtable(_self, service_id);

//  std::map<name,uint64_t> provider2pushtimes;
//   auto action_idx = data_service_provision_logs.get_index<"byaction"_n>();
//  auto id = get_hash_key(get_nn_hash(contract_account,action_name));
// auto lower =action_idx.lower_bound(id);
// auto upper =action_idx.upper_bound(id);
//   for (;lower != upper;++lower) {
//      name account = lower->account;
//      bool flag = is_request?(0==lower->request_id):(0!=lower->request_id);
     
//      if(flag){
//           continue;
//      }

//    std::map<name,uint64_t>::iterator it = provider2pushtimes.find(account);
//    if(it == provider2pushtimes.end()){
//      provider2pushtimes[account] = 1;
//    }
//    else{
//      provider2pushtimes[account]++;
//    }
//   }

//   return provider2pushtimes;
// }


void bos_oracle::addtimes(uint64_t service_id, name account,
                          name contract_account, name action_name) {
 
  push_records pushtable(_self, service_id);
   provider_push_records providertable(_self, service_id);
    action_push_records actiontable(_self, service_id);
 provider_action_push_records provideractiontable(_self, service_id);


    auto push_itr = pushtable.find(service_id);
    if (push_itr == pushtable.end()) {
      providertable.emplace(_self, [&](auto &p) {
        p.service_id = service_id;
        p.times =1;
      });
    } 
    else {
      pushtable.modify(push_itr, same_payer, [&](auto &p) {
          p.times +=1;
      });
    }

    auto provider_itr = providertable.find(account.value);
    if (provider_itr == providertable.end()) {
      providertable.emplace(_self, [&](auto &p) {
        p.service_id = service_id;
        p.account = account;
        p.times =1;
      });
    } 
    else {
      providertable.modify(provider_itr, same_payer, [&](auto &p) {
          p.times +=1;
      });
    }

   uint64_t a_id = get_hash_key(get_nn_hash(contract_account, action_name));
    auto action_itr = actiontable.find(a_id);
    if (action_itr == actiontable.end()) {
      actiontable.emplace(_self, [&](auto &a) {
        a.service_id = service_id;
        a.contract_account = contract_account;
        a.action_name = action_name;
        a.times =1;
      });
    } 
    else {
      actiontable.modify(action_itr, same_payer, [&](auto &a) {
          a.times +=1;
      });
    }

    
    uint64_t pa_id = get_hash_key(get_nnn_hash(account,contract_account, action_name));
    auto provider_action_itr = provideractiontable.find(pa_id);
    if (provider_action_itr == provideractiontable.end()) {
      provideractiontable.emplace(_self, [&](auto &pa) {
        pa.service_id = service_id;
        pa.account = account;
        pa.contract_account = contract_account;
        pa.action_name = action_name;
        pa.times =1;
      });
    } 
    else {
      provideractiontable.modify(provider_action_itr, same_payer, [&](auto &pa) {
          pa.times +=1;
      });
    }
  

}


void bos_oracle::addfeetype(uint64_t service_id, std::vector<uint8_t> fee_types,
                            std::vector<asset> service_prices) {
  check(fee_types.size() > 0 && fee_types.size() == service_prices.size(),
        "fee_types size have to equal service prices size");
  data_service_fees feetable(_self, _self.value);
  for (int i = 0; i < fee_types.size(); i++) {
    auto type = fee_types[i];
    auto price = service_prices[i];
    check(type >= data_service_fee_type::fee_times &&
              type < data_service_fee_type::fee_type_count,
          "unknown fee type");

    uint64_t id = get_hash_key(get_uu_hash(service_id, type));
    auto fee_itr = feetable.find(id);
    if (fee_itr == feetable.end()) {
      feetable.emplace(_self, [&](auto &f) {
        f.id = id;
        f.service_id = service_id;
        f.fee_type = type;
        f.service_price = price;
      });
    } else {
      feetable.modify(fee_itr, same_payer, [&](auto &f) {
        f.service_id = service_id;
        f.fee_type = type;
        f.service_price = price;
      });
    }
  }
}

asset bos_oracle::get_price_by_fee_type(uint64_t service_id, uint8_t fee_type) {
  //  check(fee_types.size() > 0 && fee_types.size() ==
  //  service_prices.size(),"fee_types size have to equal service prices size");
  data_service_fees feetable(_self, _self.value);
  // for(int i = 0;i < fee_types.size();i++)
  // {
  auto type = fee_type; // s[i];
  // auto price = service_prices[i];
  check(type >= data_service_fee_type::fee_times &&
            type < data_service_fee_type::fee_type_count,
        "unknown fee type");

  auto fee_itr = feetable.find(get_hash_key(get_uu_hash(service_id, type)));
  check(fee_itr != feetable.end(), " service's fee type does not found");

  return fee_itr->service_price;
}

// } // namespace bosoracle