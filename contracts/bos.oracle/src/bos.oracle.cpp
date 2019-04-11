/*

  DelphiOracle

  Author: Guillaume "Gnome" Babin-Tremblay - EOS Titan
  
  Website: https://eostitan.com
  Email: guillaume@eostitan.com

  Github: https://github.com/eostitan/delphioracle/
  
  Published under MIT License

*/

#include <eosiolib/eosio.hpp>
#include <eosiolib/fixedpoint.hpp>
#include <eosiolib/chain.h>
#include "bos.oracle/bos.oraclize.hpp"

using namespace eosio;

//Controlling account
static const name titan_account = "eostitanprod"_n;

//Number of datapoints to hold
static const uint64_t datapoints_count = 21;

//Min value set to 0.01$ , max value set to 10,000$
static const uint64_t val_min = 100;
static const uint64_t val_max = 100000000;

const uint64_t one_minute = 1000000 * 55; //give extra time for cron jobs

class [[eosio::contract("DelphiOracle")]] DelphiOracle : public eosio::contract {
 public:
 using contract::contract;
  DelphiOracle(name receiver, name code, datastream<const char*> ds ) : contract( receiver,  code, ds ){}

  //Types

  //Holds the last datapoints_count datapoints from qualified oracles
  struct [[eosio::table]] eosusd {
    uint64_t id;
    name owner; 
    uint64_t value;
    uint64_t average;
    uint64_t timestamp;

    uint64_t primary_key() const {return id;}
    uint64_t by_timestamp() const {return timestamp;}
    uint64_t by_value() const {return value;}

    EOSLIB_SERIALIZE( eosusd, (id)(owner)(value)(average)(timestamp))

  };

  //Holds the count and time of last eosusd writes for approved oracles
  struct [[eosio::table]] eosusdstats {
    name owner; 
    uint64_t timestamp;
    uint64_t count;

    uint64_t primary_key() const {return owner.value;}

  };

  //Holds the list of oracles
  struct [[eosio::table]] oracles {
    name owner;

    uint64_t primary_key() const {return owner.value;}

  };

  //Multi index types definition
  typedef eosio::multi_index<"eosusdstats"_n, eosusdstats> statstable;
  typedef eosio::multi_index<"oracles"_n, oracles> oraclestable;
  typedef eosio::multi_index<"eosusd"_n, eosusd,
      indexed_by<"value"_n, const_mem_fun<eosusd, uint64_t, &eosusd::by_value>>, 
      indexed_by<"timestamp"_n, const_mem_fun<eosusd, uint64_t, &eosusd::by_timestamp>>> usdtable;

  //Check if calling account is a qualified oracle
  bool check_oracle(const name owner){

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
  void check_last_push(const name owner){

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
  void update_eosusd_oracle(const name owner, const uint64_t value){

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
  [[eosio::action]]
  void write(const name owner, const uint64_t value) {
    
    require_auth(owner);

    eosio_assert(value >= val_min && value <= val_max, "value outside of allowed range");
    eosio_assert(check_oracle(owner), "account is not an active producer or approved oracle");
    
    check_last_push(owner);
    update_eosusd_oracle(owner, value);
    
  }

  //Update oracles list
  [[eosio::action]]
  void setoracles(const std::vector<name>& oracleslist) {
    
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
  [[eosio::action]]
  void clear() {
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

};

EOSIO_DISPATCH(DelphiOracle, (write)(setoracles)(clear))
