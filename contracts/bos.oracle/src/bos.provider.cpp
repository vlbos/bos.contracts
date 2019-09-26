#include "bos.oracle/bos.oracle.hpp"
#include <cmath>
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/transaction.hpp>
using namespace eosio;
using std::string;
/**
 * @brief
 *
 * @param service_id
 * @param account
 * @param amount
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
void bos_oracle::regservice(name account, asset base_stake_amount, std::string data_format, uint8_t data_type, std::string criteria, uint8_t acceptance, uint8_t injection_method, uint32_t duration,
                            uint8_t provider_limit, uint32_t update_cycle) {
   require_auth(account);

   std::string checkmsg = "set base stake amount could not be less than " + std::to_string(service_stake_limit);
   check(base_stake_amount.amount >= uint64_t(service_stake_limit) * pow(10, core_symbol().precision()), checkmsg);

   check(provider_limit >= 3 && provider_limit <= 100, "provider_limit could not be less than 3 or greater than 100");
   check(update_cycle >= 60 && update_cycle <= 3600 * 24 * uint32_t(100), "update_cycle could not be less than 60 seconds or greater than 100 days");
   check(duration >= 30 && duration <= 3600, "duration could not be less than 30 or greater than 3600 seconds");
   check(duration < update_cycle - 10, "duration could not be  greater than update_cycle-10 seconds");
   check(injection_method == chain_indirect || injection_method == chain_direct || injection_method == chain_outside,
         "injection_method only set chain_indirect(0) or chain_direct(1)or chain_outside(2)");
   check(data_type == data_deterministic || data_type == data_non_deterministic, "data_type only set value data_deterministic(0) or data_non_deterministic(1)1");
   check(acceptance >= 3 && acceptance <= 100, "acceptance could not be less than 3 or greater than 100 ");
   check(data_format.size() <= 256, "data_format could not be greater than 256");
   check(criteria.size() <= 256, "criteria could not be greater than 256");
   // check(declaration.size() <= 256, "declaration could not be greater than 256");
   // check(update_start_time > bos_oracle::current_time_point_sec(), "update_start_time could not be earlier than current time");

   data_services svctable(_self, _self.value);

   uint64_t id = svctable.available_primary_key();
   if (0 == id) {
      id++;
   }

   save_id(0, id);

   // add service
   svctable.emplace(_self, [&](auto& s) {
      s.service_id = id;
      s.data_format = data_format;
      s.data_type = data_type;
      s.criteria = criteria;
      s.acceptance = acceptance;
      s.declaration = "";
      s.injection_method = injection_method;
      s.base_stake_amount = base_stake_amount;
      s.duration = duration;
      s.provider_limit = provider_limit;
      s.update_cycle = update_cycle;
      s.update_start_time = bos_oracle::current_time_point_sec(); // update_start_time;
      s.last_update_number = 0;
      s.appeal_freeze_period = 0;
      s.exceeded_risk_control_freeze_period = 0;
      s.guarantee_id = 0;
      s.risk_control_amount = asset(0, core_symbol());
      s.pause_service_stake_amount = asset(0, core_symbol());
      s.status = service_status::service_init;
   });
}

void bos_oracle::save_id(uint8_t id_type, uint64_t id) {
   oracle_ids idstable(_self, _self.value);
   auto ids_itr = idstable.find(id_type);
   if (ids_itr == idstable.end()) {
      idstable.emplace(_self, [&](auto& s) {
         s.id = id;
         s.id_type = id_type;
      });
   } else {
      idstable.modify(ids_itr, same_payer, [&](auto& s) { s.id = id; });
   }
}

void bos_oracle::reg_service_provider(uint64_t service_id, name account) {
   require_auth(account);

   data_services svctable(_self, _self.value);
   auto service_itr = svctable.find(service_id);
   check(service_itr != svctable.end(), "no service id in reg_service_provider");

   // add provider
   data_providers providertable(_self, _self.value);
   auto provider_itr = providertable.find(account.value);
   if (provider_itr == providertable.end()) {
      providertable.emplace(_self, [&](auto& p) {
         p.account = account;
         p.total_stake_amount = asset(0, core_symbol());
         p.total_freeze_amount = asset(0, core_symbol());
         p.unconfirmed_amount = asset(0, core_symbol());
         p.claim_amount = asset(0, core_symbol());
         p.last_claim_time = time_point_sec();
         p.services.push_back(service_id);
      });
   } else {
      auto provision = std::find(provider_itr->services.begin(), provider_itr->services.end(), service_id);
      if (provision == provider_itr->services.end()) {
         providertable.modify(provider_itr, same_payer, [&](auto& p) { p.services.push_back(service_id); });
      }
   }

   data_service_provisions provisionstable(_self, service_id);

   auto provision_itr = provisionstable.find(account.value);
   if (provision_itr == provisionstable.end()) {
      provisionstable.emplace(_self, [&](auto& p) {
         p.service_id = service_id;
         p.account = account;
         p.amount = asset(0, core_symbol());
         p.freeze_amount = asset(0, core_symbol());
         p.service_income = asset(0, core_symbol());
         p.status = provision_status::provision_reg;
         p.public_information = "";
         p.create_time = bos_oracle::current_time_point_sec();
      });
   }
}

void bos_oracle::check_service_status(uint64_t service_id) {
   data_services svctable(_self, _self.value);
   auto service_itr = svctable.find(service_id);
   check(service_itr != svctable.end(), "no service id in check_service_status");

   check(service_itr->status == service_status::service_in, " service status is not 'in(1)' ");
}

void bos_oracle::update_service_status(uint64_t service_id) {
   print("update_service_status in");
   data_services svctable(_self, _self.value);
   auto service_itr = svctable.find(service_id);
   check(service_itr != svctable.end(), "no service id in update_service_status");
   data_service_provisions provisionstable(_self, service_id);

   uint16_t available_service_providers_count = 0;

   for (auto& p : provisionstable) {
      if (provision_status::provision_reg == p.status) {
         available_service_providers_count++;
      }
   }

   print("\navailable_service_providers_count=", available_service_providers_count, "status", service_itr->status);
   if (available_service_providers_count < service_itr->provider_limit && service_status::service_in == service_itr->status) {
      svctable.modify(service_itr, same_payer, [&](auto& p) {
         p.status = service_status::service_pause_insufficient_providers;
         print("\navailable_service_providers_count=", available_service_providers_count, "status", service_itr->status);
      });
   } else if (available_service_providers_count >= service_itr->provider_limit &&
              (service_status::service_init == service_itr->status || service_status::service_pause_insufficient_providers == service_itr->status)) {
      print("\n ======svctable.modify(service_itr, same_payer, [&](auto& p) { p.status = service_status::service_in; });");
      svctable.modify(service_itr, same_payer, [&](auto& p) { p.status = service_status::service_in; });
   }
}

void bos_oracle::check_service_provider_status(uint64_t service_id, name account) {
   check_service_status(service_id);
   // print("===service_id==");
   // print(service_id);
   data_service_provisions provisionstable(_self, service_id);

   auto provision_itr = provisionstable.find(account.value);
   check(provision_itr != provisionstable.end(), "the account has not provided service   ");

   check(provision_itr->status == provision_status::provision_reg, "the account's provision service status is not 'reg(0)'  ");
}

void bos_oracle::update_service_provider_status(uint64_t service_id, name account) {
   data_services svctable(_self, _self.value);
   auto service_itr = svctable.find(service_id);
   check(service_itr != svctable.end(), "no service id in update_service_provider_status");

   data_service_provisions provisionstable(_self, service_id);
   auto provision_itr = provisionstable.find(account.value);
   check(provision_itr != provisionstable.end(), "the account has not provided service");

   if (provision_itr->amount <= asset(0, core_symbol())) {
      print("========================zero================");
      provisionstable.modify(provision_itr, same_payer, [&](auto& p) { p.status = provision_status::provision_suspend; });
   } else if (provision_itr->amount - provision_itr->freeze_amount <= asset(0, core_symbol())) {
      print("========================freeze================");
      provisionstable.modify(provision_itr, same_payer, [&](auto& p) { p.status = provision_status::provision_freeze_suspend; });
   } else if (provision_itr->amount >= service_itr->base_stake_amount &&
              (provision_status::provision_freeze_suspend == provision_itr->status || provision_status::provision_suspend == provision_itr->status)) {
      provisionstable.modify(provision_itr, same_payer, [&](auto& p) { p.status = provision_status::provision_reg; });
   }

   update_service_status(service_id);
}

void bos_oracle::unstakeasset(uint64_t service_id, name account, asset amount, std::string memo) {
   check(memo.size() <= 256, "memo could not be greater than 256");

   require_auth(account);
   update_stake_asset(service_id, account, -amount);
}

void bos_oracle::stake_asset(uint64_t service_id, name account, asset amount, std::string memo) {
   check(memo.size() <= 256, "memo could not be greater than 256");

   require_auth(account);
   reg_service_provider(service_id, account);
   update_stake_asset(service_id, account, amount);
}

/**
 * @brief
 *
 * @param service_id
 * @param account
 * @param amount
 */
