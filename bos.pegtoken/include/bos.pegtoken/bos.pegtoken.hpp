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
         void approve( symbol_code  sym_code ,
                       uint64_t     issue_id );

         [[eosio::action]]
         void unapprove( symbol_code  sym_code ,
                         uint64_t     issue_id );

         [[eosio::action]]
         void retire( asset quantity, string memo );

         [[eosio::action]]
         void transfer( name    from,
                        name    to,
                        asset   quantity,
                        string  memo );

         [[eosio::action]]
         void lockall( symbol_code sym_code );

         [[eosio::action]]
         void unlockall( symbol_code sym_code );

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
            uint64_t       apply_num;

            uint64_t primary_key()const { return owner.value; }
            uint64_t by_address()const { return hash64( address ); }
            uint64_t by_apply_num()const { return apply_num; }
         };

         struct [[eosio::table]] issue_ts {
            uint64_t issue_id;
            name     to;
            asset    quantity;
            string   memo

            uint64_t primary_key()const { return issue_id; }
         };

         struct [[eosio::table]] withdraw_ts {
            uint64_t             seq_num;
            transaction_id_type  trx_id;
            string               feedback_trx_id;
            uint8_t              feedback_state;
            string               feedback_msg;
            time_point_sec       feedback_time;

            uint64_t  primary_key()const { return seq_num; }
            uint256_t by_trxid()const { return static_cast<uint256_t>(trx_id); }
         };

         struct [[eosio::table]] account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };

         struct [[eosio::table]] currency_stats {
            asset   supply;
            asset   max_supply;
            asset   large_asset;
            name    issuer;
            name    auditor;
            name    address_style;
            string  organization;
            string  website;
            string  miner_fee;
            string  service_fee;
            string  unified_recharge_address;
            bool    active;

            uint64_t primary_key()const { return supply.symbol.code().raw(); }
         };

         typedef eosio::singleton< "global"_n, global_ts > global_singleton;
         typedef eosio::multi_index< "applicants"_n, applicant_ts > applicants;
         typedef eosio::multi_index< "symbols"_n, symbol_ts > symbols;
         typedef eosio::multi_index< "rchrgaddr"_n, recharge_address_ts ,
            indexed_by<"address"_n, const_mem_fun<recharge_address_ts, uint64_t, &recharge_address_ts::by_address> >,
            indexed_by<"applynum"_n, const_mem_fun<recharge_address_ts, uint64_t, &recharge_address_ts::by_apply_num> >
         > addresses;
         typedef eosio::multi_index< "withdraws"_n, withdraw_ts,
            indexed_by<"trxid"_n, const_mem_fun<withdraw_ts, uint256_t, &withdraw_ts::by_trxid>  >
         > withdraws;
         typedef eosio::multi_index< "accounts"_n, account > accounts;
         typedef eosio::multi_index< "stat"_n, currency_stats > stats;

         void sub_balance( name owner, asset value );
         void add_balance( name owner, asset value, name ram_payer );
         static uint64_t hash64( string str );
   };

} /// namespace eosio
