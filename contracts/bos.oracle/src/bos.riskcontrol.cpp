
#include "bos.oracle/bos.oracle.hpp"
#include <eosiolib/action.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/eosio.hpp>
#include <eosiolib/print.h>
#include <eosiolib/time.hpp>
#include <eosiolib/transaction.hpp>
#include <string>

void bos_oracle::transfer(name from, name to, asset quantity, string memo) {
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
  action(permission_level{from, "active"_n}, token_account, "transfer"_n,
         std::make_tuple(from, to, quantity, memo))
      .send();
  // INLINE_ACTION_SENDER(eosio::token, transfer)
  // (token_account, {{from, active_permission}, {to, active_permission}},
  //  {from, to, quantity, memo});
}

/// from dapp user to dapp
void bos_oracle::deposit(name from, name to, asset quantity, string memo,
                         bool is_notify) {
  transfer(from, to, quantity, memo);

  auto payer = has_auth(to) ? to : from;
  add_balance(from, quantity, payer);

  if (is_notify) {
    // notify dapp
  }
}

/// from dapp to dapp user
void bos_oracle::withdraw(name from, name to, asset quantity, string memo) {

  sub_balance(from, quantity);

  // find service
  data_service_subscriptions svcsubstable(_self, _self.value);
  auto acc_idx = svcsubstable.get_index<"byaccount"_n>();
  auto svcsubs_itr = acc_idx.find(from.value);
  check(svcsubs_itr != acc_idx.end(), "account does not subscribe services");
  // find stake
  data_service_stakes svcstaketable(_self, _self.value);
  auto svcstake_itr = svcstaketable.find(svcsubs_itr->service_id);
  check(svcstake_itr != svcstaketable.end(), " no service stake  found");
  // check(  svcstake_itr->total_stake_amount- svcstake_itr->freeze_amount >
  // quantity, " no service stake  found" );
  //
  uint64_t time_length = 1;
  if (svcstake_itr->total_stake_amount - svcstake_itr->freeze_amount >=
      quantity) {
    svcstaketable.modify(svcstake_itr, same_payer,
                         [&](auto &ss) { ss.freeze_amount += quantity; });

    transfer(from, to, quantity, memo);
    add_freeze_delay(svcsubs_itr->service_id, from, time_point_sec(now()),
                     time_point_sec(time_length), quantity, 0,
                     transfer_type::transfer_freeze);
  } else {
    // risk guarantee
    //    data_services datasvctable( _self, _self.value );
    //    auto svc_itr=datasvctable.find(svcsubs_itr->service_id);
    //   check( svc_itr != datasvctable.end(), " no service   found" );
    // if(svc_itr->risk_control_amount.amount==0)
    // {
    //    //risk delay
    // }

    /// delay time length

    add_freeze_delay(svcsubs_itr->service_id, from, time_point_sec(now()),
                     time_point_sec(time_length), quantity, 0,
                     transfer_type::transfer_delay);

    transaction t;
    t.actions.emplace_back(permission_level{_self, active_permission}, _self,
                           "transfer"_n,
                           std::make_tuple(from, to, quantity, memo));
    t.delay_sec = time_length;
    uint128_t deferred_id = (uint128_t(from.value) << 64) | to.value;
    cancel_deferred(deferred_id);
    t.send(deferred_id, from);
  }
}

void bos_oracle::add_freeze_delay(uint64_t service_id, name account,
                                  time_point_sec start_time,
                                  time_point_sec duration, asset amount,
                                  uint64_t status, uint64_t type) {
  transfer_freeze_delays transfertable(_self, _self.value);

  transfertable.emplace(_self, [&](auto &t) {
    t.transfer_id = transfertable.available_primary_key();
    t.service_id = service_id;
    t.account = account;
    t.start_time = start_time;
    t.duration = duration;
    t.amount = amount;
    t.status = status;
    t.type = type;
  });
}

uint64_t bos_oracle::add_guarantee(uint64_t service_id, name account,
                                   time_point_sec start_time,
                                   time_point_sec duration, asset amount,
                                   uint64_t status) {
  risk_guarantees guaranteetable(_self, _self.value);

  uint64_t risk_id = 0;
  guaranteetable.emplace(_self, [&](auto &g) {
    g.risk_id = guaranteetable.available_primary_key();
    g.account = account;
    g.amount = amount;
    g.start_time = start_time;
    g.duration = duration;
    g.status = status;
    //  g.sig = sig;
    risk_id = g.risk_id;
  });

  return risk_id;
}

void bos_oracle::sub_balance(name owner, asset value) {
  accounts dapp_acnts(_self, owner.value);

  const auto &dapp =
      dapp_acnts.get(value.symbol.code().raw(), "no balance object found");
  check(dapp.balance.amount >= value.amount, "overdrawn balance");

  dapp_acnts.modify(dapp, owner, [&](auto &a) { a.balance -= value; });
}

void bos_oracle::add_balance(name owner, asset value, name ram_payer) {
  accounts dapp_acnts(_self, owner.value);
  auto dapp = dapp_acnts.find(value.symbol.code().raw());
  if (dapp == dapp_acnts.end()) {
    dapp_acnts.emplace(ram_payer, [&](auto &a) { a.balance = value; });
  } else {
    dapp_acnts.modify(dapp, same_payer, [&](auto &a) { a.balance += value; });
  }
}