void bos_oracle::update_stake_asset(uint64_t service_id, name account, asset amount) {

   // if (amount.amount > 0) {
   //    transfer(account, provider_account, amount, "");
   // }
   data_services svctable(_self, _self.value);
   auto service_itr = svctable.find(service_id);
   check(service_itr != svctable.end(), "no service id in update_stake_asset");

   data_providers providertable(_self, _self.value);
   auto provider_itr = providertable.find(account.value);
   check(provider_itr != providertable.end(), "no provider in update_stake_asset");

   data_service_provisions provisionstable(_self, service_id);

   auto provision_itr = provisionstable.find(account.value);
   check(provision_itr != provisionstable.end(), "the account does not provide services");

   if (amount.amount < 0) {
      check(provider_itr->total_stake_amount >= asset(std::abs(amount.amount), core_symbol()), "total stake amount should be greater than unstake asset amount");
      check(provision_itr->amount >= asset(std::abs(amount.amount), core_symbol()), "service stake amount should be greater than unstake asset amount");
   }

   providertable.modify(provider_itr, same_payer, [&](auto& p) { p.total_stake_amount += amount; });

   provisionstable.modify(provision_itr, same_payer, [&](auto& p) {
      p.amount += amount;
      check(p.amount >= service_itr->base_stake_amount, "stake amount could not be less than  the base_stake amount of the service");

      if (provision_status::provision_unreg != p.status && p.amount - p.freeze_amount >= service_itr->base_stake_amount) {
         p.status = provision_status::provision_reg;
      }
   });

   if (amount < asset(0, core_symbol())) {
      transfer(_self, account, asset(std::abs(amount.amount), core_symbol()), "");
   }

   data_service_stakes svcstaketable(_self, _self.value);
   auto svcstake_itr = svcstaketable.find(service_id);
   if (svcstake_itr == svcstaketable.end()) {
      svcstaketable.emplace(_self, [&](auto& ss) {
         ss.service_id = service_id;
         ss.amount = amount;
         ss.freeze_amount = asset(0, core_symbol());
         ss.unconfirmed_amount = asset(0, core_symbol());
      });
   } else {
      svcstaketable.modify(svcstake_itr, same_payer, [&](auto& ss) { ss.amount += amount; });
   }

   update_service_status(service_id);
}

