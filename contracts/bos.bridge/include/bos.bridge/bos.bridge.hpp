#pragma once

#include "bos.config.hpp"
#include <cmath>
#include <eosio/eosio.hpp>
using namespace eosio;
using namespace std;


class [[eosio::contract("bos.bridge")]] bos_bridge : public eosio::contract {
 private:
   bridge_meta_parameters_singleton _bridge_meta_parameters_singleton;
   bridge_meta_parameters _bridge_meta_parameters;

 public:
   static constexpr eosio::name token_account{"eosio.token"_n};
   static constexpr eosio::name active_permission{"active"_n};

   static time_point_sec current_time_point_sec();


   using contract::contract;
   bos_bridge(name receiver, name code, datastream<const char*> ds) : contract(receiver, code, ds), _bridge_meta_parameters_singleton(_self, _self.value) {
      _bridge_meta_parameters = _bridge_meta_parameters_singleton.exists() ? _bridge_meta_parameters_singleton.get() : bridge_meta_parameters{};
   }
   ~bos_bridge() { _bridge_meta_parameters_singleton.set(_bridge_meta_parameters, _self); }


   /**
    * @brief  
    */
   [[eosio::action]] void regvalidator(name chain, name account);

   /**
    * @brief  transfer to foreign   transferToForeign(string recipient, uint amount)
    */
   [[eosio::action]] void transfertf(string recipient, string amount);

   /**
    * @brief   transfer to foreign event  TransferToForeign (string recipient, uint256 value)
    */
   [[eosio::action]] void transfertfe(string recipient, string value);


   /**
    * @brief  Sets config parameters
    */
   [[eosio::action]] void setparameter(ignore<uint8_t> version, ignore<bridge_parameters> parameters);

   using regservice_action = eosio::action_wrapper<"regservice"_n, &bos_bridge::regservice>;
   using unstakeasset_action = eosio::action_wrapper<"unstakeasset"_n, &bos_bridge::unstakeasset>;
   using addfeetypes_action = eosio::action_wrapper<"addfeetypes"_n, &bos_bridge::addfeetypes>;
   using pushdata_action = eosio::action_wrapper<"pushdata"_n, &bos_bridge::pushdata>;



   [[eosio::action]] void deposit(name chain,name account,  asset amount);

   /**
    * @brief  Withdraws core tokens from bridge risk control fund
    */
   [[eosio::action]] void withdraw(name chain,name account, asset amount);

   using deposit_action = eosio::action_wrapper<"deposit"_n, &bos_bridge::deposit>;
   using withdraw_action = eosio::action_wrapper<"withdraw"_n, &bos_bridge::withdraw>;


 private:
   /// common
   symbol core_symbol() const {
      check(_bridge_meta_parameters.version == current_bridge_version, "config parameters must first be initialized ");
      auto paras = unpack<bridge_parameters>(_bridge_meta_parameters.parameters_data);
      symbol _core_symbol = symbol(symbol_code(paras.core_symbol), paras.precision);
      return _core_symbol;
   };
};
