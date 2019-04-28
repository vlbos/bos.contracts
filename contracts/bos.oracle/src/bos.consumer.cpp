/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */

#include <eosiolib/asset.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/time.hpp>
#include <string>
#include "bos.oracle/bos.oracle.hpp"
// namespace eosio {

using eosio::asset;
using eosio::public_key;
using std::string;

void bos_oracle::subscribe(uint64_t service_id, name contract_account, name action_name,std::string publickey, name account, asset amount,std::string memo) {

  // //   token::transfer_action transfer_act{ token_account, { account,
  // active_permission } };
  // //          transfer_act.send( account, consumer_account, amount, memo );

  //       INLINE_ACTION_SENDER(eosio::token, transfer)(
  //          token_account, { {account, active_permission} },
  //          { account, consumer_account, amount, memo }
  //       );
  require_auth(account);
  require_auth(contract_account);
  transfer(account, consumer_account, amount, memo);

  // add consumer
  data_consumers consumertable(_self, _self.value);
  auto consumer_itr = consumertable.find(account.value);
  if (consumer_itr == consumertable.end()) {
    consumertable.emplace(_self, [&](auto &c) {
      c.account = account;
      // c.pubkey = publickey;
      c.status = 1;
      c.create_time = time_point_sec(now());
    });
  }

  // add consumer service subscription relation
  data_service_subscriptions substable(_self, _self.value);

  auto subs_itr = substable.find(contract_account.value);
  check(subs_itr == substable.end(), "contract_account exist");

  substable.emplace(_self, [&](auto &subs) {
    subs.service_id = service_id;
    subs.contract_account = contract_account;
    subs.action_name = action_name;
    subs.payment = amount;
    subs.consumption = asset(0,core_symbol());
  });
}

void bos_oracle::payservice(uint64_t service_id, name contract_account, name account,
                asset amount, std::string memo) {

  // //   token::transfer_action transfer_act{ token_account, { account,
  // active_permission } };
  // //          transfer_act.send( account, consumer_account, amount, memo );

  //       INLINE_ACTION_SENDER(eosio::token, transfer)(
  //          token_account, { {account, active_permission} },
  //          { account, consumer_account, amount, memo }
  //       );
  require_auth(account);
  require_auth(contract_account);
  transfer(account, consumer_account, amount, memo);

  data_service_subscriptions substable(_self, _self.value);

  auto subs_itr = substable.find(contract_account.value);
  check(subs_itr != substable.end(), "contract_account does not exist");

  substable.modify(subs_itr, _self,
                   [&](auto &subs) { subs.payment += amount; });
}

void bos_oracle::requestdata(uint64_t update_number, uint64_t service_id, name request,
                 std::string request_content) {
  require_auth(request);
  data_service_requests reqtable(_self, _self.value);

  reqtable.emplace(_self, [&](auto &r) {
    r.request_id = reqtable.available_primary_key();
    r.service_id = service_id;
    r.request = request;
    r.request_time = time_point_sec(now());
  });
}

// } /// namespace eosio