/**
 * @brief
 *
 * @param service_id
 * @param fee_types
 * @param service_prices
 */
void bos_oracle::addfeetypes(uint64_t service_id, std::vector<uint8_t> fee_types, std::vector<asset> service_prices) {
   require_auth(_self);
   check(fee_types.size() > 0 && fee_types.size() == service_prices.size(), "fee_types size have to equal service prices size");
   data_service_fees feetable(_self, service_id);
   for (int i = 0; i < fee_types.size(); i++) {
      auto fee_type = fee_types[i];
      auto service_price = service_prices[i];
      addfeetype(service_id, fee_type, service_price);
   }
}

void bos_oracle::addfeetype(uint64_t service_id, uint8_t fee_type, asset service_price) {
   require_auth(_self);

   data_services svctable(get_self(), get_self().value);
   auto service_itr = svctable.find(service_id);
   check(service_itr != svctable.end(), "no service id in addfeetype");

   data_service_fees feetable(_self, service_id);
   check(fee_type >= fee_type::fee_times && fee_type < fee_type::fee_type_count, "unknown fee type");

   auto fee_itr = feetable.find(static_cast<uint64_t>(fee_type));
   if (fee_itr == feetable.end()) {
      feetable.emplace(_self, [&](auto& f) {
         f.service_id = service_id;
         f.fee_type = fee_type;
         f.service_price = service_price;
      });
   } else {
      feetable.modify(fee_itr, same_payer, [&](auto& f) {
         f.service_id = service_id;
         f.fee_type = fee_type;
         f.service_price = service_price;
      });
   }
}

