#include <string>
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/crypto.h>
// #include <eosiolib/print.h>
#include "bos.oracle/bos.oracle.hpp"


  void bos_oracle::addoracle(name oracle)
  {
    require_auth(_self);
    auto itt = oraclizes_table.find(oracle.value);
    
    eosio_assert(itt == oraclizes_table.end(), "Already known oracle");

    oraclizes_table.emplace(_self, [&](oraclizes &i) {
      i.account = oracle;
    });
  }

  void bos_oracle::removeoracle(name oracle)
  {
    require_auth(_self);
    auto itt = oraclizes_table.find(oracle.value);
    eosio_assert(itt != oraclizes_table.end(), "Unknown oracle");

    oraclizes_table.erase(itt);
  }

  // @abi action
  void bos_oracle::ask(name administrator, name contract, string task, uint32_t update_each, string memo, bytes args)
  {
    require_auth(administrator);
    auto itt = requests.find(pack_hash(get_full_hash(task, memo, contract)));
    eosio_assert(itt == requests.end() || itt->mode != REPEATABLE_REQUEST, "Already repeatable request");
    set(request{task,
                memo,
                args,
                administrator,
                contract,
                0,
                update_each,
                REPEATABLE_REQUEST},
        administrator);
  }

  // @abi action
  void bos_oracle::disable(name administrator, name contract, string task, string memo)
  {
    require_auth(administrator);
    uint64_t id = pack_hash(get_full_hash(task, memo, contract));
    auto itt = requests.find(id);
    eosio_assert(itt != requests.end(), "Unknown request");
    eosio_assert(itt->mode != DISABLED_REQUEST, "Non-active request");

    request changed(*itt);
    changed.mode = DISABLED_REQUEST;
    set(changed, administrator);
  }

  // @abi action
  void bos_oracle::once(name administrator, name contract, string task, string memo, bytes args)
  {
    require_auth(administrator);
    auto itt = requests.find(pack_hash(get_full_hash(task, memo, contract)));
    eosio_assert(itt == requests.end() || itt->mode != ONCE_REQUEST, "Already repeatable request");
    set(request{task,
                memo,
                args,
                administrator,
                contract,
                0,
                0,
                ONCE_REQUEST},
        administrator);
  }

  // @abi action
  void bos_oracle::push(name oracle, name contract, string task, string memo, bytes data)
  {
    require_auth(oracle);
    uint64_t id = pack_hash(get_full_hash(task, memo, contract));
    auto itt = requests.find(id);
    eosio_assert(itt != requests.end(), "Unknown request");

    eosio_assert(itt->mode != DISABLED_REQUEST, "Disabled request push");
    eosio_assert(now() >= itt->timestamp + itt->update_each, "Too early to update");
    // carbon-copy call
    require_recipient(itt->contract);

    request changed(*itt);
    if (changed.mode == ONCE_REQUEST)
    {
      changed.mode = DISABLED_REQUEST;
    }
    changed.timestamp = now();
    set(changed, _self);
  }

  void bos_oracle::set(const request &value, name bill_to_account)
  {
    auto itr = requests.find(value.primary_key());
    if (itr != requests.end())
    {
      requests.modify(itr, bill_to_account, [&](request &r) {
        r.task = value.task;
        r.contract = value.contract;
        r.memo = value.memo;
        r.args = value.args;
        r.timestamp = value.timestamp;
        r.update_each = value.update_each;
        r.mode = value.mode;
      });
    }
    else
    {
      requests.emplace(bill_to_account, [&](request &r) {
        r.task = value.task;
        r.contract = value.contract;
        r.memo = value.memo;
        r.args = value.args;
        r.timestamp = value.timestamp;
        r.update_each = value.update_each;
        r.mode = value.mode;
      });
    }
  }
