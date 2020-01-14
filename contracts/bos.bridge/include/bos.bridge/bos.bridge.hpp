#pragma once
#include <cmath>
#include <eosio/eosio.hpp>
#include "bos.config.hpp"
#include "bos.tables.hpp"



using namespace eosio;
using namespace std;


class [[eosio::contract("bos.bridge")]] bos_bridge : public eosio::contract {
 private:
   bridge_meta_parameters_singleton _bridge_meta_parameters_singleton;
   bridge_meta_parameters _bridge_meta_parameters;

 public:
   static constexpr eosio::name token_account{"eosio.token"_n};
   static constexpr eosio::name active_permission{"active"_n};
   static eosio::time_point_sec current_time_point_sec();



   using contract::contract;
   bos_bridge(name receiver, name code, datastream<const char*> ds) : contract(receiver, code, ds), _bridge_meta_parameters_singleton(_self, _self.value)
   
    {
      _bridge_meta_parameters = _bridge_meta_parameters_singleton.exists() ? _bridge_meta_parameters_singleton.get() : bridge_meta_parameters{};
   }
   ~bos_bridge() { _bridge_meta_parameters_singleton.set(_bridge_meta_parameters, _self); }

// BasicBridge  begin
   /**
    * @brief  
    */
   [[eosio::action]] void impvalidator(uint64_t requiredSignatures, std::vector<name>& initialValidators, name owner);
// BasicBridge end

// ForeignBridge  begin
   /**
    * @brief    void transferNativeToHome(name _recipient)
    */
   [[eosio::action]] void transfern2h(name sender,name recipient,uint64_t value);

   /**
    * @brief    transferTokenToHome(symbol _token, name _recipient, uint64_t _value)
    */
   [[eosio::action]] void transfert2h(name sender,std::string token, name recipient, uint64_t value);

 
    /**
    * @brief       // Event created on transfer to home.   event TransferToHome(address indexed token, address recipient, uint256 value);
    */
   [[eosio::action]] void transfer2he( const std::string& token, name recipient,uint64_t value);//


   /**
    * @brief  transferFromHome(signature sig, bytes message)
    */
   [[eosio::action]] void transferfrom(name sender,std::vector<signature> sig, bytes message);

/// ForeignBridge  end

/// HomeBridge begin

   /**
    * @brief    void registerToken(name foreignAddress, name homeAddress)
    */
   [[eosio::action]] void regtoken(name sender,std::string foreignAddress, std::string homeAddress);

   /**
    * @brief    void transferNativeToHome(name _recipient)
    */
   [[eosio::action]] void transfern2f(name sender,name recipient,uint64_t value);

   /**
    * @brief    transferTokenToHome(symbol _token, name _recipient, uint64_t _value)
    */
   [[eosio::action]] void transfert2f(name sender,std::string token, name recipient, uint64_t value);

 
    /**
    * @brief       // Event created on transfer to home.   event TransferToHome(address indexed token, address recipient, uint256 value);
    */
   [[eosio::action]] void transfer2fe(std::string token, name recipient, uint64_t value);


   /**
    * @brief  transferFromForeign(std::string foreignToken, name recipient,uint64_t value, checksum256 transactionHash) 
    */
   [[eosio::action]] void transferfrof(name sender,std::string foreignToken, name recipient,uint64_t value, checksum256 transactionHash);


    /**
    * @brief  submitSignature(name sender, public_key sender_key, signature sig,bytes message) 
    */
   [[eosio::action]] void submitsig(name sender, public_key sender_key, signature sig, bytes message) ;

/// HomeBridge end

 [[eosio::action]] void setparameter(ignore<uint8_t> version,
                              ignore<std::string> core_symbol,
                              ignore<uint8_t> precision,
                              ignore<bridge_parameters> foreign,
                              ignore<bridge_parameters> home);

 private:
   /// common
   symbol core_symbol() const {
      check(_bridge_meta_parameters.version == current_bridge_version, "config parameters must first be initialized ");
      symbol _core_symbol = symbol(symbol_code(_bridge_meta_parameters.core_symbol), _bridge_meta_parameters.precision);
      return _core_symbol;
   };
};


