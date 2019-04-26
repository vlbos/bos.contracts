#pragma once
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
#include "bos.oracle/tables/oracle.hpp"
#include "bos.oracle/tables/oraclize.hpp"
#include "bos.oracle/tables/riskcontrol.hpp"
#include "bos.oracle/tables/provider.hpp"
#include "bos.oracle/tables/consumer.hpp"
#include "bos.oracle/tables/arbiration.hpp"

using namespace eosio;

class [[eosio::contract("bos.oracle")]] bos_oracle : public eosio::contract {
 public:
   static constexpr eosio::name provider_account{"provider.bos"_n};
   static constexpr eosio::name consumer_account{"consumer.bos"_n};
   static constexpr eosio::name riskctrl_account{"riskctrl.bos"_n};
   static constexpr eosio::name token_account{"eosio.token"_n};
   using contract::contract;
   bos_oracle(name receiver, name code, datastream<const char *> ds)
       : contract(receiver, code, ds), requests(_self, _self.value),
         token("boracletoken"_n), oraclizes_table(_self, _self.value) {}

   // Check if calling account is a qualified oracle
   bool check_oracle(const name owner);
   // Ensure account cannot push data more often than every 60 seconds
   void check_last_push(const name owner);

   // Push oracle message on top of queue, pop oldest element if queue size is
   // larger than X
   void update_eosusd_oracle(const name owner, const uint64_t value);

   // Write datapoint
   [[eosio::action]] void write(const name owner, const uint64_t value);

   // Update oracles list
   [[eosio::action]] void setoracles(const std::vector<name> &oracleslist);

   // Clear all data
   [[eosio::action]] void clear();

   /// bos.oraclize begin
   ///
   ///
   request_table requests;
   name token;
   oracle_identities oraclizes_table;

   // oraclize(name receiver, name code, datastream<const char*> ds ) :
   // contract( receiver,  code, ds ), requests(_self, _self.value),
   // token("boracletoken"_n), oracles_table(_self, _self.value) {}

   [[eosio::action]] void addoracle(name oracle);

   [[eosio::action]] void removeoracle(name oracle);

   // @abi action
   [[eosio::action]] void ask(name administrator, name contract, string task,
                              uint32_t update_each, string memo, bytes args);

   // @abi action
   [[eosio::action]] void disable(name administrator, name contract,
                                  string task, string memo);

   // @abi action
   [[eosio::action]] void once(name administrator, name contract, string task,
                               string memo, bytes args);

   // @abi action
   [[eosio::action]] void push(name oracle, name contract, string task,
                               string memo, bytes data);

   void set(const request &value, name bill_to_account);

   using addoracle_action =
       eosio::action_wrapper<"addoracle"_n, &bos_oracle::addoracle>;
   using removeoracle_action =
       eosio::action_wrapper<"removeoracle"_n, &bos_oracle::removeoracle>;
   using ask_action = eosio::action_wrapper<"ask"_n, &bos_oracle::ask>;
   using disable_action =
       eosio::action_wrapper<"disable"_n, &bos_oracle::disable>;
   using once_action = eosio::action_wrapper<"once"_n, &bos_oracle::once>;
   using push_action = eosio::action_wrapper<"push"_n, &bos_oracle::push>;
   ///
   ///
   /// bos.oraclize end
   /// bos.provideer begin
   ///
   ///
void register(uint64_t service_id,uint64_t service_price,uint64_t fee_type,std::string data_format,
uint64_t data_type,std::string criteria,uint64_t  acceptance,std::string declaration,uint64_t injection_method,
uint64_t stake_amount,uint64_t duration,uint64_t provider_limit,uint64_t update_cycle,uint64_t update_start_time);
void unregister(uint64_t service_id,std::string signature,name account,uint64_t is_suspense);
void execaction(uint64_t service_id,uint64_t action_type);
void stakeamount(uint64_t service_id,uint64_t provider_id,name account,std::string signature,stake_amount);
void pushdata(uint64_t service_id,uint64_t update_number,uint64_t data_json,uint64_t provider_signature,uint64_t request_id);

using register_action =
    eosio::action_wrapper<"register"_n, &bos_oracle::register>;
using unregister_action =
    eosio::action_wrapper<"unregister"_n, &bos_oracle::unregister>;
using execaction_action =
    eosio::action_wrapper<"execaction"_n, &bos_oracle::execaction>;
using stakeamount_action =
    eosio::action_wrapper<"stakeamount"_n, &bos_oracle::stakeamount>;
using pushdata_action =
    eosio::action_wrapper<"pushdata"_n, &bos_oracle::pushdata>;

///
///
/// bos.provideer end
/// bos.consumer begin
///
///
void subscribe(uint64_t service_id,name contract_account,name action_name,std::string publickey,uint64_t account,uint64_t amount);
void requestdata(uint64_t update_number,uint64_t service_id,name request_signature,uint64_t request_content);

using subscribe_action =
    eosio::action_wrapper<"subscribe"_n, &bos_oracle::subscribe>;
using requestdata_action =
    eosio::action_wrapper<"requestdata"_n, &bos_oracle::requestdata>;

   ///
   ///
   /// bos.consumer end
   /// bos.riskctrl begin
   ///
   ///

   ///
   ///
   /// bos.riskctrl end
 private:

         void sub_balance( name owner, asset value );
         void add_balance( name owner, asset value, name ram_payer );
        
};