void bos_oracle::pushdata(uint64_t service_id, name provider, uint64_t update_number, uint64_t request_id, string data_json) {
   require_auth(provider);
   check(data_json.size() <= 256, "data_json could not be greater than 256");

   check(!(0 != update_number && 0 != request_id), "both update_number and request_id could not be greater than 0");

   if (0 == request_id) {
      check_service_current_update_number(service_id, update_number);
   }

   check_service_provider_status(service_id, provider);

   data_services svctable(_self, _self.value);
   auto service_itr = svctable.find(service_id);
   check(service_itr != svctable.end(), "no service id in pushdata");

   check(check_provider_no_push_data(service_id, provider, update_number, request_id, service_itr->data_type), "repeat push data");

   if (service_itr->data_type == data_type::data_deterministic) {
      innerpublish(service_id, provider, update_number, request_id, data_json);
   } else {
      innerpush(service_id, provider, update_number, request_id, data_json);
   }
}
/**
 * @brief
 *
 * @param service_id
 * @param provider
 * @param contract_account
 * @param data_json
 * @param request_id
 */
void bos_oracle::innerpush(uint64_t service_id, name provider, uint64_t update_number, uint64_t request_id, string data_json) {
   check(data_json.size() <= 256, "data_json could not be greater than 256");
   data_services svctable(get_self(), get_self().value);
   auto service_itr = svctable.find(service_id);
   check(service_itr != svctable.end(), "no service id in innerpush");
   uint64_t servic_injection_method = service_itr->injection_method;

   check(service_status::service_in == get_service_status(service_id), "service and subscription must be available");

   data_service_provision_logs logtable(_self, service_id);
   logtable.emplace(_self, [&](auto& l) {
      l.log_id = logtable.available_primary_key();
      l.service_id = service_id;
      l.account = provider;
      l.data_json = data_json;
      l.update_number = update_number;
      l.status = log_status::log_init;
      l.contract_account = name{};
      l.request_id = request_id;
      l.update_time = bos_oracle::current_time_point_sec();
   });

   if (injection_method::chain_indirect == servic_injection_method) {
      save_publish_data(service_id, update_number, request_id, data_json, data_non_deterministic);
   } else {
      print("does not support  'direct push' feature");
      // /////////////////reserve  version 2.0 above
      // //  check(service_status::service_in == get_service_status(service_id) && subscription_status::subscription_subscribe == get_subscription_status(service_id, contract_account),
      // //    "service and subscription must be available");
      // // // subscription
      // // std::vector<name> subscription_receive_contracts = get_subscription_list(service_id);

      // // for (const auto& src : subscription_receive_contracts) {
      // //    if (0 == request_id) {
      // //    time_point_sec pay_time = get_payment_time(service_id, contract_account);
      // //    pay_time += eosio::days(30);
      // //    if (pay_time < bos_oracle::current_time_point_sec()) {
      // //       fee_service(service_id, contract_account, fee_type::fee_month);
      // //    }
      // // }
      // // add_times(service_id, provider, contract_account, 0 != request_id);
      // //    push_data(service_id, provider, src, 0, data_json);
      // // }
      // // require_recipient(contract_account);
      // transaction t;
      // t.actions.emplace_back(permission_level{_self, active_permission}, _self, "oraclepush"_n, std::make_tuple(service_id, 0, request_id, data_json));
      // t.delay_sec = 0;
      // uint128_t deferred_id = (uint128_t(service_id) << 64) | (update_number + request_id);
      // cancel_deferred(deferred_id);
      // t.send(deferred_id, _self, true);
   }
}

void bos_oracle::oraclepush(uint64_t service_id, uint64_t update_number, uint64_t request_id, string data_json, name contract_account) {
   check(data_json.size() <= 256, "data_json could not be greater than 256");
   require_auth(_self);
   require_recipient(contract_account);
}

