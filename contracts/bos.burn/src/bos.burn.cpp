#include <eosio/eosio.hpp>
#include "bos.burn.hpp"
#include "eosio.token.hpp"
#include <eosio/action.hpp>

using namespace eosio;

void bos_burn::importacnts(std::vector<airdrop_unactive_account> burn_accounts) {
   require_auth(_self);

   auto burn_account_table = accounts(get_self(), get_self().value);
   std::string checkmsg = "";
   for (auto& account : burn_accounts) {
      checkmsg = account.account.to_string()+" account does not exist";
      check(is_account(account.account), checkmsg.c_str());
      check(account.quantity.is_valid(), "invalid quantity");
      check(account.quantity.amount > 0, "must transfer positive quantity");
      check(account.quantity.symbol == core_symbol(), "symbol precision mismatch");

      auto itr = burn_account_table.find(account.account.value);
      checkmsg = account.account.to_string()+" account already added";
      check(itr == burn_account_table.end(),checkmsg.c_str());

      burn_account_table.emplace(get_self(), [&](auto& a) {
         a.account = account.account;
         a.quantity = account.quantity;
         a.is_burned = 0;
      });
   }
}

void bos_burn::setparameter(uint8_t version,name executer) {
   require_auth(_self);
   check(is_account(executer), "executer account does not exist");

   std::string checkmsg = "unsupported version for setparameter action,current_oracle_version=" + std::to_string(current_oracle_version);
   check(version == current_oracle_version, checkmsg.c_str());
   _meta_parameters.version = version;
   _meta_parameters.executer = executer;
}


void bos_burn::burns(name account) {
   require_auth(_meta_parameters.executer);
   check(is_account(account), "account does not exist");
   auto burn_account_table = accounts(get_self(), get_self().value);
   auto itr = burn_account_table.find(account.value);
   check(itr != burn_account_table.end(), "account is not on list ");

   auto newBalance = eosio::token::get_balance("eosio.token"_n, account, core_symbol().code());
   if (newBalance.amount >= itr->quantity.amount) {
      action(permission_level{_meta_parameters.executer, "active"_n}, token_account, "burn"_n, std::make_tuple(account, itr->quantity)).send();
   } else {
      // name from,
      name receiver;
      asset unstake_net_quantity;
      asset unstake_cpu_quantity;

      action(permission_level{_meta_parameters.executer, "active"_n}, "eosio"_n, "undelegatebs"_n, std::make_tuple(account, receiver,unstake_net_quantity,unstake_cpu_quantity)).send();
      action(permission_level{_meta_parameters.executer, "active"_n}, token_account, "burn"_n, std::make_tuple(account, itr->quantity)).send();
   }
}


void bos_burn::burn(name account) {
   action(permission_level{_meta_parameters.executer, "active"_n}, _self, "burns"_n, std::make_tuple(account)).send();
}

EOSIO_DISPATCH(bos_burn,
 (importacnts)(setparameter)(burns)(burn)
)

