/*

  bos_oracle

  Author: Guillaume "Gnome" Babin-Tremblay - EOS Titan
  
  Website: https://eostitan.com
  Email: guillaume@eostitan.com

  Github: https://github.com/eostitan/bos_oracle/
  
  Published under MIT License

*/

#include <eosiolib/eosio.hpp>
#include <eosiolib/fixedpoint.hpp>
#include <eosiolib/chain.h>
#include "bos.oracle/bos.oracle.hpp"
#include "bos.oraclize.cpp"
#include "bos.provider.cpp"
#include "bos.consumer.cpp"
#include "bos.riskcontrol.cpp"
#include "bos.arbitration.cpp"
#include "bos.fee.cpp"
#include "bos.stat.cpp"

using namespace eosio;



  //Check if calling account is a qualified oracle
  bool bos_oracle::check_oracle(const name owner){

    oraclestable oracles(_self, _self.value);

    auto itr = oracles.begin();
    while (itr != oracles.end()) {
        if (itr->owner == owner) return true;
        else itr++;
    }

    capi_name producers[21] = { {} };
    uint32_t bytes_populated = get_active_producers(producers, sizeof(name)*21);
    
   

    //Account is oracle if account is an active producer
    for(int i = 0; i < 21; i=i+1 ) {
      if (producers[i] == owner.value){
        return true;
      }
    }

    return false;

  }

  //Ensure account cannot push data more often than every 60 seconds
  void bos_oracle::check_last_push(const name owner){

    statstable store(_self, _self.value);

    auto itr = store.find(owner.value);
    if (itr != store.end()) {

      uint64_t ctime = current_time();
      auto last = store.get(owner.value);

      eosio_assert(last.timestamp + one_minute <= ctime, "can only call every 60 seconds");

      store.modify( itr, get_self(), [&]( auto& s ) {
        s.timestamp = ctime;
        s.count++;
      });

    } else {

      store.emplace(get_self(), [&](auto& s) {
        s.owner = owner;
        s.timestamp = current_time();
        s.count = 0;
      });

    }

  }

  //Push oracle message on top of queue, pop oldest element if queue size is larger than X
  void bos_oracle::update_eosusd_oracle(const name owner, const uint64_t value){

    usdtable usdstore(_self,_self.value);

    auto size = std::distance(usdstore.begin(), usdstore.end());

    uint64_t avg = 0;
    uint64_t primary_key ;

    //Calculate approximative rolling average
    if (size>0){

      //Calculate new primary key by substracting one from the previous one
      auto latest = usdstore.begin();
      primary_key = latest->id - 1;

      //If new size is greater than the max number of datapoints count
      if (size+1>datapoints_count){

        auto oldest = usdstore.end();
        oldest--;

        //Pop oldest point
        usdstore.erase(oldest);

        //Insert next datapoint
        auto c_itr = usdstore.emplace(get_self(), [&](auto& s) {
          s.id = primary_key;
          s.owner = owner;
          s.value = value;
          s.timestamp = current_time();
        });

        //Get index sorted by value
        auto value_sorted = usdstore.get_index<"value"_n>();

        //skip first 6 values
        auto itr = value_sorted.begin();
        itr++;
        itr++;
        itr++;
        itr++;
        itr++;

        //get next 9 values
        for (int i = 0; i<9;i++){
          itr++;
          avg+=itr->value;
        }

        //calculate average
        usdstore.modify(c_itr, get_self(), [&](auto& s) {
          s.average = avg / 9;
        });

      }
      else {

        //No average is calculated until the expected number of datapoints have been received
        avg = value;

        //Push new point at the end of the queue
        usdstore.emplace(get_self(), [&](auto& s) {
          s.id = primary_key;
          s.owner = owner;
          s.value = value;
          s.average = avg;
          s.timestamp = current_time();
        });

      }

    }
    else {
      primary_key = std::numeric_limits<unsigned long long>::max();
      avg = value;

      //Push new point at the end of the queue
      usdstore.emplace(get_self(), [&](auto& s) {
        s.id = primary_key;
        s.owner = owner;
        s.value = value;
        s.average = avg;
        s.timestamp = current_time();
      });

    }

  }

  //Write datapoint

  void bos_oracle::write(const name owner, const uint64_t value) {
    
    require_auth(owner);

    eosio_assert(value >= val_min && value <= val_max, "value outside of allowed range");
    eosio_assert(check_oracle(owner), "account is not an active producer or approved oracle");
    
    check_last_push(owner);
    update_eosusd_oracle(owner, value);
    
  }

  //Update oracles list
 
  void bos_oracle::setoracles(const std::vector<name>& oracleslist) {
    
    require_auth(titan_account);

    oraclestable oracles(_self,_self.value);

    while (oracles.begin() != oracles.end()) {
        auto itr = oracles.end();
        itr--;
        oracles.erase(itr);
    }

    for(const name& oracle : oracleslist){
      oracles.emplace(get_self(), [&](auto& o) {
        o.owner = oracle;
      });
    }

  }

  //Clear all data
  void bos_oracle::clear() {
    require_auth(titan_account);
    statstable lstore(_self,_self.value);
    usdtable estore(_self,_self.value);
    oraclestable oracles(_self,_self.value);
    
    while (lstore.begin() != lstore.end()) {
        auto itr = lstore.end();
        itr--;
        lstore.erase(itr);
    }
    
    while (estore.begin() != estore.end()) {
        auto itr = estore.end();
        itr--;
        estore.erase(itr);
    }
    
    while (oracles.begin() != oracles.end()) {
        auto itr = oracles.end();
        itr--;
        oracles.erase(itr);
    }

  }


