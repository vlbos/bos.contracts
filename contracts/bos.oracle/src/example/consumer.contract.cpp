

#include "bos.oracle/example/consumer.contract.hpp"
#include "bos.oracle/example/eosio.token.hpp"
#include "bos.oracle/tables/oracle_api.hpp"
#include <eosio/action.hpp>
#include <eosio/eosio.hpp>
// #include <eosio/singleton.hpp>
// #include <eosio/system.hpp>
// #include <eosio/time.hpp>
#include <string>
using eosio::symbol;

using namespace eosio;
using std::string;
static constexpr symbol _core_symbol = symbol(symbol_code("BOS"), 4);

void consumer_contract::receivejson(name self, name code) {

   auto payload = unpack_action_data<push_json>();

   std::string p = payload.data;

   print(p.c_str());
}

void consumer_contract::fetchdata(name oracle, uint64_t service_id, uint64_t cycle_number, uint64_t request_id) {
    require_auth(_self);
   oracle_data oracledatatable(oracle, service_id);
   check(oracledatatable.begin() != oracledatatable.end(), " no  data found ");

   uint128_t id = (uint128_t(request_id) << 64) | cycle_number;

   auto update_id_idx = oracledatatable.get_index<"bynumber"_n>();
   // auto update_number_itr_lower = update_number_idx.lower_bound(id);
   // auto update_number_itr_upper = update_number_idx.upper_bound(id);
   // for (auto itr = update_number_itr_lower; itr != update_number_idx.end() && itr != update_number_itr_upper; ++itr) {
   //    print(itr->value.c_str());
   // }

   auto itr = update_id_idx.find(id);
   check(itr != update_id_idx.end(), " no  update id  found ");
   print(itr->data.c_str());
}

void consumer_contract::transfer(name from, name to, asset quantity, string memo) {
   //  require_auth(_self);
   // if (from == _self || to != _self) {
   //    return;
   // }
   print("\n consumer_contract::transfer");
   require_recipient("oracle.bos"_n);
}

void consumer_contract::dream(name who, asset value, string memo) {
   require_auth(_self);
   // auto eos_token = eosio::token("eosio.token"_n);
   auto balance = eosio::token::get_balance("eosio.token"_n, _self, _core_symbol.code()); // symbol_type(S(4, EOS)).name());
   // action(permission_level{_self, N(active)}, "eosio.token"_n, N(transfer), std::make_tuple(_self, who, value, memo)).send();
   // action(permission_level{_self, N(active)}, _self, N(reality), std::make_tuple(balance)).send();
   action(permission_level{_self, "active"_n}, "eosio.token"_n, "transfer"_n, std::make_tuple(_self, who, value, memo)).send();
   action(permission_level{_self, "active"_n}, _self, "reality"_n, std::make_tuple(balance)).send();
}

void consumer_contract::reality(asset data) {
   require_auth(_self);
   // auto eos_token = eosio::token("eosio.token"_n);
   auto newBalance = eosio::token::get_balance("eosio.token"_n, _self, _core_symbol.code()); // symbol_type(S(4, EOS)).name());
   check(newBalance.amount > data.amount, "bad day");
}


struct transferx {
  name from;
  name to;
  asset quantity;
  string memo;

  EOSLIB_SERIALIZE(transferx, (from)(to)(quantity)(memo))
};

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action) {
   name self = name(receiver);
   if (action == "onerror"_n.value) {
      /* onerror is only valid if it is for the "eosio" code account and
       * authorized by "eosio"'s "active permission */
      check(code == "eosio"_n.value, "onerror action's are only valid from the \"eosio\" system account");
   }

   datastream<const char*> ds = datastream<const char*>(nullptr, 0);
   consumer_contract thiscontract(self, self, ds);

   if (code == self.value || action == "onerror"_n.value) {

      switch (action) {
         // NB: Add custom method in bracets after (setup) to use them as
         // endpoints
         EOSIO_DISPATCH_HELPER(consumer_contract, (fetchdata)(transfer)(dream)(reality))
      }
   }

if (code == "eosio.token"_n.value && action == "transfer"_n.value) {
    const transferx &t = unpack_action_data<transferx>();
    thiscontract.transfer(t.from, t.to, t.quantity, t.memo);
  }

   if (code != self.value && action == "oraclepush"_n.value) {
      thiscontract.receivejson(name(receiver), name(code));
   }
}
