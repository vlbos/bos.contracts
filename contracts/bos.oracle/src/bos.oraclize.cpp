#include <string>
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/print.h>
#include "bos.oracle/bos.oracle.hpp"


// // namespace bosoracle
// {

// checksum256 get_hash(const string &task, const name &contract)
// {
//   checksum256 result;
//   size_t tasklen = strlen(task.c_str());
//   char *buffer = (char *)malloc(tasklen + 8);
//   memcpy(buffer, &contract, 8);
//   memcpy(buffer + 8, task.data(), tasklen);
//   result = sha256(buffer, tasklen + 8);
//   return result;
// }

// checksum256 get_full_hash(const string &task, const string &memo, const name &contract)
// {
//   checksum256 result;
//   size_t tasklen = strlen(task.c_str());
//   size_t memolen = strlen(memo.c_str());
//   char *buffer = (char *)malloc(tasklen + memolen + 8);
//   memcpy(buffer, &contract, 8);
//   memcpy(buffer + 8, task.data(), tasklen);
//   memcpy(buffer + tasklen + 8, memo.data(), memolen);
//   result = sha256(buffer, tasklen + 8);
//   return result;
// }

// uint64_t pack_hash(checksum256 hash)
// {
//   const uint64_t *p64 = reinterpret_cast<const uint64_t *>(&hash);
//   return p64[0] ^ p64[1] ^ p64[2] ^ p64[3];
// }

// // @abi table request i64
// struct request
// {
//   string task;
//   string memo;
//   bytes args;
//   name administrator;
//   name contract;
//   uint32_t timestamp;
//   uint32_t update_each;
//   request_type mode;

//   uint64_t primary_key() const
//   {
//     return pack_hash(get_full_hash(task, memo, contract));
//   }

//   EOSLIB_SERIALIZE(request, (task)(memo)(args)(administrator)(contract)(timestamp)(update_each)(mode))
// };

// // @abi table oraclizes i64
// struct oraclizes
// {
//   name account;

//   uint64_t primary_key() const
//   {
//     return account.value;
//   }

//   EOSLIB_SERIALIZE(oraclizes, (account))
// };

// typedef multi_index<"request"_n, request> request_table;
// typedef multi_index<"oraclizes"_n, oraclizes> oracle_identities;

// class bos_oracle : public eosio::contract
// {
// public:
//   using contract::contract;

//   request_table requests;
//   name token;
//   oracle_identities oraclizes_table;

  // bos_oracle(name receiver, name code, datastream<const char*> ds ) : contract( receiver,  code, ds ), requests(_self, _self.value), token("bosoracletoken"_n), oraclizes_table(_self, _self.value) {}

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
// };


// } // namespace bosoracle


// EOSIO_DISPATCH( bosoracle::bos_oracle, (addoracle)(removeoracle)(ask)(once)(disable)(push))