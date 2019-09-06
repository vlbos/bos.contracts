
#include "bos.oracle/bos.oracle.hpp"
#include <eosio/action.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
// #include <eosio/print.h>
#include "bos.oracle/bos.util.hpp"
#include <eosio/time.hpp>
#include <eosio/transaction.hpp>

void bos_oracle::on_transfer(name from, name to, asset quantity, string memo) {
   //  check(get_first_receiver() == "eosio.token"_n, "should be eosio.token");
   print_f("On notify : % % % %", from, to, quantity, memo);
   if (from == _self || memo.empty()) {
      // print("memo is empty on trasfer");
      return;
   }

   //  check(quantity.amount>100, "amount could not be less than 100");
   //
   // check(find(to) != end, "no account's business found ");

   std::vector<std::string> parameters = bos_util::get_parameters(memo);
   check(parameters.size() > 0, "parse memo failed ");
   uint64_t transfer_category = bos_util::convert_to_int(parameters[index_category]);

   auto check_parameters_size = [&](uint64_t category) -> bool {
      std::vector<uint8_t> index_counts = {arbitrator_count, arbitrator_count, deposit_count, appeal_count, arbitrator_count, resp_case_count, risk_guarantee_case_count};
      std::vector<std::string> help_strings = {"stake_category,index_id",
                                               "pay_category,index_id",
                                               "deposit_category,from,to,notify ",
                                               "appeal_category,service_id ,evidence,info,reason,provider",
                                               "arbitrator_category,type ",
                                               "resp_case_category,arbitration_id,evidence",
                                               "risk_guarantee_category,id,duration"};

      // check(category >= 0 && category < index_counts.size(), "unknown category");
      // check(parameters.size() == index_counts[category],
      //       "the parameters'size does not match ");

      if (category >= 0 && category < index_counts.size()) {
         if (parameters.size() == index_counts[category]) {
            return true;
         }
         std::string str = "the parameters'size does not match " + help_strings[category];
         check(false, str.c_str());
      }

      return false;
   };

   if (!check_parameters_size(transfer_category)) {
      print("unknown category");
      return;
   }

   auto s2name = [&](uint64_t index) -> name {
      if (index >= 0 && index < parameters.size()) {
         return name(parameters[index]);
      } else {
         print("index invalid", index, parameters.size());
      }

      return name{};
   };
   auto s2int = [&](uint64_t index) -> uint64_t {
      if (index >= 0 && index < parameters.size()) {
         return bos_util::convert_to_int(parameters[index]);
      } else {
         print("index invalid", index, parameters.size());
      }
      return 0;
   };

   if (tc_deposit == transfer_category) {
      call_deposit(s2name(static_cast<uint64_t>(index_from)), s2name(index_to), quantity, 0 != s2int(index_notify));
      // transfer(_self, riskctrl_account, quantity, memo);
   } else {

      name account = from;
      switch (transfer_category) {
      case tc_service_stake:
         stake_asset(s2int(index_id), account, quantity, memo);
         // oracle_transfer(_self, provider_account, quantity, memo, true);
         break;
      case tc_pay_service:
         pay_service(s2int(index_id), account, quantity);
         // oracle_transfer(_self, consumer_account, quantity, memo, true);
         break;
      case tc_arbitration_stake_appeal:
         _appeal(account, s2int(index_id), quantity, parameters[index_reason], parameters[index_evidence], s2int(index_provider));
         // oracle_transfer(_self, arbitrat_account, quantity, memo, true);
         break;
      case tc_arbitration_stake_arbitrator:
         _regarbitrat(account, s2int(index_type), quantity, "");
         // oracle_transfer(_self, arbitrat_account, quantity, memo, true);
         break;
      case tc_arbitration_stake_resp_case:
         _respcase(account, s2int(index_id), quantity, parameters[index_evidence]);
         // oracle_transfer(_self, arbitrat_account, quantity, memo, true);
         break;
      case tc_risk_guarantee:
         add_guarantee(s2int(index_id), account, quantity, s2int(index_duration));
         // oracle_transfer(_self, arbitrat_account, quantity, memo, true);
         break;
      default:
         //  check(false, "unknown  transfer category ");
         print("unknown  transfer category ");
         break;
      }
   }
}

