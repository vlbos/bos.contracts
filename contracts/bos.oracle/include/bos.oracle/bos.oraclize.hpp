#pragma once
#include <eosiolib/eosio.hpp>
using std::vector;

typedef std::vector<char> bytes;


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

struct [[eosio::table, eosio::contract("bos.oraclize")]] request_args
{
  bytes schema;
  bytes args;

  EOSLIB_SERIALIZE(request_args, (schema)(args))
};

// @abi table price i64
struct [[eosio::table, eosio::contract("bos.oraclize")]] price
{
  uint64_t value;
  uint8_t decimals;

  EOSLIB_SERIALIZE(price, (value)(decimals))
};


namespace ducatur
{

checksum256 get_hash(const string &task, const name &contract);

checksum256 get_full_hash(const string &task, const string &memo, const name &contract);

uint64_t pack_hash(checksum256 hash);

// @abi table request i64
struct [[eosio::table, eosio::contract("bos.oraclize")]] request
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
struct [[eosio::table, eosio::contract("bos.oraclize")]] oracles
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

class [[eosio::contract("bos.oraclize")]] masteroracle : public eosio::contract
{
public:
  using contract::contract;

  request_table requests;
  name token;
  oracle_identities oracles_table;

  masteroracle(name receiver, name code, datastream<const char*> ds ) : contract( receiver,  code, ds ), requests(_self, _self.value), token("ducaturtoken"_n), oracles_table(_self, _self.value) {}

  void addoracle(name oracle);

  void removeoracle(name oracle);

  // @abi action
  [[eosio::action]]
  void ask(name administrator, name contract, string task, uint32_t update_each, string memo, bytes args);

  // @abi action
  [[eosio::action]]
  void disable(name administrator, name contract, string task, string memo);

  // @abi action
  [[eosio::action]]
  void once(name administrator, name contract, string task, string memo, bytes args);

  // @abi action
  [[eosio::action]]
  void push(name oracle, name contract, string task, string memo, bytes data);

  void set(const request &value, name bill_to_account);
};


} // namespace ducatur