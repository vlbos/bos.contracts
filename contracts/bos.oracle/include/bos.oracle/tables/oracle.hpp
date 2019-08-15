#pragma once
/*



*/

#include <eosio/eosio.hpp>

using namespace eosio;

// Controlling account
static const name titan_account = "eostitanprod"_n;

// Number of datapoints to hold
static const uint64_t datapoints_count = 21;

// Min value set to 0.01$ , max value set to 10,000$
static const uint64_t val_min = 100;
static const uint64_t val_max = 100000000;

const uint64_t one_minute = 1000000 * 55; // give extra time for cron jobs

// Types

// Holds the last datapoints_count datapoints from qualified oracles
struct [[eosio::table, eosio::contract("bos.oracle")]] eosusd {
  uint64_t id;
  name owner;
  uint64_t value;
  uint64_t average;
  uint64_t timestamp;

  uint64_t primary_key() const { return id; }
  uint64_t by_timestamp() const { return timestamp; }
  uint64_t by_value() const { return value; }

  EOSLIB_SERIALIZE(eosusd, (id)(owner)(value)(average)(timestamp))
};

// Holds the count and time of last eosusd writes for approved oracles
struct [[eosio::table, eosio::contract("bos.oracle")]] eosusdstats {
  name owner;
  uint64_t timestamp;
  uint64_t count;

  uint64_t primary_key() const { return owner.value; }
};

// Holds the list of oracles
struct [[eosio::table, eosio::contract("bos.oracle")]] oracles {
  name owner;

  uint64_t primary_key() const { return owner.value; }
};

// Multi index types definition
typedef eosio::multi_index<"eosusdstats"_n, eosusdstats> statstable;
typedef eosio::multi_index<"oracles"_n, oracles> oraclestable;
typedef eosio::multi_index<
    "eosusd"_n, eosusd,
    indexed_by<"value"_n, const_mem_fun<eosusd, uint64_t, &eosusd::by_value>>,
    indexed_by<"timestamp"_n,
               const_mem_fun<eosusd, uint64_t, &eosusd::by_timestamp>>>
    usdtable;

// class [[eosio::contract("bos.oracle")]] bos_oracle : public eosio::contract {
//  public:
//  using contract::contract;
//   bos_oracle(name receiver, name code, datastream<const char*> ds ) :
//   contract( receiver,  code, ds ){}

//   //Check if calling account is a qualified oracle
//   bool check_oracle(const name owner);
//   //Ensure account cannot push data more often than every 60 seconds
//   void check_last_push(const name owner);

//   //Push oracle message on top of queue, pop oldest element if queue size is
//   larger than X void update_eosusd_oracle(const name owner, const uint64_t
//   value);

//   //Write datapoint
//   [[eosio::action]]
//   void write(const name owner, const uint64_t value);

//   //Update oracles list
//   [[eosio::action]]
//   void setoracles(const std::vector<name>& oracleslist);

//   //Clear all data
//   [[eosio::action]]
//   void clear() ;

// ///bos.oraclize begin
//   request_table requests;
//   name token;
//   oracle_identities oracles_table;

//   // oraclize(name receiver, name code, datastream<const char*> ds ) :
//   contract( receiver,  code, ds ), requests(_self, _self.value),
//   token("boracletoken"_n), oracles_table(_self, _self.value) {}

// [[eosio::action]]
//   void addoracle(name oracle);

// [[eosio::action]]
//   void removeoracle(name oracle);

//   // @abi action
//   [[eosio::action]]
//   void ask(name administrator, name contract, string task, uint32_t
//   update_each, string memo, bytes args);

//   // @abi action
//   [[eosio::action]]
//   void disable(name administrator, name contract, string task, string memo);

//   // @abi action
//   [[eosio::action]]
//   void once(name administrator, name contract, string task, string memo,
//   bytes args);

//   // @abi action
//   [[eosio::action]]
//   void push(name oracle, name contract, string task, string memo, bytes
//   data);

//   void set(const request &value, name bill_to_account);

//   ///bos.oraclize end
// };