/**
 * @brief
 *
 * @param from
 * @param to
 * @param quantity
 * @param memo
 */
void bos_oracle::transfer(name from, name to, asset quantity, string memo) { oracle_transfer(from, to, quantity, memo, false); }

void bos_oracle::oracle_transfer(name from, name to, asset quantity, string memo, bool is_deferred) {
   check(from != to, "cannot transfer to self");
   //  require_auth( from );
   check(is_account(to), "to account does not exist");
   //  auto sym = quantity.symbol.code();
   //  stats statstable( _self, sym.raw() );
   //  const auto& st = statstable.get( sym.raw() );

   //  require_recipient( from );
   //  require_recipient( to );

   check(quantity.is_valid(), "invalid quantity");
   check(quantity.amount > 0, "must transfer positive quantity");
   // check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
   check(memo.size() <= 256, "memo has more than 256 bytes");

   //   token::transfer_action transfer_act{ token_account, { account,
   //   active_permission } };
   //          transfer_act.send( account, consumer_account, amount, memo );

   //  auto payer = has_auth( to ) ? to : from;
   // print("===quantity");
   quantity.print();

   if (!is_deferred) {
      action(permission_level{from, "active"_n}, token_account, "transfer"_n, std::make_tuple(from, to, quantity, memo)).send();
   } else {
      transaction t;
      t.actions.emplace_back(permission_level{from, active_permission}, token_account, "transfer"_n, std::make_tuple(from, to, quantity, memo));
      t.delay_sec = 0;
      uint128_t deferred_id = (uint128_t(to.value) << 64) | bos_oracle::current_time_point_sec().sec_since_epoch();
      cancel_deferred(deferred_id);
      t.send(deferred_id, _self, true);
   }

   // INLINE_ACTION_SENDER(eosio::token, transfer)(token_account, {{from,
   // active_permission}, {to, active_permission}},{from, to, quantity, memo});
}

/// from dapp user to dapp
void bos_oracle::deposit(name from, name to, asset quantity, string memo, bool is_notify) {
   require_auth(_self);
   call_deposit(from, to, quantity, is_notify);
}

void bos_oracle::call_deposit(name from, name to, asset quantity, bool is_notify) {
   // transfer(from, to, quantity, memo);
   // auto payer = has_auth(to) ? to : from;
   add_balance(to, quantity, _self);

   if (is_notify) {
      // notify dapp
      // require_recipient( to );
   }
}

/// from dapp to dapp user
/**
 * @brief
 *
 * @param from
 * @param to
 * @param quantity
 * @param memo
 */
void bos_oracle::withdraw(uint64_t service_id, name from, name to, asset quantity, string memo) {
   require_auth(_self);
   sub_balance(from, quantity);

   // find service
   data_service_subscriptions svcsubstable(_self, service_id);
   auto acc_idx = svcsubstable.get_index<"byaccount"_n>();
   auto svcsubs_itr = acc_idx.find(from.value);
   check(svcsubs_itr != acc_idx.end(), "account does not subscribe services");

   // find stake
   data_service_stakes svcstaketable(_self, _self.value);
   auto svcstake_itr = svcstaketable.find(svcsubs_itr->service_id);
   check(svcstake_itr != svcstaketable.end(), " no service stake  found");

   uint32_t time_length = 1;
   if (svcstake_itr->amount - svcstake_itr->freeze_amount >= quantity) {
      svcstaketable.modify(svcstake_itr, same_payer, [&](auto& ss) { ss.freeze_amount += quantity; });

      add_freeze(svcsubs_itr->service_id, from, bos_oracle::current_time_point_sec(), time_length, quantity);
   } else {

      add_delay(svcsubs_itr->service_id, from, bos_oracle::current_time_point_sec(), time_length, quantity);
   }
}

/**
 * @brief
 *
 * @param service_id
 * @param account
 * @param start_time
 * @param duration
 * @param amount
 * @param status
 * @param type
 */
