#pragma once

#include "decoder.hpp"
#include <eosiolib/asset.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/time.hpp>
#include <string>

namespace eosio {

using transaction_id_type = capi_checksum256;

class[[eosio::contract("bos.pegtoken")]] pegtoken : public contract
{
public:
    using contract::contract;

    [[eosio::action]] void create( symbol sym, name issuer, name acceptor, name address_style, string organization, string website );

    [[eosio::action]] void update( symbol_code sym_code, string organization, string website );

    [[eosio::action]] void setlimit( asset max_limit, asset min_limit, asset total_limit, uint64_t frequency_limit, uint64_t interval_limit );

    [[eosio::action]] void setauditor( symbol_code sym_code, string action, name auditor );

    [[eosio::action]] void setfee( double service_fee_rate, asset min_service_fee, asset miner_fee );

    [[eosio::action]] void issue( asset quantity, string memo );

    [[eosio::action]] void retire( asset quantity, string memo );

    [[eosio::action]] void setpartner( symbol_code sym_code, string action, name applicant );

    [[eosio::action]] void applyaddr( name applicant, symbol_code sym_code, name to );

    [[eosio::action]] void assignaddr( symbol_code sym_code, name to, string address );

    [[eosio::action]] void withdraw( name from, string to, asset quantity, string memo );

    [[eosio::action]] void deposit( name to, asset quantity, string memo );

    [[eosio::action]] void transfer( name from, name to, asset quantity, string memo );

    [[eosio::action]] void clear( symbol_code sym_code, uint64_t num );

    [[eosio::action]] void feedback( symbol_code sym_code, transaction_id_type trx_id, string remote_trx_id, string memo );

    [[eosio::action]] void rollback( symbol_code sym_code, transaction_id_type trx_id, string memo );

    [[eosio::action]] void setacceptor( symbol_code sym_code, name acceptor );

    [[eosio::action]] void setdelay( symbol_code sym_code, uint64_t delayday );

    [[eosio::action]] void lockall( symbol_code sym_code, name auditor );

    [[eosio::action]] void unlockall( symbol_code sym_code, name auditor );

    [[eosio::action]] void approve( symbol_code sym_code, name auditor, transaction_id_type trx_id, string memo );

    [[eosio::action]] void unapprove( symbol_code sym_code, name auditor, transaction_id_type trx_id, string memo );

    [[eosio::action]] void sendback( name auditor, transaction_id_type trx_id, name to, asset quantity, string memo );

    [[eosio::action]] void rmwithdraw( uint64_t id, symbol_code sym_code );

private:
    void verify_address( name style, string address );
    void add_balance( name owner, asset value, name ram_payer );
    void sub_balance( name owner, asset value );
    asset calculate_service_fee( asset sum, double service_fee_rate, asset min_service_fee );

    bool balance_check( symbol_code sym_code, name user );
    bool addr_check( symbol_code sym_code, name user );

    struct [[eosio::table]] symbol_ts {
        symbol sym;

        uint64_t primary_key() const { return sym.code().raw(); }
    };

    struct [[eosio::table]] applicant_ts {
        name applicant;

        uint64_t primary_key() const { return applicant.value; }
    };

    struct [[eosio::table]] addr_ts {
        name owner;
        string address;
        uint64_t state;

        time_point_sec assign_time;
        time_point_sec create_time;

        uint64_t primary_key() const { return owner.value; }

        uint64_t by_addr() const { return hash64( address ); }

        uint64_t by_state() const { return state; }
    };

    struct [[eosio::table]] operate_ts {
        uint64_t id;
        name to;
        asset quantity;
        uint64_t type;
        string memo;
        time_point_sec operate_time;

        uint64_t primary_key() const { return id; }
    };

    struct [[eosio::table]] withdraw_ts {
        uint64_t id;
        transaction_id_type trx_id;
        name from;
        string to;
        asset quantity;
        uint64_t state;

        bool enable;
        name auditor;
        string remote_trx_id;
        string msg;

        time_point_sec create_time;
        time_point_sec update_time;

