#include "bos.burn.hpp"
#include "eosio.token.hpp"
#include <eosio/action.hpp>
#include <eosio/eosio.hpp>
#include <eosio/transaction.hpp>

using namespace eosio;

void bos_burn::importacnts(std::vector<std::pair<name, asset>> unactivated_airdrop_accounts) {
   require_auth(_self);

   std::string checkmsg = "";
   for (auto& account : unactivated_airdrop_accounts) {
      checkmsg = account.first.to_string() + " account does not exist";
      // check(is_account(account.first), checkmsg.c_str());
      if(!is_account(account.first))
      {
         print("\n",checkmsg);
         continue;
      }

      check(account.second.is_valid(), "invalid quantity");
      check(account.second.amount > 0, "must transfer positive quantity");
      check(account.second.symbol == core_symbol(), "symbol precision mismatch");
      auto unactivated_airdrop_account_table = accounts(get_self(), account.first.value);
      auto itr = unactivated_airdrop_account_table.find(account.first.value);
      checkmsg = account.first.to_string() + " account already added";
      check(itr == unactivated_airdrop_account_table.end(), checkmsg.c_str());

      unactivated_airdrop_account_table.emplace(get_self(), [&](auto& a) {
         a.account = account.first;
         a.quantity = account.second;
         a.is_burned = 0;
      });
   }
}

void bos_burn::clear(vector<name> clear_accounts) {
   require_auth(_self);

   bool erased_flag = false;
   for (name account : clear_accounts) {
      std::string checkmsg = account.to_string() + " Account does not exist";
      check(is_account(account), checkmsg.c_str());
      auto unactivated_airdrop_account_table = accounts(get_self(), account.value);
      auto itr = unactivated_airdrop_account_table.find(account.value);
      if (itr != unactivated_airdrop_account_table.end()) {
         unactivated_airdrop_account_table.erase(itr);
         erased_flag = true;
      }
   }

   check(erased_flag, "all clear accounts are not on list");
}

void bos_burn::setparameter(uint8_t version, name executer) {
   require_auth(_self);
   check(is_account(executer), "executer account does not exist");

   std::string checkmsg = "unsupported version for setparameter action,current_version=" + std::to_string(current_version);
   check(version == current_version, checkmsg.c_str());
   _meta_parameters.version = version;
   _meta_parameters.executer = executer;
}

void bos_burn::burn(asset quantity) {
   require_auth(_self);
   auto available_balance = eosio::token::get_balance("eosio.token"_n, hole_account, core_symbol().code());
   check(available_balance >= quantity, "available balance of hole.bos must be greater than  quantity of burning tokens");
   std::string memo = "";
   action(permission_level{_self, "active"_n}, token_account, "burn"_n, std::make_tuple(_self, hole_account, quantity, memo)).send();
}

void bos_burn::transferairs(name account) {
   check(_meta_parameters.version == current_version, "config executer parameters must first be initialized ");
   require_auth(_meta_parameters.executer);
   std::string checkmsg = account.to_string() + " Account does not exist";
   check(is_account(account), checkmsg.c_str());
   auto unactivated_airdrop_account_table = accounts(get_self(), account.value);
   auto itr = unactivated_airdrop_account_table.find(account.value);
   check(itr != unactivated_airdrop_account_table.end(), "Account is not on list ");
   check(!itr->is_burned, "The airdrop tokens of the account are burned ");

   std::string memo = "";
   const uint8_t token_burned = 1;
   const uint8_t token_burned_failed_insufficient = 2;

   asset zero_asset(0, core_symbol());
   asset one_asset(1, core_symbol());
   asset staked_asset(2000, core_symbol());
   auto available_balance = eosio::token::get_balance("eosio.token"_n, account, core_symbol().code());
   auto total_quantity = itr->quantity;
   user_resources_table totals_tbl("eosio"_n, account.value);
   auto tot_itr = totals_tbl.find(account.value);
   auto new_balance = available_balance + tot_itr->net_weight + tot_itr->cpu_weight; // eosio::token::get_balance("eosio.token"_n, account, core_symbol().code());
   if (new_balance < itr->quantity) {
      unactivated_airdrop_account_table.modify(itr, same_payer, [&](auto& a) { a.is_burned = token_burned_failed_insufficient; });
      print("\n burn failed, the balance of the account less than quantity of token from airdrop :", account);
      return;
   }

   if (available_balance==itr->quantity && new_balance == itr->quantity + staked_asset) {
      total_quantity += staked_asset;
   }

   auto unstake_available_quantity = [&](asset available_quantity, asset unstake_quantity) -> asset {
      print("\n available_quantity=", available_quantity.amount, ";quantity.amount=", itr->quantity.amount);
      if (unstake_quantity <= zero_asset) {
         return zero_asset;
      }

      if (available_quantity <= unstake_quantity) {
         return available_quantity;
      }

      return unstake_quantity;
   };

   asset to_unstake = (total_quantity - available_balance);
   asset to_unstake_half = to_unstake / 2;
   if (to_unstake.amount % 2 != 0) {
      to_unstake_half += one_asset;
   }
   asset unstake_net_quantity = unstake_available_quantity(tot_itr->net_weight, to_unstake_half);
   asset unstake_cpu_quantity = unstake_available_quantity(tot_itr->cpu_weight, to_unstake_half);
   name receiver = account;
   if (unstake_net_quantity + unstake_cpu_quantity > zero_asset) {
      action(permission_level{_meta_parameters.executer, "active"_n}, "eosio"_n, "undelegatebs"_n,
             std::make_tuple(_meta_parameters.executer, account, receiver, unstake_net_quantity, unstake_cpu_quantity))
          .send();
   }

   action(permission_level{_meta_parameters.executer, "active"_n}, token_account, "transferburn"_n, std::make_tuple(_meta_parameters.executer, account, hole_account, total_quantity, memo)).send();

   unactivated_airdrop_account_table.modify(itr, same_payer, [&](auto& a) { a.is_burned = token_burned; });
}

void bos_burn::transferair(name account) {
   check(_meta_parameters.version == current_version, "config executer parameters must first be initialized ");
   action(permission_level{_meta_parameters.executer, "active"_n}, _self, "transferairs"_n, std::make_tuple(account)).send();
}

EOSIO_DISPATCH(bos_burn, (importacnts)(setparameter)(burn)(transferairs)(transferair)(clear))