void bos_oracle::add_freeze_delay(uint64_t service_id, name account, time_point_sec start_time, uint32_t duration, asset amount, uint64_t type) {

   transfer_freeze_delays transfertable(_self, service_id);

   transfertable.emplace(_self, [&](auto& t) {
      t.transfer_id = transfertable.available_primary_key();
      t.service_id = service_id;
      t.account = account;
      t.start_time = start_time;
      t.duration = duration;
      t.amount = amount;
      t.status = transfer_status::transfer_start;
      t.type = type;
   });
}

void bos_oracle::add_freeze(uint64_t service_id, name account, time_point_sec start_time, uint32_t duration, asset amount) {

   std::vector<std::tuple<name, asset>> providers = get_provider_list(service_id);

   check(providers.size() > 0, " no provider found when add freeze");

   uint64_t average_amount = amount.amount / providers.size();
   uint64_t unfreeze_amount = 0;
   uint64_t real_freeze_amount = 0;

   std::set<name> finish_providers;
   for (const auto& p : providers) {
      real_freeze_amount = std::get<1>(p).amount;
      if (real_freeze_amount >= average_amount) {
         real_freeze_amount = average_amount;
         finish_providers.insert(std::get<0>(p));
      } else {
         unfreeze_amount += average_amount - real_freeze_amount;
      }

      freeze_asset(service_id, std::get<0>(p), asset(real_freeze_amount, core_symbol()));
   }

   // unfreeze_amount  freeze amount from provider who stake amount greater than
   // base stake amount.
   if (unfreeze_amount > 0 && finish_providers.size() > 0) {
      unfreeze_amount = freeze_providers_amount(service_id, finish_providers, asset(unfreeze_amount, core_symbol()));
   }

   add_freeze_delay(service_id, account, bos_oracle::current_time_point_sec(), duration, amount - asset(unfreeze_amount, core_symbol()), transfer_type::tt_freeze);

   if (unfreeze_amount > 0 && finish_providers.size() > 0) {
      // delay
      add_delay(service_id, account, bos_oracle::current_time_point_sec(), duration, asset(unfreeze_amount, core_symbol()));
   }
}

void bos_oracle::add_delay(uint64_t service_id, name account, time_point_sec start_time, uint32_t duration, asset amount) {

   add_freeze_delay(service_id, account, bos_oracle::current_time_point_sec(), duration, amount, transfer_type::tt_delay);
}

uint64_t bos_oracle::freeze_providers_amount(uint64_t service_id, const std::set<name>& available_providers, asset freeze_amount) {

   std::vector<std::tuple<name, asset>> providers = get_provider_list(service_id);

   check(providers.size() > 0, " no provider found in freeze providers amount");

   uint64_t average_amount = freeze_amount.amount / providers.size();
   uint64_t unfreeze_amount = 0;
   uint64_t real_freeze_amount = 0;

   std::set<name> finish_providers;
   for (const auto& p : providers) {
      name account = std::get<0>(p);
      if (available_providers.find(account) == available_providers.end()) {
         continue;
      }

      real_freeze_amount = std::get<1>(p).amount;
      if (real_freeze_amount >= average_amount) {
         real_freeze_amount = average_amount;
         finish_providers.insert(account);
      } else {
         print("\nunfreeze_amount", unfreeze_amount, ",average_amount=", average_amount, ",real_freeze_amount=", real_freeze_amount);
         unfreeze_amount += average_amount - real_freeze_amount;
      }

      freeze_asset(service_id, account, asset(real_freeze_amount, core_symbol()));
   }

   if (unfreeze_amount > 0 && finish_providers.size() > 0) {
      unfreeze_amount = freeze_providers_amount(service_id, finish_providers, asset(unfreeze_amount, core_symbol()));
   }

   return unfreeze_amount;
}

void bos_oracle::freeze_asset(uint64_t service_id, name account, asset amount) {

   data_providers providertable(_self, _self.value);
   auto provider_itr = providertable.find(account.value);
   check(provider_itr != providertable.end(), "no provider found in freeze asset");

   data_service_provisions provisionstable(_self, service_id);

   auto provision_itr = provisionstable.find(account.value);
   check(provision_itr != provisionstable.end(), "account does not subscribe the service");

   providertable.modify(provider_itr, same_payer, [&](auto& p) { p.total_freeze_amount += amount; });

   provisionstable.modify(provision_itr, same_payer, [&](auto& p) { p.freeze_amount += amount; });

   add_freeze_log(service_id, account, amount);

   update_service_provider_status(service_id, account);
}