void bos_oracle::innerpublish(uint64_t service_id, name provider, uint64_t update_number, uint64_t request_id, string data_json) {
   check(data_json.size() <= 256, "data_json could not be greater than 256");
   print(" innerpublish in");
   name contract_account = _self; // placeholder
   // require_auth(_self);
   check(service_status::service_in == get_service_status(service_id), "service and subscription must be available");

   data_service_provision_logs logtable(_self, service_id);
   logtable.emplace(_self, [&](auto& l) {
      l.log_id = logtable.available_primary_key();
      l.service_id = service_id;
      l.account = provider;
      l.data_json = data_json;
      l.update_number = update_number;
      l.status = log_status::log_init;
      l.contract_account = contract_account;
      l.request_id = request_id;
      l.update_time = bos_oracle::current_time_point_sec();
   });

   add_times(service_id, provider, contract_account, 0 != request_id);

   print("check publish before=", provider, "s=", service_id, "u=", update_number);
   check_publish_service(service_id, update_number, request_id);
   // require_recipient(contract_account);
}

/**
 * @brief
 *
 * @param account
 * @param receive_account
 */
void bos_oracle::claim(name account, name receive_account) {
   require_auth(account);
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

   check(bos_oracle::current_time_point_sec() - provider_itr->last_claim_time >= eosio::days(1), "claim span must be greater than one day");

   auto calc_income = [](uint64_t service_times, uint64_t provide_times, uint64_t consumption) -> uint64_t {
      uint64_t income = 0;
      if (provide_times > 0 && service_times >= provide_times) {

         income = consumption * provide_times / static_cast<double>(service_times);
      }

      return income;
   };

   for (const auto& service_id : provider_itr->services) {
      std::tie(consumption, month_consumption) = get_consumption(service_id);
      std::tie(service_times, service_month_times, provide_times, provide_month_times) = get_times(service_id, account);

      std::tie(stake_freeze_amount, service_stake_freeze_amount) = get_freeze_stat(service_id, account);

      income += calc_income(service_times, provide_times, consumption * 0.8);

      month_income += calc_income(service_month_times, provide_month_times, month_consumption * 0.8);

      uint64_t stake_income = (consumption + month_consumption) * 0.8;
      stake_freeze_income = 0;
      if (stake_freeze_amount.amount > 0 && service_stake_freeze_amount.amount >= stake_freeze_amount.amount) {
         stake_freeze_income = stake_income * stake_freeze_amount.amount / static_cast<double>(service_stake_freeze_amount.amount);
      }
   }
   asset new_income = asset(income + month_income + stake_freeze_income, core_symbol()) - provider_itr->claim_amount;

   check(new_income.amount > 0, "no income ");

   providertable.modify(provider_itr, same_payer, [&](auto& p) { p.claim_amount += new_income; });

   transfer(_self, receive_account, new_income, "claim");
}

/**
 * @brief
 *
 * @param service_id
 * @param action_type
 */
void bos_oracle::execaction(uint64_t service_id, uint8_t action_type) {
   require_auth(_self);
   check(service_status::service_freeze == action_type || service_status::service_emergency == action_type, "unknown action type,support action:freeze(5),emergency(6)");
   data_services svctable(_self, _self.value);
   auto service_itr = svctable.find(service_id);
   check(service_itr != svctable.end(), "service does not exist");
   svctable.modify(service_itr, _self, [&](auto& s) { s.status = action_type; });
}

/**
 * @brief
 *
 * @param service_id
 * @param signature
 * @param account
 * @param status
 */
void bos_oracle::unregservice(uint64_t service_id, name account, uint8_t status) {
   require_auth(account);

   check(provision_status::provision_unreg == status || provision_status::provision_suspend == status, "unknown status,support provision_unreg(1),provision_suspend(2)");

   data_service_provisions provisionstable(_self, service_id);
   auto provision_itr = provisionstable.find(account.value);
   check(provision_itr != provisionstable.end(), "provider does not subscribe service ");

   provisionstable.modify(provision_itr, same_payer, [&](auto& p) { p.status = status; });

   if (static_cast<uint64_t>(service_status::service_cancel) == status) {
      svc_provision_cancel_applys applytable(_self, _self.value);
      applytable.emplace(_self, [&](auto& a) {
         a.apply_id = applytable.available_primary_key();
         a.service_id = service_id;
         a.provider = account;
         a.status = apply_status::apply_init;
         a.update_time = bos_oracle::current_time_point_sec();
      });

      data_providers providersstable(_self, _self.value);

      auto provider_itr = providersstable.find(account.value);
      check(provider_itr != providersstable.end(), "provider does not provision service");
      auto id_itr = std::find(provider_itr->services.begin(), provider_itr->services.end(), service_id);
      if (id_itr != provider_itr->services.end()) {
         providersstable.modify(provider_itr, same_payer, [&](auto& p) { p.services.erase(id_itr); });
      }

      asset freeze_amount = asset(0, core_symbol()); /// calculates  unfinish code
      check(asset(0, core_symbol()) == freeze_amount, "freeze amount must equal zero");

      // asset amount = asset(0, core_symbol());
      // transfer(_self, account, amount, "");
   }
}

