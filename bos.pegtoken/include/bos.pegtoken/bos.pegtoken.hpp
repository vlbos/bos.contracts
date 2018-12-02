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

namespace eosio {
   typedef capi_checksum256   transaction_id_type;
   using std::string;

   class [[eosio::contract("bos.pegtoken")]] pegtoken : public contract {
      public:
         using contract::contract;

         [[eosio::action]]
         void create( name    issuer,
                      name    auditor,
                      asset   maximum_supply,
                      asset   large_asset,
                      name    address_style,
                      string  organization,
                      string  website,
                      string  miner_fee,
                      string  service_fee,
                      string  unified_recharge_address,
                      bool    active );

         [[eosio::action]]
         void setmaxsupply( asset maximum_supply );

         [[eosio::action]]
         void setlargeast( asset large_asset );

         [[eosio::action]]
         void lockall( symbol_code sym_code );

         [[eosio::action]]
         void unlockall( symbol_code sym_code );

         [[eosio::action]]
         void update( symbol_code sym_code,
                      string  parameter,
                      string  value );

         [[eosio::action]]
         void applicant( symbol_code   sym_code,
                         name          action,
                         name          applicant );

         [[eosio::action]]
         void applyaddr( name          applicant,
                         name          to,
                         symbol_code   sym_code );

         [[eosio::action]]
         void assignaddr( symbol_code  sym_code,
                          name         to,
                          string       address );

         [[eosio::action]]
         void issue( uint64_t seq_num, name to, asset quantity, string memo );

         [[eosio::action]]
         void approve( symbol_code  sym_code ,
                       uint64_t     issue_seq_num );

         [[eosio::action]]
         void unapprove( symbol_code  sym_code ,
                         uint64_t     issue_seq_num );

         [[eosio::action]]
         void retire( asset quantity, string memo );

         [[eosio::action]]
         void transfer( name    from,
                        name    to,
                        asset   quantity,
                        string  memo );



         [[eosio::action]]
         void withdraw( name    from,
                        string  to_address,
                        asset   quantity,
                        string  memo );

         [[eosio::action]]
         void feedback( symbol_code  sym_code,
                        uint64_t     id,
                        name         state,
                        string       trx_id,
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

         struct [[eosio::table]] symbol_ts {
            symbol  sym;

            uint64_t primary_key()const { return sym.code().raw(); }
         };

         struct [[eosio::table]] applicant_ts {
            name  applicant;

            uint64_t primary_key()const { return applicant.value; }
         };

         struct [[eosio::table]] recharge_address_ts {
            name           owner;
            string         address;
            time_point_sec assign_time;
            uint64_t       state;

            uint64_t primary_key()const { return owner.value; }
            uint64_t by_address()const { return hash64( address ); }
            uint64_t by_state()const { return state; }
         };

         struct [[eosio::table]] issue_ts {
            uint64_t seq_num;
            name     to;
            asset    quantity;
            string   memo;

            uint64_t primary_key()const { return seq_num; }
         };

         struct [[eosio::table]] withdraw_ts {
            uint64_t             id;
            transaction_id_type  trx_id;
            name                 from;
            string               to;
            asset                quantity;
            name                 state;
            string               feedback_trx_id;
            string               feedback_msg;
            time_point_sec       feedback_time;

            uint64_t  primary_key()const { return id; }
            uint64_t by_trxid()const { return static_cast<uint64_t>(/*trx_id*/0); }  // TODO 256
         };

         struct [[eosio::table]] account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };

         struct [[eosio::table]] currency_stats {
            asset    supply;
            asset    max_supply;
            asset    large_asset;
            name     issuer;
            name     auditor;
            name     address_style;
            string   organization;
            string   website;
            string   miner_fee;
            string   service_fee;
            string   unified_recharge_address;
            bool     active;

            uint64_t issue_seq_num;

            uint64_t primary_key()const { return supply.symbol.code().raw(); }
         };

         typedef eosio::multi_index< "applicants"_n, applicant_ts > applicants;
         typedef eosio::multi_index< "symbols"_n, symbol_ts > symbols;
         typedef eosio::multi_index< "issues"_n, issue_ts > issues;
         typedef eosio::multi_index< "rchrgaddr"_n, recharge_address_ts ,
            indexed_by<"address"_n, const_mem_fun<recharge_address_ts, uint64_t, &recharge_address_ts::by_address> >,
            indexed_by<"state"_n, const_mem_fun<recharge_address_ts, uint64_t, &recharge_address_ts::by_state> >
         > addresses;
         typedef eosio::multi_index< "withdraws"_n, withdraw_ts,
            indexed_by<"trxid"_n, const_mem_fun<withdraw_ts, uint64_t, &withdraw_ts::by_trxid>  >
         > withdraws;
         typedef eosio::multi_index< "accounts"_n, account > accounts;
         typedef eosio::multi_index< "stat"_n, currency_stats > stats;

         void sub_balance( name owner, asset value );
         void add_balance( name owner, asset value, name ram_payer );

         void verify_maximum_supply(asset maximum_supply);
         void verify_address( name style, string addr);
         void issue_handle( symbol_code sym_code, uint64_t issue_seq_num, bool pass);
         static uint64_t hash64( string str );

   };

} /// namespace eosio
