#pragma once
#include "bos.types.hpp"
#include <eosiolib/eosio.hpp>

// using namespace eosio;
// using std::string;

void register(uint64_t service_id, name account, uint64_t service_price,
              uint64_t fee_type, std::string data_format, uint64_t data_type,
              std::string criteria, uint64_t acceptance,
              std::string declaration, uint64_t injection_method,
              uint64_t stake_amount, uint64_t duration, uint64_t provider_limit,
              uint64_t update_cycle, uint64_t update_start_time) {
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
      s.risk_control_amount = 0;
      s.pause_service_stake_amount = 0;
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
      p.pubkey = publickey;
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
enum data_service_status : uint8_t { normal = 0, cancel, pause };
void unregister(uint64_t service_id, std::string signature, name account,
                uint64_t status) {

  auto service_itr = svctable.find(service_id);
  check(service_itr == svctable.end(), "service does not exist");
  svctable.modify(service_itr, _self, [&](auto &s) { s.status = status; });

  if (static_cast<uint64_t>(data_service_status::cancel) == status) {
    svc_provision_cancel_applys applytable(_self, _self.value);
    applytable.emplace(_self, [&](auto &a) {
      l.apply_id = applytable.available_primary_key();
      l.service_id = service_id;
      l.provider = account;
      l.status = 0;
      l.cancel_time = now();
      l.finish_time = now();
    });

    transfer(provider_account, account, stake_amount, "");
  }
}

void execction(uint64_t service_id, uint64_t action_type) {

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

void stakeamount(uint64_t service_id, uint64_t provider_id, name account,
                 std::string signature, asset stake_amount) {

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

  auto provision_itr = provisionstable.find(acc_itr.provision_id);
  check(provision_itr != provisionstable.end(),
        "account does not subscribe services");

  if (stake_amount.amount < 0) {
    check(provider_itr->total_stake_amount >= asset(fabs(stake_amount.amount)),
          "");
    check(provision_itr->total_stake_amount >= asset(fabs(stake_amount.amount)),
          "");
  }

  providertable.modify(provider_itr, same_payer,
                       [&](auto &p) { p.total_stake_amount += stake_amount; });

  provisionstable.modify(provision_itr, same_payer,
                         [&](auto &p) { p.stake_amount += stake_amount; });

  if (stake_amount.amount < 0) {
    transfer(provider_account, account, asset(fabs(stake_amount.amount)), "");
  }
}

void pushdata(uint64_t service_id, uint64_t update_number, uint64_t data_json,
              uint64_t provider_signature, uint64_t request_id) {
  data_service_provision_log logtable(_self, _self.value);
  logtable.emplace(_self, [&](auto &l) {
    l.log_id = logtable.available_primary_key();
    l.service_id = new_service_id;
    l.update_number = update_number;
    l.data_json = data_json;
    l.status = 0;
    l.provider_sig = provider_signature;
    l.request_id = request_id;
    l.update_time = now();
  });
}

void addfeetype(uint64_t service_id, asset service_price, uint8_t fee_type) {
  data_service_fees feetable(_self, _self.value);
  auto fee_itr = feetable.find(account.value);
  if (fee_itr == feetable.end()) {
    feetable.emplace(_self, [&](auto &f) {
      f.service_id = service_id;
      f.service_price = service_price;
      f.fee_type = fee_type;
    });
  } else {
    feetable.modify(fee_itr, same_payer, [&](auto &f) {
      f.service_id = service_id;
      f.service_price = service_price;
      f.fee_type = fee_type;
    });
  }
}
// } // namespace bosoracle