void bos_oracle::add_freeze_log(uint64_t service_id, name account, asset amount) {
   account_freeze_logs freezelogtable(_self, service_id);

   freezelogtable.emplace(_self, [&](auto& t) {
      t.log_id = freezelogtable.available_primary_key();
      t.service_id = service_id;
      t.account = account;
      t.amount = amount;
      t.update_time = bos_oracle::current_time_point_sec();
   });

   add_freeze_stat(service_id, account, amount);
}

void bos_oracle::add_freeze_stat(uint64_t service_id, name account, asset amount) {
   account_freeze_stats freezestatstable(_self, service_id);
   auto freeze_stats = freezestatstable.find(account.value);
   if (freeze_stats == freezestatstable.end()) {
      freezestatstable.emplace(_self, [&](auto& f) {
         f.account = account;
         f.amount = amount;
      });
   } else {
      freezestatstable.modify(freeze_stats, same_payer, [&](auto& f) { f.amount += amount; });
   }

   service_freeze_stats svcfreezestatstable(_self, service_id);
   auto svcfreeze_stats = svcfreezestatstable.find(service_id);
   if (svcfreeze_stats == svcfreezestatstable.end()) {
      svcfreezestatstable.emplace(_self, [&](auto& s) {
         s.service_id = service_id;
         s.amount = amount;
      });
   } else {
      svcfreezestatstable.modify(svcfreeze_stats, same_payer, [&](auto& s) { s.amount += amount; });
   }
}

std::tuple<asset, asset> bos_oracle::get_freeze_stat(uint64_t service_id, name account) {
   account_freeze_stats freezestatstable(_self, service_id);
   auto freeze_stats = freezestatstable.find(account.value);
   check(freeze_stats != freezestatstable.end(), "");

   service_freeze_stats svcfreezestatstable(_self, service_id);
   auto svcfreeze_stats = svcfreezestatstable.find(service_id);
   check(svcfreeze_stats != svcfreezestatstable.end(), "");

   return std::make_tuple(freeze_stats->amount, svcfreeze_stats->amount);
}

/**
 * @brief
 *
 * @param service_id
 * @param account
 * @param start_time
 * @param duration
 * @param amount
 * @param status
 * @return uint64_t
 */
uint64_t bos_oracle::add_guarantee(uint64_t service_id, name account, asset amount, uint32_t duration) {
   risk_guarantees guaranteetable(_self, _self.value);

   uint64_t risk_id = 0;
   guaranteetable.emplace(_self, [&](auto& g) {
      g.risk_id = guaranteetable.available_primary_key();
      g.account = account;
      g.amount = amount;
      g.start_time = bos_oracle::current_time_point_sec();
      g.duration = duration;
      g.status = 0;
      //  g.sig = sig;
      risk_id = g.risk_id;
   });

   return risk_id;
}

/**
 * @brief
 *
 * @param owner
 * @param value
 */
void bos_oracle::sub_balance(name owner, asset value) {
   riskcontrol_accounts dapp_acnts(_self, owner.value);

   const auto& dapp = dapp_acnts.get(value.symbol.code().raw(), "no balance object found sub");
   check(dapp.balance.amount >= value.amount, "overdrawn balance");

   dapp_acnts.modify(dapp, same_payer, [&](auto& a) { a.balance -= value; });
}

void bos_oracle::add_balance(name owner, asset value, name ram_payer) {
   riskcontrol_accounts dapp_acnts(_self, owner.value);
   auto dapp = dapp_acnts.find(value.symbol.code().raw());
   if (dapp == dapp_acnts.end()) {
      dapp_acnts.emplace(ram_payer, [&](auto& a) { a.balance = value; });
   } else {
      dapp_acnts.modify(dapp, same_payer, [&](auto& a) { a.balance += value; });
   }
}
