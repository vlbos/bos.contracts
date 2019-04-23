#pragma once
#include <eosiolib/eosio.hpp>
#include "bos.consumer/bostypes.hpp"
#include "bos.oraclized.hpp"
// using namespace eosio;
// using std::string;
// template <uint64_t OraclizeName, uint32_t BestBeforeOffset, uint32_t UpdateOffset, typename T>
// class oraclized
// {
//   struct data
//   {
//     uint32_t best_before;
//     uint32_t update_after;
//     T value;

//     EOSLIB_SERIALIZE(data, (best_before)(update_after)(value))
//   };

//   constexpr static uint64_t pk_value = OraclizeName;
//   struct row
//   {
//     data value;

//     uint64_t primary_key() const { return pk_value; }

//     EOSLIB_SERIALIZE(row, (value))
//   };

//   typedef eosio::multi_index<name(OraclizeName), row> ctable;

// private:
//   ctable _t;

// public:
//   oraclized(name code, name scope) : _t(code, scope.value) {}

//   bool exists()
//   {
//     return _t.find(pk_value) != _t.end();
//   }

//   bool fresh()
//   {
//     return exists() && get().best_before > now();
//   }

//   bool require_update()
//   {
//     return exists() && get().update_after < now();
//   }

//   T value()
//   {
//     return get().value;
//   }

//   data get()
//   {
//     auto itr = _t.find(pk_value);
//     eosio_assert(itr != _t.end(), "singleton does not exist");
//     return itr->value;
//   }

//   data get_or_default(const T &def = T())
//   {
//     auto itr = _t.find(pk_value);
//     return itr != _t.end() ? itr->value : def;
//   }

//   data get_or_create(name bill_to_account, const T &def = T())
//   {
//     auto itr = _t.find(pk_value);
//     return itr != _t.end() ? itr->value
//                            : _t.emplace(bill_to_account, [&](row &r) { r.value = data{}; });
//   }

//   void set(const T &value, name bill_to_account)
//   {
//     auto itr = _t.find(pk_value);
//     if (itr != _t.end())
//     {
//       _t.modify(itr, bill_to_account, [&](row &r) { r.value = data{now() + BestBeforeOffset, now() + UpdateOffset, value}; });
//     }
//     else
//     {
//       _t.emplace(bill_to_account, [&](row &r) { r.value = data{now() + BestBeforeOffset, now() + UpdateOffset, value}; });
//     }
//   }

//   void remove()
//   {
//     auto itr = _t.find(pk_value);
//     if (itr != _t.end())
//     {
//       _t.erase(itr);
//     }
//   }
// };

// // @abi table args i64
struct request_args
{
  bytes schema;
  bytes args;
};

// carbon-copy call structure
struct  push_data
{
  name oracle;
  name contract;
  string task;
  string memo;
  bytes data;

  EOSLIB_SERIALIZE(push_data, (oracle)(contract)(task)(memo)(data))
};

struct price
{
  uint64_t value;
  uint8_t decimals;

  EOSLIB_SERIALIZE(price, (value)(decimals))
};

// @abi table ethbtc i64
struct [[eosio::table, eosio::contract("bos.consumer")]] ethbtc
{
  uint32_t best_before;
  uint32_t update_after;
  price value;

  EOSLIB_SERIALIZE(ethbtc, (best_before)(update_after)(value))
};

typedef oraclized<("ethbtc"_n).value, 11, 10, price> ethbtc_data;

typedef singleton<"master"_n, name> account_master;

// class [[eosio::contract("bos.consumer")]] YOUR_CONTRACT_NAME : public eosio::contract
// {
// private:
//   ethbtc_data ethbtc;

//   name known_master;

// public:
//   using contract::contract;

//   YOUR_CONTRACT_NAME(name s, name code, datastream<const char*> ds ) : contract(s,code,ds), ethbtc(_self, _self)
//   {
//     known_master = account_master(_self, _self.value).get_or_create(_self, "undefined"_n);
//   }

//   void receive(name self, name code);

//   // @abi action
//   [[eosio::action]]
//   void setup(name oracle);

//     void ask_data(name administrator,
//                 name registry,
//                 string data,
//                 uint32_t update_each,
//                 string memo,
//                 bytes args);
// };

// // extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action)
// // {
// //   name self = name(receiver);
// //   if (action == "onerror"_n.value)
// //   {
// //     /* onerror is only valid if it is for the "eosio" code account and authorized by "eosio"'s "active permission */
// //     eosio_assert(code == "eosio"_n.value, "onerror action's are only valid from the \"eosio\" system account");
// //   }

// // datastream<const char*> ds = datastream<const char*>(nullptr, 0);
// //   YOUR_CONTRACT_NAME thiscontract(self,self,ds);

// //   if (code == self.value || action == "onerror"_n.value)
// //   {

// //     switch (action)
// //     {
// //       // NB: Add custom method in bracets after (setup) to use them as endpoints
// //       EOSIO_DISPATCH_HELPER(YOUR_CONTRACT_NAME, (setup))
// //     }
// //   }

// //   if (code != self.value && action == "push"_n.value)
// //   {
// //     thiscontract.receive(name(receiver), name(code));
// //   }
// // }