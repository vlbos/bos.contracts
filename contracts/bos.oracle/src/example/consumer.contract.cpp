#include <eosio/action.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>
#include <string>
#include "bos.oracle/tables/oracle_api.hpp"
#include "bos.oracle/example/consumer.contract.hpp"

using namespace eosio;
using std::string;

void consumer_contract::receivejson(name self, name code) {

   auto payload = unpack_action_data<push_json>();


   std::string p = payload.data_json; 

   print(p.c_str());
}

void consumer_contract::fetchdata(name oracle, uint64_t service_id, uint64_t update_number, uint64_t request_id) {

   if (0 != update_number) {
      oracle_data oracledatatable(oracle, service_id);
      print("update number =", update_number);
      check(oracledatatable.begin() != oracledatatable.end(), " no  data found ");
      auto itr = oracledatatable.find(update_number);
      check(itr != oracledatatable.end(), " no update number found ");
      print(itr->value.c_str());
   } else {
      oracle_request_data oracledatatable(oracle, service_id);
      print("update number =", update_number);
      check(oracledatatable.begin() != oracledatatable.end(), " no  data found ");
      auto itr = oracledatatable.find(request_id);
      check(itr != oracledatatable.end(), " no request id  found ");
      print(itr->value.c_str());
   }
}


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
         EOSIO_DISPATCH_HELPER(consumer_contract, (fetchdata))
      }
   }


   if (code != self.value && action == "oraclepush"_n.value) {
      thiscontract.receivejson(name(receiver), name(code));
   }
}