/**
 * @brief
 *
 * @param service_id
 * @param contract_account
 * @param amount
 */
void bos_oracle::starttimer(uint64_t service_id, uint64_t update_number, uint64_t request_id) {
   require_auth(_self);
   print("\n======starttimer===========update_number=", update_number, ",request_id=", request_id);
   uint128_t id = make_update_id(update_number, request_id);

   oracle_data oracledatatable(_self, service_id);

   auto update_id_idx = oracledatatable.get_index<"bynumber"_n>(); //
   auto itr = update_id_idx.find(id);

   if (itr != update_id_idx.end()) {
      return;
   }

   check_publish_service(service_id, update_number, request_id, true);
}

void bos_oracle::start_timer(uint64_t service_id, uint64_t update_number, uint64_t request_id) {

   auto get_time = [&](uint64_t service_id, uint64_t id) -> uint32_t {
      data_service_requests reqtable(_self, service_id);
      auto req_itr = reqtable.find(id);
      check(req_itr != reqtable.end(), "request id could not be found");
      if (req_itr->status != request_status::reqeust_valid) {
         return 0;
      }

      uint32_t end_time_sec = (req_itr->request_time + eosio::hours(request_time_deadline)).sec_since_epoch();
      uint32_t now_sec = bos_oracle::current_time_point_sec().sec_since_epoch();
      if (end_time_sec > now_sec) {
         return end_time_sec - now_sec;
      }

      return 0;
   };

   auto get_update_number_time = [&](uint64_t service_id, uint64_t update_number) -> uint32_t {
      data_services svctable(_self, _self.value);
      auto service_itr = svctable.find(service_id);
      check(service_itr != svctable.end(), "service does not exist");

      uint32_t now_sec = bos_oracle::current_time_point_sec().sec_since_epoch();
      // uint32_t update_number = (now_sec - s.update_start_time.sec_since_epoch()) / s.update_cycle + 1;
      uint32_t current_data_collection_duration_end_time = service_itr->update_start_time.sec_since_epoch() + (update_number - 1) * service_itr->update_cycle + service_itr->duration;
      if (current_data_collection_duration_end_time > now_sec) {
         return current_data_collection_duration_end_time - now_sec;
      }

      return 0;
   };

   uint32_t delay_sec = 0;
   if (0 != request_id) {
      delay_sec = get_time(service_id, request_id);
   } else {
      delay_sec = get_update_number_time(service_id, update_number);
   }

   if (0 == delay_sec) {
      return;
   }

   transaction t;
   t.actions.emplace_back(permission_level{_self, active_permission}, _self, "starttimer"_n, std::make_tuple(service_id, update_number, request_id));
   t.delay_sec = delay_sec; // seconds
   uint128_t deferred_id = uint128_t(service_id) << 64 | (update_number | request_id);
   cancel_deferred(deferred_id);
   t.send(deferred_id, _self);
}

