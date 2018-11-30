/**
 *  @file
 *  @copyright defined in bos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/singleton.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   class [[eosio::contract("bos.pegtoken")]] pegtoken : public contract {
      public:
         using contract::contract;

         [[eosio::action]]
         void init( symbol sym_base, string repeatable);

         [[eosio::action]]
         void create( name    issuer,
                      asset   maximum_supply,
                      string  organization,
                      string  website,
                      string  miner_fee,
                      string  service_fee,
                      string  unified_recharge_address,
                      string  state);

         [[eosio::action]]
         void setmaxsupply( asset maximum_supply );

         [[eosio::action]]
         void update( symbol_code sym_code,
                      string  parameter,
                      string  value );

         [[eosio::action]]
         void assignaddr( symbol_code  sym_code,
                          name         to,
                          string       address );

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
         void withdraw( name    from,
                        string  to,
                        asset   quantity,
                        string  memo );

         [[eosio::action]]
         void feedback( symbol_code  sym_code,
                        uint64_t     id,
                        uint8_t      state,
                        string       memo );

         [[eosio::action]]
         void rollback( symbol_code  sym_code,
                        uint64_t     id,
                        string       memo );

         [[eosio::action]]
         void open( name owner, const symbol& symbol, name ram_payer );

         [[eosio::action]]
         void close( name owner, const symbol& symbol );

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

      private:
         struct [[eosio::table("global")]] global_ts {
            symbol sym_base;
            bool   repeatable;   // recharge addresses can be used repeatably between different organizations.
         };

         struct [[eosio::table]] symbol_code_ts {
            symbol_code  sym_code;

            uint64_t primary_key()const { return sym_code.raw(); }
         };

         struct [[eosio::table]] recharge_address_ts {
            name           owner;
            string         address;
            time_point_sec create_time;
            time_point_sec last_update;

            uint64_t primary_key()const { return owner.value; }
            uint64_t by_address()const { return hash64( address ); }
         };

         struct [[eosio::table]] withdraw_ts {
            uint64_t       id;
            name           from;
            string         to;
            asset          quantity;
            time_point_sec create_time;
            time_point_sec feedback_time;
            string         feedback_msg;
            uint8_t        state;

            uint64_t primary_key()const { return id; }
            uint64_t by_time()const { return static_cast<uint64_t>(create_time.sec_since_epoch()); }
         };

         struct [[eosio::table]] account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };

         struct [[eosio::table]] currency_stats {
            asset   supply;
            asset   max_supply;
            name    issuer;
            string  organization;
            string  website;
            string  miner_fee;
            string  service_fee;
            string  unified_recharge_address;
            string  state;

            uint64_t primary_key()const { return supply.symbol.code().raw(); }
         };

         typedef eosio::singleton< "global"_n, global_ts > global_singleton;
         typedef eosio::multi_index< "symcodes"_n, symbol_code_ts > symcodes;
         typedef eosio::multi_index< "rchrgaddr"_n, recharge_address_ts ,
            indexed_by<"address"_n, const_mem_fun<recharge_address_ts, uint64_t, &recharge_address_ts::by_address>  >
         > addresses;
         typedef eosio::multi_index< "withdraws"_n, withdraw_ts,
            indexed_by<"time"_n, const_mem_fun<withdraw_ts, uint64_t, &withdraw_ts::by_time>  >
         > withdraws;
         typedef eosio::multi_index< "accounts"_n, account > accounts;
         typedef eosio::multi_index< "stat"_n, currency_stats > stats;

         void sub_balance( name owner, asset value );
         void add_balance( name owner, asset value, name ram_payer );

         static uint64_t hash64( string str );
         const global_ts get_global();
         void verify_maximum_supply(asset maximum_supply);
   };

} /// namespace eosio
