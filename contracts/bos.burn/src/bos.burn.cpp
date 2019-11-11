#include <eosio/eosio.hpp>
#include "bos.burn.hpp"
#include "eosio.token.hpp"
#include <eosio/action.hpp>
#include <eosio/transaction.hpp>

using namespace eosio;

void bos_burn::importacnts(std::vector<std::pair<name,asset>> unactivated_airdrop_accounts) {
   require_auth(_self);

   auto unactivated_airdrop_account_table = accounts(get_self(), get_self().value);
   std::string checkmsg = "";
   for (auto& account : unactivated_airdrop_accounts) {
      checkmsg = account.first.to_string()+" account does not exist";
      check(is_account(account.first), checkmsg.c_str());
      check(account.second.is_valid(), "invalid quantity");
      check(account.second.amount > 0, "must transfer positive quantity");
      check(account.second.symbol == core_symbol(), "symbol precision mismatch");

      auto itr = unactivated_airdrop_account_table.find(account.first.value);
      checkmsg = account.first.to_string()+" account already added";
      check(itr == unactivated_airdrop_account_table.end(),checkmsg.c_str());

      unactivated_airdrop_account_table.emplace(get_self(), [&](auto& a) {
         a.account = account.first;
         a.quantity = account.second;
         a.is_burned = 0;
      });
   }
}

void bos_burn::clear() {
   require_auth(_self);
    auto unactivated_airdrop_account_table = accounts(get_self(), get_self().value);

   for (auto itr = unactivated_airdrop_account_table.begin(); itr != unactivated_airdrop_account_table.end();) {
      itr = unactivated_airdrop_account_table.erase(itr);
   }
}

void bos_burn::setparameter(uint8_t version,name executer) {
   require_auth(_self);
   check(is_account(executer), "executer account does not exist");

   std::string checkmsg = "unsupported version for setparameter action,current_version=" + std::to_string(current_version);
   check(version == current_version, checkmsg.c_str());
   _meta_parameters.version = version;
   _meta_parameters.executer = executer;
}

void bos_burn::burnhole(asset quantity) {
   require_auth(_self);
   name account="hole.bos"_n;
   std::string memo = "";
   action(permission_level{_self, "active"_n}, token_account, "burn"_n, std::make_tuple(_self,account, quantity,memo)).send();

}


void bos_burn::burns(name account) {
   check(_meta_parameters.version == current_version, "config executer parameters must first be initialized ");
   require_auth(_meta_parameters.executer);
   std::string checkmsg = account.to_string() + " Account does not exist";
   check(is_account(account), checkmsg.c_str());
   auto unactivated_airdrop_account_table = accounts(get_self(), get_self().value);
   auto itr = unactivated_airdrop_account_table.find(account.value);
   check(itr != unactivated_airdrop_account_table.end(), "Account is not on list ");
   check(!itr->is_burned, "The airdrop tokens of the account are burned ");

   std::string memo = "";
   uint8_t token_burned = 1;
   const uint8_t token_burned_failed_insufficient = 2;

   asset zero_asset(0, core_symbol());
   asset staked_asset(2000, core_symbol());
   auto available_balance = eosio::token::get_balance("eosio.token"_n, account, core_symbol().code());
   auto total_quantity = itr->quantity + staked_asset;
   user_resources_table totals_tbl("eosio"_n, account.value);
   auto tot_itr = totals_tbl.find(account.value);
   asset available_unstake_net_weight = tot_itr->net_weight;
   auto unstake_available_quantity = [&](asset available_quantity, asset available_balance, asset quantity) -> asset {
      if (available_quantity <= zero_asset) {
         return zero_asset;
      }

      if (available_balance + available_quantity < quantity) {
         return available_quantity;
      }

      return quantity - available_balance;
   };

   asset unstake_net_quantity = unstake_available_quantity(available_unstake_net_weight, available_balance, total_quantity);
   asset unstake_cpu_quantity = unstake_available_quantity(tot_itr->cpu_weight, available_balance + unstake_net_quantity, total_quantity);
   print("\nunstake_net_quantity=",unstake_net_quantity.amount,";unstake_cpu_quantity=",unstake_cpu_quantity.amount);
   name receiver = account;
   action(permission_level{_meta_parameters.executer, "active"_n}, "eosio"_n, "undelegatebs"_n,
          std::make_tuple(_meta_parameters.executer, account, receiver, unstake_net_quantity, unstake_cpu_quantity))
       .send();

   auto new_balance = eosio::token::get_balance("eosio.token"_n, account, core_symbol().code());

   if (new_balance.amount >= total_quantity.amount) {
      action(permission_level{_meta_parameters.executer, "active"_n}, token_account, "burn"_n, std::make_tuple(_meta_parameters.executer, account, total_quantity, memo)).send();
   } else {
      token_burned = token_burned_failed_insufficient;
   }

   unactivated_airdrop_account_table.modify(itr, same_payer, [&](auto& a) { a.is_burned = token_burned; });
}


void bos_burn::burn(name account) {
   check(_meta_parameters.version == current_version, "config executer parameters must first be initialized ");
   action(permission_level{_meta_parameters.executer, "active"_n}, _self, "burns"_n, std::make_tuple(account)).send();
}


EOSIO_DISPATCH(bos_burn,
 (importacnts)(setparameter)(burnhole)(burns)(burn)(clear)
)