void bos_oracle::check_publish_service(uint64_t service_id, uint64_t update_number, uint64_t request_id, bool is_expired) {
   print("check_publish_service");

   data_services svctable(_self, _self.value);
   auto service_itr = svctable.find(service_id);
   check(service_itr != svctable.end(), "service does not exist");

   uint64_t pc = get_provider_count(service_id);
   uint64_t ppc = get_publish_provider_count(service_id, update_number, request_id);

   if (!is_expired) {
      if (pc < service_itr->provider_limit || ppc < pc) {
         return;
      }

      uint128_t deferred_id = uint128_t(service_id) << 64 | (update_number | request_id);
      cancel_deferred(deferred_id);
   } else if (pc < service_itr->provider_limit || ppc < service_itr->provider_limit) {
      update_service_current_log_status(service_id, update_number, request_id, service_itr->data_type, log_status::log_fail);
      return;
   }

   string data_json = get_publish_data(service_id, update_number, service_itr->provider_limit, request_id);
   if (data_json.empty()) {
      if (is_expired) {
         update_service_current_log_status(service_id, update_number, request_id, service_itr->data_type, log_status::log_fail);
      }

      return;
   }

   if (injection_method::chain_indirect == service_itr->injection_method) {
      save_publish_data(service_id, update_number, request_id, data_json);
   } else {
      innerpush(service_id, name{}, update_number, request_id, data_json);
   }
}

uint128_t bos_oracle::make_update_id(uint64_t update_number, uint64_t request_id) { return (uint128_t(request_id) << 64) | update_number; }

void bos_oracle::save_publish_data(uint64_t service_id, uint64_t update_number, uint64_t request_id, string data, uint8_t data_type) {

   uint128_t id = make_update_id(update_number, request_id);

   oracle_data oracledatatable(_self, service_id);

   auto update_id_idx = oracledatatable.get_index<"bynumber"_n>(); //
   auto itr = update_id_idx.find(id);

   if (data_deterministic == data_type && itr != update_id_idx.end()) {
      return;
   }

   update_service_current_log_status(service_id, update_number, request_id, data_type);

   oracledatatable.emplace(_self, [&](auto& d) {
      d.record_id = oracledatatable.available_primary_key();
      d.request_id = request_id;
      d.update_number = update_number;
      d.data = data;
      d.timestamp = bos_oracle::current_time_point_sec().sec_since_epoch();
   });
}

uint64_t bos_oracle::get_provider_count(uint64_t service_id) {

   data_service_provisions provisionstable(_self, service_id);

   uint64_t providers_count = 0;

   for (const auto& p : provisionstable) {
      if (p.status == provision_status::provision_reg && p.amount.amount - p.freeze_amount.amount > 0) {
         providers_count++;
      }
   }

   return providers_count;
}

uint64_t bos_oracle::get_publish_provider_count(uint64_t service_id, uint64_t update_number, uint64_t request_id) {

   data_service_provision_logs logtable(_self, service_id);
   auto update_number_idx = logtable.get_index<"bynumber"_n>(); //

   uint128_t id = make_update_id(update_number, request_id);

   auto update_number_itr_lower = update_number_idx.lower_bound(id);
   auto update_number_itr_upper = update_number_idx.upper_bound(id);

   uint64_t provider_count = 0;

   for (auto itr = update_number_itr_lower; itr != update_number_idx.end() && itr != update_number_itr_upper; ++itr) {
      provider_count++;
   }

   return provider_count;
}

bool bos_oracle::check_provider_no_push_data(uint64_t service_id, name provider, uint64_t update_number, uint64_t request_id, uint8_t data_type) {

   data_service_provision_logs logtable(_self, service_id);
   auto update_number_idx = logtable.get_index<"bynumber"_n>();

   uint128_t id = make_update_id(update_number, request_id);

   if (data_deterministic == data_type && update_number_idx.find(id) == update_number_idx.end()) {
      start_timer(service_id, update_number, request_id);
   }

   auto update_number_itr_lower = update_number_idx.lower_bound(id);
   auto update_number_itr_upper = update_number_idx.upper_bound(id);

   for (auto itr = update_number_itr_lower; itr != update_number_itr_upper; ++itr) {
      if (itr->account == provider) {
         print("check_provider_no_push_data u=", update_number, "r=", request_id, "id=", id, "p=", provider.to_string());
         return false;
      }
   }

   return true;
}