        uint64_t primary_key() const { return id; }

        fixed_bytes< 32 > by_trxid() const { return trxid( trx_id ); }

        uint128_t by_delindex() const
        {
            uint128_t index = ( state == 2 || state == 3 ) ? 1 : 2;
            index <<= 64;
            index += quantity.amount;
            return index;
        }

        uint128_t by_queindex() const
        {
            uint128_t index = enable ? 1 : 0;
            index <<= 32;
            index += state;
            index <<= 64;
            index += id;
            return index;
        }

        static fixed_bytes< 32 > trxid( transaction_id_type trx_id ) { return fixed_bytes< 32 >( trx_id.hash ); }
    };

    struct [[eosio::table]] deposit_ts {
        uint64_t id;
        transaction_id_type trx_id;
        name from;
        string to;
        asset quantity;
        uint64_t state;
        string remote_trx_id;
        string msg;

        time_point_sec create_time;
        time_point_sec update_time;

        uint64_t primary_key() const { return id; }

        uint64_t by_delindex() const { return create_time.utc_seconds; }
    };

    struct [[eosio::table]] statistic_ts {
        name owner;
        time_point_sec last_time;
        uint64_t frequency;
        asset total;
        time_point_sec update_time;

        uint64_t primary_key() const { return owner.value; }
    };

    struct [[eosio::table]] account_ts {
        asset balance;

        uint64_t primary_key() const { return balance.symbol.code().raw(); }
    };

    struct [[eosio::table]] stat_ts {
        asset supply;
        asset max_limit;
        asset min_limit;
        asset total_limit;
        uint64_t frequency_limit;
        uint64_t interval_limit;
        uint64_t delayday;
        name issuer;
        name acceptor;
        name address_style;
        string organization;
        string website;
        double service_fee_rate;
        asset min_service_fee;
        asset miner_fee;
        bool active;

        uint64_t primary_key() const { return supply.symbol.code().raw(); }

        uint64_t by_issuer() const { return issuer.value; }

        uint64_t by_acceptor() const { return acceptor.value; }
    };

    struct [[eosio::table]] auditor_ts {
        name auditor;

        uint64_t primary_key() const { return auditor.value; }
    };

    using symbols = eosio::multi_index< "symbols"_n, symbol_ts >;

    using applicants = eosio::multi_index< "applicants"_n, applicant_ts >;

    using addrs = eosio::multi_index< "addrs"_n, addr_ts,
        indexed_by< "addr"_n, const_mem_fun< addr_ts, uint64_t, &addr_ts::by_addr > >,
        indexed_by< "state"_n, const_mem_fun< addr_ts, uint64_t, &addr_ts::by_state > > >;

    using operates = eosio::multi_index< "operates"_n, operate_ts >;

    using withdraws = eosio::multi_index< "withdraws"_n, withdraw_ts,
        indexed_by< "trxid"_n, const_mem_fun< withdraw_ts, fixed_bytes< 32 >, &withdraw_ts::by_trxid > >,
        indexed_by< "delindex"_n, const_mem_fun< withdraw_ts, uint128_t, &withdraw_ts::by_delindex > >,
        indexed_by< "queindex"_n, const_mem_fun< withdraw_ts, uint128_t, &withdraw_ts::by_queindex > > >;

    using deposits = eosio::multi_index< "deposits"_n, deposit_ts,
        indexed_by< "delindex"_n, const_mem_fun< deposit_ts, uint64_t, &deposit_ts::by_delindex > > >;

    using statistics = eosio::multi_index< "statistics"_n, statistic_ts >;

    using accounts = eosio::multi_index< "accounts"_n, account_ts >;

    using stats = eosio::multi_index< "stats"_n, stat_ts,
        indexed_by< "issuer"_n, const_mem_fun< stat_ts, uint64_t, &stat_ts::by_issuer > >,
        indexed_by< "acceptor"_n, const_mem_fun< stat_ts, uint64_t, &stat_ts::by_acceptor > > >;

    using auditors = eosio::multi_index< "auditors"_n, auditor_ts >;
};

} // namespace eosio