// EOSIO_DISPATCH(bos_oracle, (write)(setoracles)(clear)(addoracle)(removeoracle)(ask)(once)(disable)(push)
// (regservice)(unregservice)(execaction)(stakeasset)(unstakeasset)(innerpush)(pushdata)(multipush)(innerpublish)(publishdata)(multipublish)(autopublish)(addfeetypes)(addfeetype)(claim)
// (subscribe)(requestdata)(payservice)(starttimer)
// (regarbitrat)(complain)(uploadeviden)(uploadresult)(acceptarbi)(respcase)
// (deposit)(withdraw)
// )

struct transferx {
      name from;
      name to;
      asset quantity;
      string memo;

      EOSLIB_SERIALIZE(transferx, (from)(to)(quantity)(memo))
    };

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    // print("&&&&&&&&&&&&&&&&&&&");
  if (action == "onerror"_n.value) {
    /* onerror is only valid if it is for the "eosio" code account and
     * authorized by "eosio"'s "active permission */
    eosio_assert(
        code == "eosio"_n.value,
        "onerror action's are only valid from the \"eosio\" system account");
  }

 if (code == receiver || action == "onerror"_n.value) {

      switch (action) {
        // NB: Add custom method in bracets after () to use them as
        // endpoints
        EOSIO_DISPATCH_HELPER(bos_oracle, (write)(setoracles)(clear)(addoracle)(removeoracle)(ask)(once)(disable)(push)
(regservice)(unregservice)(execaction)(stakeasset)(unstakeasset)(innerpush)(pushdata)(multipush)(innerpublish)(publishdata)(multipublish)(autopublish)(addfeetypes)(addfeetype)(claim)
(subscribe)(requestdata)(payservice)(starttimer)
(regarbitrat)(complain)(uploadeviden)(uploadresult)(acceptarbi)(respcase)
(deposit)(withdraw))
      }
    }
  // print("&&&&&&&&&&&&&&&&&&&");
  if ( code == "eosio.token"_n.value && action == "transfer"_n.value ) {

    datastream<const char *> ds = datastream<const char *>(nullptr, 0);
  
    bos_oracle thiscontract(name(receiver), name(code), ds);

    
    const transferx &t = unpack_action_data <transferx> ();
    thiscontract.on_transfer(t.from, t.to, t.quantity, t.memo);
  }

}