string bos_oracle::get_publish_data(uint64_t service_id, uint64_t update_number, uint8_t provider_limit, uint64_t request_id) {

   if (provider_limit < 3) {
      print("provider limit could not be less than 3  in get_publish_data");
      return "";
   }

   data_service_provision_logs logtable(_self, service_id);
   auto update_number_idx = logtable.get_index<"bynumber"_n>(); //

   std::map<string, int64_t> data_count;
   uint64_t provider_count = 0;

   uint128_t id = make_update_id(update_number, request_id);

   auto update_number_itr_lower = update_number_idx.lower_bound(id);
   auto update_number_itr_upper = update_number_idx.upper_bound(id);

   for (auto itr = update_number_itr_lower; itr != update_number_itr_upper; ++itr) {
      auto it = data_count.find(itr->data_json);
      if (it != data_count.end()) {
         data_count[itr->data_json]++;
      } else {
         data_count[itr->data_json] = one_time;
      }

      provider_count++;
   }

   std::string result = "";
   if (provider_count >= provider_limit && data_count.size() == one_time) {
      result = data_count.begin()->first;
   } else if (provider_count >= provider_limit) {
      for (auto& d : data_count) {
         if (d.second >= provider_count / 2 + 1) {
            result = data_count.begin()->first;
            print("get data that is greater than one half of providers  ");
            break;
         }
      }

      print("provider's data is not the same");
   } else {
      print("provider's count is less than provider's limit");
   }

   return result;
}

void bos_oracle::check_service_current_update_number(uint64_t service_id, uint64_t update_number) {

   data_services svctable(_self, _self.value);
   auto service_itr = svctable.find(service_id);
   check(service_itr != svctable.end(), "service does not exist");
   check(service_itr->update_cycle > 0, "update_cycle should be greater than 0");
   check(update_number > service_itr->last_update_number, "update_number should be greater than last_number of the service");

   uint32_t now_sec = bos_oracle::current_time_point_sec().sec_since_epoch();
   uint32_t update_start_time = service_itr->update_start_time.sec_since_epoch();
   check(now_sec >= update_start_time, "current time  should be greater than update_start_time");
   uint32_t update_cycle = service_itr->update_cycle;
   uint32_t duration = service_itr->duration;
   uint32_t expected_update_number = (now_sec - update_start_time) / update_cycle + 1;
   uint32_t current_duration_begin_time = time_point_sec(update_start_time + (expected_update_number - 1) * update_cycle).sec_since_epoch();
   uint32_t current_duration_end_time = current_duration_begin_time + duration;

   std::string checkmsg = "wrong update number,expected_update_number=" + std::to_string(expected_update_number) +
                          " or current time is not in timespan of data collection,begin_time=" + std::to_string(current_duration_begin_time) +
                          ",end_time=" + std::to_string(current_duration_end_time) + ",now=" + std::to_string(now_sec) + ",start=" + std::to_string(update_start_time);

   check(update_number == expected_update_number && now_sec >= current_duration_begin_time && now_sec <= current_duration_end_time, checkmsg.c_str());
}

void bos_oracle::update_service_current_log_status(uint64_t service_id, uint64_t update_number, uint64_t request_id, uint8_t data_type, uint8_t status) {

   print("\n  update_service_current_log_status in last update number=", update_number, ",data_type=", data_type, ",status=", status);

   auto is_push_finish = [&]() -> bool {
      uint64_t pc = get_provider_count(service_id);
      uint64_t ppc = get_publish_provider_count(service_id, update_number, request_id);
      return ppc == pc;
   };

   data_services svctable(_self, _self.value);

   uint128_t id = make_update_id(update_number, request_id);
   if (0 != update_number && (data_deterministic == data_type || log_status::log_fail == status || is_push_finish())) {
      auto service_itr = svctable.find(service_id);
      check(service_itr != svctable.end(), "service does not exist");
      print("\n  update_service_current_log_status last update number", update_number, "data_type=", data_type);
      svctable.modify(service_itr, _self, [&](auto& ss) { ss.last_update_number = update_number; });
   }

   data_service_provision_logs logtable(_self, service_id);

   auto update_number_idx = logtable.get_index<"bynumber"_n>(); //
   auto update_number_itr_lower = update_number_idx.lower_bound(id);
   auto update_number_itr_upper = update_number_idx.upper_bound(id);

   std::vector<uint64_t> ids;
   for (auto itr = update_number_itr_lower; itr != update_number_itr_upper; ++itr) {
      ids.push_back(itr->log_id);
   }

   for (auto& id : ids) {
      auto itr = logtable.find(id);
      if (itr != logtable.end()) {
         print("\n  update_service_current_log_status modify last update number=", update_number, ",data_type=", data_type, ",status=", status);

         logtable.modify(itr, _self, [&](auto& l) { l.status = status; });
      }
   }
}

// } // namespace bosoracle