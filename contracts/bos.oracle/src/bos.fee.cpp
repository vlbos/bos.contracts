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
 * @param account
 * @param contract_account
 * @param is_request
 */
void bos_oracle::add_times(uint64_t service_id, name account, name contract_account, bool is_request) {

  
   auto add_time = [is_request](uint64_t& times, uint64_t& month_times, bool is_new = false) {
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

   push_records pushtable(_self, _self.value);
   auto push_itr = pushtable.find(service_id);
   if (push_itr == pushtable.end()) {
      pushtable.emplace(_self, [&](auto& p) {
        p.service_id = service_id;
         add_time(p.times, p.month_times, true);
      });
   } else {
      pushtable.modify(push_itr, same_payer, [&](auto& p) {
         add_time(p.times, p.month_times);
      });
   }

   provider_push_records providetable(_self, service_id);
   auto provide_itr = providetable.find(account.value);
   if (provide_itr == providetable.end()) {
      providetable.emplace(_self, [&](auto& p) {
         p.account = account;
         add_time(p.times, p.month_times, true);
      });
   } else {
      providetable.modify(provide_itr, same_payer, [&](auto& p) { add_time(p.times, p.month_times); });
   }
}

std::tuple<uint64_t, uint64_t, uint64_t, uint64_t> bos_oracle::get_times(uint64_t service_id, name account) {

   push_records pushtable(_self, _self.value);
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

   return std::make_tuple(service_times, service_month_times, provide_times, provide_month_times);
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
   check(fee_type >= fee_type::fee_times && fee_type < fee_type::fee_type_count, "unknown fee type");

   auto fee_itr = feetable.find(fee_type);
   check(fee_itr != feetable.end(), " service's fee type does not found");

   return fee_itr->service_price;
}

/**
 * @brief
 *
 * @param service_id
 * @param contract_account
 * @param fee_type
 */
void bos_oracle::fee_service(uint64_t service_id, name contract_account, uint8_t fee_type) {
   asset price_by_times = get_price_by_fee_type(service_id, fee_type);

   check(price_by_times.amount > 0, " get price by times cound not be greater than zero");

   data_service_subscriptions subscriptionstable(_self, service_id);

   auto subscriptions_itr = subscriptionstable.find(contract_account.value);
   check(subscriptions_itr != subscriptionstable.end(), "contract_account does not exist");

   check(subscriptions_itr->balance >= price_by_times, "balance cound not be  greater than price by times");

   subscriptionstable.modify(subscriptions_itr, _self, [&](auto& subs) {
      if (fee_type::fee_times == fee_type) {
         subs.consumption += price_by_times;
      } else {
         subs.month_consumption += price_by_times;
         subs.last_payment_time = bos_oracle::current_time_point_sec();
      }
      subs.balance = subs.payment - subs.consumption - subs.month_consumption;
   });

   service_consumptions consumptionstable(_self, service_id);
   auto consumptions_itr = consumptionstable.find(service_id);
   if (consumptions_itr == consumptionstable.end()) {
      consumptionstable.emplace(_self, [&](auto& c) {
         c.service_id = service_id;
         if (fee_type::fee_times == fee_type) {
            c.consumption = price_by_times;
            c.month_consumption = asset(0, core_symbol());
         } else {
            c.consumption = asset(0, core_symbol());
            c.month_consumption = price_by_times;
         }
         c.update_time = bos_oracle::current_time_point_sec();
      });
   } else {
      consumptionstable.modify(consumptions_itr, same_payer, [&](auto& c) {
         if (fee_type::fee_times == fee_type) {
            c.consumption.print();
            price_by_times.print();
            c.consumption += price_by_times;
         } else {
            c.month_consumption += price_by_times;
         }
         c.update_time = bos_oracle::current_time_point_sec();
      });
   }
}

/**
 * @brief
 *
 * @param service_id
 * @param contract_account
 * @return uint8_t
 */
uint8_t bos_oracle::get_subscription_status(uint64_t service_id, name contract_account) {

   data_service_subscriptions subscriptionstable(_self, service_id);

   auto subscriptions_itr = subscriptionstable.find(contract_account.value);
   check(subscriptions_itr != subscriptionstable.end(), "contract_account does not exist");

   return subscriptions_itr->status;
}

/**
 * @brief
 *
 * @param service_id
 * @param contract_account
 * @return time_point_sec
 */
time_point_sec bos_oracle::get_payment_time(uint64_t service_id, name contract_account) {

   data_service_subscriptions subscriptionstable(_self, service_id);

   auto subscriptions_itr = subscriptionstable.find(contract_account.value);
   check(subscriptions_itr != subscriptionstable.end(), "contract_account does not exist");

   return subscriptions_itr->last_payment_time;
}

/**
 * @brief
 *
 * @param service_id
 * @return std::vector<std::tuple<name, name>>
 */
std::vector<name> bos_oracle::get_subscription_list(uint64_t service_id) {

   data_service_subscriptions subscriptionstable(_self, service_id);
   auto subscription_time_idx = subscriptionstable.get_index<"bytime"_n>();
   std::vector<name> receive_contracts;

   for (const auto& s : subscription_time_idx) {
      if (s.status == subscription_status::subscription_subscribe) {
         receive_contracts.push_back(s.contract_account);
      }
   }

   return receive_contracts;
}


/**
 * @brief
 *
 * @param service_id
 * @return std::tuple<uint64_t, uint64_t>
 */
std::tuple<uint64_t, uint64_t> bos_oracle::get_consumption(uint64_t service_id) {

   data_service_subscriptions subscriptionstable(_self, service_id);

   service_consumptions consumptionstable(_self, service_id);
   auto service_itr = consumptionstable.find(service_id);
   check(service_itr != consumptionstable.end(), "service could not be found");
   uint64_t consumptions = service_itr->consumption.amount;
   uint64_t month_consumptions = service_itr->month_consumption.amount;

   if (service_itr->update_time+unpack<oracle_parameters>(_oracle_meta_parameters.parameters_data).time_deadline < bos_oracle::current_time_point_sec() ) {
      consumptions = 0;
      month_consumptions = 0;
      for (const auto& s : subscriptionstable) {
         consumptions += s.consumption.amount;
         month_consumptions += s.month_consumption.amount;
      }

      consumptionstable.modify(service_itr, _self, [&](auto& consump) {
         consump.consumption = asset(consumptions, core_symbol());
         consump.month_consumption = asset(month_consumptions, core_symbol());
         consump.update_time = bos_oracle::current_time_point_sec();
      });
   }

   return std::make_tuple(consumptions, month_consumptions);
}

std::vector<std::tuple<name, asset>> bos_oracle::get_provider_list(uint64_t service_id) {

   data_service_provisions provisionstable(_self, service_id);

   std::vector<std::tuple<name, asset>> providers;

   for (const auto& p : provisionstable) {
      if (p.status == provision_status::provision_reg && p.amount.amount - p.freeze_amount.amount > 0) {
         providers.push_back(std::make_tuple(p.account, p.amount - p.freeze_amount));
      }
   }

   return providers;
}

// } // namespace bosoracle