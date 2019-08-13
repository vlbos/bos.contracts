#include <string>
#include <eosio/eosio.hpp>
#include <eosio/action.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include <eosio/system.hpp>

#include "bos.dappuser/bos.dappuser.hpp"

using namespace eosio;
using std::string;


  void YOUR_CONTRACT_NAME::receive(name self, name code)
  {
    check(known_master != "undefined"_n, "Contract didn't setupped yet");
    check(code == known_master, "Unkown master contract");
    auto payload = unpack_action_data<push_data>();

    if (strcmp(payload.task.c_str(), "c0fe86756e446503eed0d3c6a9be9e6276018fead3cd038932cf9cc2b661d9de") == 0)
    {
      price p = unpack<price>(payload.data);
      ethbtc.set(p, _self);
      return;
    }

    check(true, "Unknown task received");
  }

  void YOUR_CONTRACT_NAME::receivejson(name self, name code)
  {
    
    // check(known_master != "undefined"_n, "Contract didn't setupped yet");
    // check(code == known_master, "Unkown master contract");
    auto payload = unpack_action_data<push_json>();

    // if (strcmp(payload.task.c_str(), "c0fe86756e446503eed0d3c6a9be9e6276018fead3cd038932cf9cc2b661d9de") == 0)
    // {
      std::string p = payload.data_json;//unpack<std::string>(payload.data_json);
    //   ethbtc.set(p, _self);
    //   return;
    // }
    // print("#################");
    // print(p.c_str());
    // print("!!!!!!!!!!!!!!!!!!");
    // check(false, p.c_str());
  }

 void YOUR_CONTRACT_NAME::fetchdata(name oracle,uint64_t service_id,uint64_t update_number)
  {
    oracle_data oracledatatable(oracle,service_id);
    print("update number =",update_number);
     check (oracledatatable.begin()!= oracledatatable.end()," no  data found ");
      auto itr = oracledatatable.find(update_number);
      check (itr!= oracledatatable.end()," no update number found ");
      print(itr->value.c_str());
  }

  // @abi action
  void YOUR_CONTRACT_NAME::setup(name oracle)
  {
    require_auth(_self);
    account_master(_self, _self.value).set(oracle, _self);
    ask_data(_self, oracle, "c0fe86756e446503eed0d3c6a9be9e6276018fead3cd038932cf9cc2b661d9de", 10u,string(),
             pack(request_args{
                 bytes{},
                 bytes{}}));
  }

  void YOUR_CONTRACT_NAME::ask_data(name administrator,
                name registry,
                string data,
                uint32_t update_each,
                string memo,
                bytes args)
  {
    action(permission_level{administrator, "active"_n},
           registry, "ask"_n,
           std::make_tuple(administrator, _self, data, update_each, memo, args))
        .send();
  }
// };

  extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    name self = name(receiver);
    if (action == "onerror"_n.value) {
      /* onerror is only valid if it is for the "eosio" code account and
       * authorized by "eosio"'s "active permission */
      check(
          code == "eosio"_n.value,
          "onerror action's are only valid from the \"eosio\" system account");
    }

    datastream<const char *> ds = datastream<const char *>(nullptr, 0);
    YOUR_CONTRACT_NAME thiscontract(self, self, ds);

    if (code == self.value || action == "onerror"_n.value) {

      switch (action) {
        // NB: Add custom method in bracets after (setup) to use them as
        // endpoints
        EOSIO_DISPATCH_HELPER(YOUR_CONTRACT_NAME, (fetchdata)(setup))
      }
    }

    if (code != self.value && action == "push"_n.value) {
      thiscontract.receive(name(receiver), name(code));
    }

    if (code != self.value && action == "oraclepush"_n.value) {
      thiscontract.receivejson(name(receiver), name(code));
    }

  }
