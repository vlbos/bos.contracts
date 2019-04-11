#include <string>
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/print.h>
#include "bos.oracle/bos.oraclize.hpp"

using namespace eosio;
using std::string;

// enum request_type
// {
//   DISABLED,
//   REPEAT,
//   ONCE
// };
constexpr uint8_t DISABLED_REQUEST = 0;
constexpr uint8_t REPEATABLE_REQUEST = 1;
constexpr uint8_t ONCE_REQUEST = 2;
typedef uint8_t request_type;

// @abi table tuple i64
typedef std::tuple<> args_tupple;

// @abi table args i64
struct request_args
{
  bytes schema;
  bytes args;

  EOSLIB_SERIALIZE(request_args, (schema)(args))
};

// @abi table price i64
struct price
{
  uint64_t value;
  uint8_t decimals;

  EOSLIB_SERIALIZE(price, (value)(decimals))
};

namespace ducatur
{

checksum256 get_hash(const string &task, const name &contract)
{
  checksum256 result;
  size_t tasklen = strlen(task.c_str());
  char *buffer = (char *)malloc(tasklen + 8);
  memcpy(buffer, &contract, 8);
  memcpy(buffer + 8, task.data(), tasklen);
  result = sha256(buffer, tasklen + 8);
  return result;
}

checksum256 get_full_hash(const string &task, const string &memo, const name &contract)
{
  checksum256 result;
  size_t tasklen = strlen(task.c_str());
  size_t memolen = strlen(memo.c_str());
  char *buffer = (char *)malloc(tasklen + memolen + 8);
  memcpy(buffer, &contract, 8);
  memcpy(buffer + 8, task.data(), tasklen);
  memcpy(buffer + tasklen + 8, memo.data(), memolen);
  result = sha256(buffer, tasklen + 8);
  return result;
}

uint64_t pack_hash(checksum256 hash)
{
  const uint64_t *p64 = reinterpret_cast<const uint64_t *>(&hash);
  return p64[0] ^ p64[1] ^ p64[2] ^ p64[3];
}

// @abi table request i64
struct request
{
  string task;
  string memo;
  bytes args;
  name administrator;
  name contract;
  uint32_t timestamp;
  uint32_t update_each;
  request_type mode;

  uint64_t primary_key() const
  {
    return pack_hash(get_full_hash(task, memo, contract));
  }

  EOSLIB_SERIALIZE(request, (task)(memo)(args)(administrator)(contract)(timestamp)(update_each)(mode))
};

// @abi table oracles i64
struct oracles
{
  name account;

  uint64_t primary_key() const
  {
    return account.value;
  }

  EOSLIB_SERIALIZE(oracles, (account))
};

typedef multi_index<"request"_n, request> request_table;
typedef multi_index<"oracles"_n, oracles> oracle_identities;

class masteroracle : public eosio::contract
{
public:
  using contract::contract;

  request_table requests;
  name token;
  oracle_identities oracles_table;

  masteroracle(name receiver, name code, datastream<const char*> ds ) : contract( receiver,  code, ds ), requests(_self, _self.value), token("ducaturtoken"_n), oracles_table(_self, _self.value) {}

  void addoracle(name oracle)
  {
    require_auth(_self);
    auto itt = oracles_table.find(oracle.value);
    eosio_assert(itt == oracles_table.end(), "Already known oracle");

    oracles_table.emplace(_self, [&](oracles &i) {
      i.account = oracle;
    });
  }

  void removeoracle(name oracle)
  {
    require_auth(_self);
    auto itt = oracles_table.find(oracle.value);
    eosio_assert(itt != oracles_table.end(), "Unknown oracle");

    oracles_table.erase(itt);
  }

  // @abi action
  void ask(name administrator, name contract, string task, uint32_t update_each, string memo, bytes args)
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
  void disable(name administrator, name contract, string task, string memo)
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
  void once(name administrator, name contract, string task, string memo, bytes args)
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
  void push(name oracle, name contract, string task, string memo, bytes data)
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

  void set(const request &value, name bill_to_account)
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
};

EOSIO_DISPATCH(masteroracle, (addoracle)(removeoracle)(ask)(once)(disable)(push))
} // namespace ducatur
