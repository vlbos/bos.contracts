#pragma once
#include <cmath>
#include <eosio/eosio.hpp>
#include "bos.config.hpp"
#include "bos.tables.hpp"


using namespace eosio;
using namespace std;


class [[eosio::contract("bos.bridge")]] bos_bridge : public contract {
 private:
   bridge_meta_parameters_singleton _bridge_meta_parameters_singleton;
   bridge_meta_parameters _bridge_meta_parameters;

 public:
   static constexpr eosio::name token_account{"eosio.token"_n};
   static constexpr eosio::name active_permission{"active"_n};
   static eosio::time_point_sec current_time_point_sec();

   using eosio::contract::contract;
   bos_bridge(name receiver, name code, datastream<const char*> ds) : eosio::contract(receiver, code, ds), _bridge_meta_parameters_singleton(_self, _self.value)
   
    {
      _bridge_meta_parameters = _bridge_meta_parameters_singleton.exists() ? _bridge_meta_parameters_singleton.get() : bridge_meta_parameters{};
   }
   ~bos_bridge() { _bridge_meta_parameters_singleton.set(_bridge_meta_parameters, _self); }

// BasicBridge  begin
   /**
    * @brief  
    */
   [[eosio::action]] void impvalidator(uint64_t requiredSignatures, std::vector<std::pair<name,public_key>>& initialValidators, name owner);
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
    * @brief  submitSignature(name sender, std::string sender_key, signature sig,bytes message) 
    */
   [[eosio::action]] void submitsig(name sender, public_key sender_key, signature sig, bytes message) ;

   

   /**
    * @brief   event CollectedSignatures(address authorityResponsibleForRelay, bytes32 messageHash, uint256 NumberOfCollectedSignatures);
    */
   [[eosio::action]] void collectedsig(name sender, checksum256 messageHash, uint64_t NumberOfCollectedSignatures) ;

/// HomeBridge end
 [[eosio::action]] void settokenpara(name sender,bool ishome ,std::string token, uint64_t minPerTx,
                        uint64_t maxPerTx,uint64_t dailyLimit);

 [[eosio::action]] void setparameter(ignore<uint8_t> version,
                              ignore<std::string> core_symbol,
                              ignore<uint8_t> precision,
                              ignore<bridge_parameters> foreign,
                              ignore<bridge_parameters> home);


[[eosio::action]] void test(name sender, public_key sender_key, signature sig, bytes message);

 static  symbol str2sym(std::string token){ 
    const uint64_t size = 3;
    std::vector<std::string> vec = split(token);
    check(vec.size()== size,"wrong format ");
  
   return symbol(symbol_code(vec[1]), std::stoi(vec[2]));
    
 }

  static  name str2contract(std::string token){ 
    const uint64_t size = 3;
    std::vector<std::string> vec = split(token);
    check(vec.size()== size,"wrong format ");
  
   return name(vec[0]);
    
 }
 [[eosio::action]]
         void create( name   issuer,
                      asset  maximum_supply);

         [[eosio::action]]
         void issue( name to, asset quantity, string memo );

         [[eosio::action]]
         void retire( asset quantity, string memo );

         [[eosio::action]]
         void transfer( name    from,
                        name    to,
                        asset   quantity,
                        string  memo );

         [[eosio::action]]
         void open( name owner, const symbol& symbol, name ram_payer );

         [[eosio::action]]
         void close( name owner, const symbol& symbol );

         [[eosio::action]] 
         void addblacklist(const std::vector<name>& accounts);

         [[eosio::action]] 
         void rmblacklist(const std::vector<name>& accounts);

         static asset get_supply( name token_contract_account, symbol_code sym_code )
         {
            stats statstable( token_contract_account, sym_code.raw() );
            const auto& st = statstable.get( sym_code.raw() );
            return st.supply;
         }

         static asset get_balance( name token_contract_account, name owner, symbol_code sym_code )
         {
            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym_code.raw() );
            return ac.balance;
         }

         using create_action = eosio::action_wrapper<"create"_n, &bos_bridge::create>;
         using issue_action = eosio::action_wrapper<"issue"_n, &bos_bridge::issue>;
         using retire_action = eosio::action_wrapper<"retire"_n, &bos_bridge::retire>;
         using transfer_action = eosio::action_wrapper<"transfer"_n, &bos_bridge::transfer>;
         using open_action = eosio::action_wrapper<"open"_n, &bos_bridge::open>;
         using close_action = eosio::action_wrapper<"close"_n, &bos_bridge::close>;
      private:
         struct [[eosio::table]] account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };

         struct [[eosio::table]] currency_stats {
            asset    supply;
            asset    max_supply;
            name     issuer;

            uint64_t primary_key()const { return supply.symbol.code().raw(); }
         };

         ///bos begin
         struct [[eosio::table]] account_blacklist
         {
            name account;
            uint64_t primary_key() const { return account.value; }
         };

         typedef eosio::multi_index<"blacklist"_n, account_blacklist> blacklist;
         static const uint8_t blacklist_limit_size = 100 ;
         ///bos end
         typedef eosio::multi_index< "accounts"_n, account > accounts;
         typedef eosio::multi_index< "stat"_n, currency_stats > stats;

         void sub_balance( name owner, asset value );
         void add_balance( name owner, asset value, name ram_payer );

 private:
   
   /// common
   symbol core_symbol() const {
      check(_bridge_meta_parameters.version == current_bridge_version, "config parameters must first be initialized ");
      symbol _core_symbol = symbol(symbol_code(_bridge_meta_parameters.core_symbol), _bridge_meta_parameters.precision);
      return _core_symbol;
   };
};


