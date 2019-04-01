#pragma once

#include <eosiolib/eosio.hpp>
#include <cmath>

namespace eosio {

static constexpr symbol BOS_SYMBOL = symbol("BOS", 4);
static constexpr symbol EOS_SYMBOL = symbol("EOS", 4);
static constexpr symbol RAMCORE_SYMBOL = symbol("RAMCORE", 4);
static constexpr symbol RAM_SYMBOL = symbol("RAM", 0);

static constexpr name BOS_CONTRACT = name{"eosio.token"};
static constexpr name EOS_CONTRACT = name{"eostk.ibc"};

using weight_type = uint16_t;
using real_type = double;

struct key_weight {
    public_key key;
    weight_type weight;
};

struct permission_level_weight {
    permission_level permission;
    weight_type weight;
};

struct wait_weight {
    uint32_t wait_sec;
    weight_type weight;
};

struct authority {
    uint32_t threshold;
    vector<key_weight> keys;
    vector<permission_level_weight> accounts;
    vector<wait_weight> waits;
};

struct newaccount {
    capi_name creator;
    capi_name name;
    authority owner;
    authority active;
};

struct exchange_state {
    asset supply;

    struct connector {
        asset balance;
        double weight = .5;

        EOSLIB_SERIALIZE( connector, (balance)(weight) )
    };

    connector base;
    connector quote;

    uint64_t primary_key()const { return supply.symbol.raw(); }

    EOSLIB_SERIALIZE( exchange_state, (supply)(base)(quote) )

    asset convert_to_exchange( connector& c, asset in ) {
        real_type R(supply.amount);
        real_type C(c.balance.amount+in.amount);
        real_type F(c.weight/1000.0);
        real_type T(in.amount);
        real_type ONE(1.0);

        real_type E = -R * (ONE - std::pow( ONE + T / C, F) );
        //print( "E: ", E, "\n");
        int64_t issued = int64_t(E);

        supply.amount += issued;
        c.balance.amount += in.amount;

        return asset( issued, supply.symbol );
    }

    asset convert_from_exchange( connector& c, asset in ) {
        eosio_assert( in.symbol== supply.symbol, "unexpected asset symbol input" );

        real_type R(supply.amount - in.amount);
        real_type C(c.balance.amount);
        real_type F(1000.0/c.weight);
        real_type E(in.amount);
        real_type ONE(1.0);


        // potentially more accurate:
        // The functions std::expm1 and std::log1p are useful for financial calculations, for example,
        // when calculating small daily interest rates: (1+x)n
        // -1 can be expressed as std::expm1(n * std::log1p(x)).
        // real_type T = C * std::expm1( F * std::log1p(E/R) );

        real_type T = C * (std::pow( ONE + E/R, F) - ONE);
        //print( "T: ", T, "\n");
        int64_t out = int64_t(T);

        supply.amount -= in.amount;
        c.balance.amount -= out;

        return asset( out, c.balance.symbol );
    }

    asset convert( asset from, symbol to ) {
        auto sell_symbol  = from.symbol;
        auto ex_symbol    = supply.symbol;
        auto base_symbol  = base.balance.symbol;
        auto quote_symbol = quote.balance.symbol;

        //print( "From: ", from, " TO ", asset( 0,to), "\n" );
        //print( "base: ", base_symbol, "\n" );
        //print( "quote: ", quote_symbol, "\n" );
        //print( "ex: ", supply.symbol, "\n" );

        if( sell_symbol != ex_symbol ) {
            if( sell_symbol == base_symbol ) {
                from = convert_to_exchange( base, from );
            } else if( sell_symbol == quote_symbol ) {
                from = convert_to_exchange( quote, from );
            } else {
                eosio_assert( false, "invalid sell" );
            }
        } else {
            if( to == base_symbol ) {
                from = convert_from_exchange( base, from );
            } else if( to == quote_symbol ) {
                from = convert_from_exchange( quote, from );
            } else {
                eosio_assert( false, "invalid conversion" );
            }
        }

        if( to != from.symbol )
            return convert( from, to );

        return from;
    }
};

using rammarket = multi_index<"rammarket"_n, exchange_state>;


asset buyrambytes(uint32_t bytes) {
    rammarket market(name{"eosio"}, name{"eosio"}.value);
    auto it = market.find(RAMCORE_SYMBOL.raw());
    eosio_assert(it != market.end(), "RAMCORE market not found");
    auto tmp = *it;
    return tmp.convert(asset(bytes, RAM_SYMBOL), BOS_SYMBOL);
}

} // endof namespace eosio

