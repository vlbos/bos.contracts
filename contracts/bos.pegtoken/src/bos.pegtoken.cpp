
#include "bos.pegtoken.hpp"
#include <eosiolib/transaction.hpp>

#define STRING_LEN_CHECK( str, len ) \
    eosio_assert( ( str ).size() <= len, "param " #str " too long, maximum length is " #len );

#define ACCOUNT_CHECK( account ) \
eosio_assert( is_account( account ), "invalid account " #account );

#define NIL_ACCOUNT "nil"_n

namespace eosio {


constexpr uint32_t ONE_DAY = 24 * 60 * 60;

enum withdraw_state : uint64_t {
    INITIAL_STATE = 0,
    FEED_BACK = 2,
    SEND_BACK = 3,
    ROLL_BACK = 5,
};

////////////////////////
// private funcs
////////////////////////

void pegtoken::verify_address( name style, string addr )
{
    if ( style == "bitcoin"_n ) {
        eosio_assert( valid_bitcoin_addr( addr ), "invalid bitcoin addr" );
    } else if ( style == "ethereum"_n ) {
        eosio_assert( valid_ethereum_addr( addr ), "invalid ethereum addr" );
    } else if ( style == "eosio"_n ) {
        auto _ = name( addr );
    } else if ( style == "other"_n ) {
        // no check
    } else {
        eosio_assert( false, "only EOS, BTC and ETH supported. address style must be one of bitcoin, ethereum, eosio or other" );
    }
}

void pegtoken::sub_balance( name owner, asset value )
{
    auto acct = accounts( get_self(), owner.value );
    auto from = acct.find( value.symbol.code().raw() );
    eosio_assert( from != acct.end(), "no balance object found" );
    eosio_assert( from->balance.amount >= value.amount, "overdrawn balance" );
    if ( from->balance.amount == value.amount ) {
        acct.erase( from );
    } else {
        acct.modify( from, same_payer, [&]( auto& p ) {
            p.balance -= value;
        } );
    }
}

asset pegtoken::calculate_service_fee( asset sum, double service_fee_rate, asset min_service_fee )
{
    asset actual_service_fee = sum * service_fee_rate;

    if ( actual_service_fee.amount < min_service_fee.amount ) {
        return min_service_fee;
    } else {
        return actual_service_fee;
    }
}

void pegtoken::add_balance( name owner, asset value, name ram_payer )
{
    auto acct = accounts( get_self(), owner.value );
    auto to = acct.find( value.symbol.code().raw() );
    if ( to == acct.end() ) {
        acct.emplace( ram_payer, [&]( auto& p ) {
            p.balance = value;
        } );
    } else {
        acct.modify( to, same_payer, [&]( auto& p ) {
            p.balance += value;
        } );
    }
}

bool pegtoken::balance_check( symbol_code sym_code, name user )
{
    auto acct = accounts( get_self(), user.value );
    auto balance = acct.find( sym_code.raw() );
    return balance == acct.end() || balance->balance.amount == 0;
}

bool pegtoken::addr_check( symbol_code sym_code, name user )
{
    auto addresses = addrs( get_self(), sym_code.raw() );
    return addresses.find( user.value ) == addresses.end();
}

////////////////////////
// actions
////////////////////////


void pegtoken::create( symbol sym, name issuer, name acceptor, name address_style, string organization, string website )
{
    require_auth( get_self() );

    STRING_LEN_CHECK( organization, 256 )
    STRING_LEN_CHECK( website, 256 )

    ACCOUNT_CHECK( acceptor )

    ACCOUNT_CHECK( issuer );

    eosio_assert( sym.is_valid(), "invalid symbol" );

    auto stats_table = stats( get_self(), sym.code().raw() );
    eosio_assert( stats_table.find( sym.code().raw() ) == stats_table.end(), "token with symbol already exists" );
    auto accp = stats_table.template get_index<"acceptor"_n>();
    eosio_assert( accp.find( acceptor.value ) == accp.end(), "acceptor already in use" );

    eosio_assert( address_style == "bitcoin"_n || address_style == "ethereum"_n || address_style == "eosio"_n || address_style == "other"_n,
        "address_style must be one of bitcoin, ethereum, eosio or other" );

    volatile auto tmp = stats_table.template get_index<"issuer"_n>();

    stats_table.emplace( get_self(), [&]( auto& p ) {
        p.supply = asset( 0, sym );
        p.max_limit = p.supply;
        p.min_limit = p.supply;
        p.min_service_fee = p.supply;
        p.miner_fee = p.supply;
        p.total_limit = p.supply;
        p.frequency_limit = 0;
        p.interval_limit = 300;
        p.delayday = 7;
        p.service_fee_rate = 0;
        p.issuer = issuer;
        p.acceptor = acceptor;
        p.address_style = address_style;
        p.organization = organization;
        p.website = website;
        p.active = true;
    } );

    auto syms = symbols( get_self(), get_self().value );
    syms.emplace( get_self(), [&]( auto& p ) { p.sym = sym; } );
}

void pegtoken::update( symbol_code sym_code, string organization, string website )
{
    STRING_LEN_CHECK( organization, 256 )
    STRING_LEN_CHECK( website, 256 )

    auto sym_raw = sym_code.raw();
    auto stats_table = stats( get_self(), sym_raw );
    auto iter = stats_table.find( sym_raw );
    eosio_assert( iter != stats_table.end(), "token not exist" );
    require_auth( iter->issuer );

    stats_table.modify( iter, same_payer, [&]( auto& p ) {
        p.organization = organization;
        p.website = website;
    } );
}

void pegtoken::setlimit( asset max_limit, asset min_limit, asset total_limit, uint64_t frequency_limit, uint64_t interval_limit )
{
    eosio_assert( max_limit >= min_limit && total_limit >= max_limit, "constrict mismatch: total_limit >= max_limit >= min_limit" );

    auto sym_raw = max_limit.symbol.code().raw();
    auto stats_table = stats( get_self(), sym_raw );
    auto iter = stats_table.find( sym_raw );
    eosio_assert( iter != stats_table.end(), "token not exist" );
    require_auth( iter->issuer );

    stats_table.modify( iter, same_payer, [&]( auto& p ) {
        p.max_limit = max_limit;
        p.min_limit = min_limit;
        p.total_limit = total_limit;
        p.frequency_limit = frequency_limit;
        p.interval_limit = interval_limit;
    } );
}

void pegtoken::setauditor( symbol_code sym_code, string action, name auditor )
{
    { ACCOUNT_CHECK( auditor ) };

    {
        auto sym_raw = sym_code.raw();
        auto stats_table = stats( get_self(), sym_raw );
        auto iter = stats_table.find( sym_raw );
        eosio_assert( iter != stats_table.end(), "token not exist" );
        require_auth( iter->issuer );
    }

    eosio_assert( balance_check( sym_code, auditor ), "auditor`s balance should be 0" );
    eosio_assert( addr_check( sym_code, auditor ), "auditor`s address should be null" );

    auto auds = auditors( _self, sym_code.raw() );
    if ( action == "add" ) {
        eosio_assert( auds.find( auditor.value ) == auds.end(), "auditor already exist" );
        auds.emplace( get_self(), [&]( auto& p ) {
            p.auditor = auditor;
        } );
    } else if ( action == "remove" ) {
        auto iter = auds.find( auditor.value );
        eosio_assert( iter != auds.end(), ( "auditor " + auditor.to_string() + " not exist" ).c_str() );
        auds.erase( iter );
    } else {
        eosio_assert( false, ( "invalid action: " + action ).c_str() );
    }
}

void pegtoken::setfee( double service_fee_rate, asset min_service_fee, asset miner_fee )
{
    eosio_assert( service_fee_rate >= 0 && service_fee_rate < 1 && min_service_fee.amount >= 0 && miner_fee.amount >= 0, "invalid service_fee_rate or min_service_fee or miner_fee" );
    eosio_assert( min_service_fee.symbol == miner_fee.symbol, "min_service_fee and miner_fee are not the same symbol" );

    auto sym_raw = min_service_fee.symbol.code().raw();
    auto stats_table = stats( get_self(), sym_raw );
    auto iter = stats_table.find( sym_raw );
    eosio_assert( iter != stats_table.end(), "token not exist" );
    require_auth( iter->issuer );

    stats_table.modify( iter, same_payer, [&]( auto& p ) {
        p.service_fee_rate = service_fee_rate;
        p.min_service_fee = min_service_fee;
        p.miner_fee = miner_fee;
    } );
}

void pegtoken::issue( asset quantity, string memo )
{
    STRING_LEN_CHECK( memo, 256 )

    eosio_assert( quantity.is_valid() && quantity.amount > 0, "invalid quantity" );

    auto sym_raw = quantity.symbol.code().raw();
    auto stats_table = stats( get_self(), sym_raw );
    auto iter = stats_table.find( sym_raw );
    eosio_assert( iter != stats_table.end(), "token not exist" );
    require_auth( iter->issuer );

    ACCOUNT_CHECK( iter->acceptor )
    eosio_assert( iter->active, "token is not active" );

    add_balance( iter->acceptor, quantity, iter->issuer );
    stats_table.modify( iter, same_payer, [&]( auto& p ) {
        p.supply += quantity;
        eosio_assert( p.supply.amount > 0, "supply overflow" );
    } );

    auto oper = operates( get_self(), quantity.symbol.code().raw() );
    oper.emplace( get_self(), [&]( auto& p ) {
        p.id = oper.available_primary_key();
        p.to = iter->acceptor;
        p.quantity = quantity;
        p.type = 1;
        p.operate_time = time_point_sec( now() );
        p.memo = memo;
    } );
}

void pegtoken::retire( asset quantity, string memo )
{
    STRING_LEN_CHECK( memo, 256 )

    eosio_assert( quantity.is_valid() && quantity.amount > 0, "invalid quantity" );

    auto sym_raw = quantity.symbol.code().raw();
    auto stats_table = stats( get_self(), sym_raw );
    auto iter = stats_table.find( sym_raw );
    eosio_assert( iter != stats_table.end(), "token not exist" );
    require_auth( iter->issuer );

    ACCOUNT_CHECK( iter->acceptor )
    eosio_assert( iter->active, "token is not active" );
    eosio_assert( iter->supply >= quantity, "invalid quantity" );

    sub_balance( iter->acceptor, quantity );
    stats_table.modify( iter, same_payer, [&]( auto& p ) {
        p.supply -= quantity;
    } );

    auto oper = operates( get_self(), quantity.symbol.code().raw() );
    oper.emplace( get_self(), [&]( auto& p ) {
        p.id = oper.available_primary_key();
        p.to = iter->acceptor;
        p.quantity = quantity;
        p.type = 0;
        p.operate_time = time_point_sec( now() );
        p.memo = memo;
    } );
}

void pegtoken::setpartner( symbol_code sym_code, string action, name appeallant )
{

    { ACCOUNT_CHECK( appeallant ) };

    {
        auto sym_raw = sym_code.raw();
        auto stats_table = stats( get_self(), sym_raw );
        auto iter = stats_table.find( sym_raw );
        eosio_assert( iter != stats_table.end(), "token not exist" );
        require_auth( iter->issuer );
        eosio_assert( appeallant != get_self(), "appeallant can`t be contract" );
        eosio_assert( appeallant != iter->issuer, "appeallant can`t be issuer" );
        eosio_assert( appeallant != iter->acceptor, "appeallant can`t be acceptor" );
    }

    eosio_assert( balance_check( sym_code, appeallant ), "appeallant`s balance should be 0" );
    eosio_assert( addr_check( sym_code, appeallant ), "appeallant`s address should be null" );

    auto appl = appeallants( get_self(), sym_code.raw() );
    if ( action == "add" ) {
        eosio_assert( appl.find( appeallant.value ) == appl.end(), "appeallant already exist" );
        appl.emplace( get_self(), [&]( auto& p ) {
            p.appeallant = appeallant;
        } );
    } else if ( action == "remove" ) {
        auto iter = appl.find( appeallant.value );
        eosio_assert( iter != appl.end(), "appeallant not exist" );
        appl.erase( iter );
    } else {
        eosio_assert( false, "action must be add or remove" );
    }
}

void pegtoken::applyaddr( name appeallant, symbol_code sym_code, name to )
{
    require_auth( appeallant );

    { ACCOUNT_CHECK( to ) };

    {
        auto account = to;
        auto sym_raw = sym_code.raw();
        eosio_assert( account != get_self(), "to can't be contract" );
        auto stats_table = stats( get_self(), sym_raw );
        auto iter = stats_table.find( sym_raw );
        eosio_assert( iter != stats_table.end(), "token not exist" );
        {
            eosio_assert( account != iter->issuer, "to can't be issuer" );
            eosio_assert( account != iter->acceptor, "to can't be acceptor" );
            auto appl = appeallants( get_self(), sym_raw );
            eosio_assert( appl.find( account.value ) == appl.end(), "to can't be appeallant " );
        }
    }

    auto appl = appeallants( get_self(), sym_code.raw() );
    eosio_assert( appl.find( appeallant.value ) != appl.end(), "appeallant dose not exist" );

    auto addresses = addrs( get_self(), sym_code.raw() );
    eosio_assert( addresses.find( to.value ) == addresses.end(), "to account has applied for address already" );

    addresses.emplace( get_self(), [&]( auto& p ) {
        p.owner = to;
        p.state = to.value;
        p.create_time = time_point_sec( now() );
        p.assign_time = time_point_sec( now() );
    } );
}

void pegtoken::assignaddr( symbol_code sym_code, name to, string address )
{
    ACCOUNT_CHECK( to )

    STRING_LEN_CHECK( address, 64 )

    auto account = to;
    auto sym_raw = sym_code.raw();
    eosio_assert( account != get_self(), "to can't be contract" );
    auto stats_table = stats( get_self(), sym_raw );
    auto iter = stats_table.find( sym_raw );
    eosio_assert( iter != stats_table.end(), "token not exist" );
    {
        eosio_assert( account != iter->issuer, "to can't be issuer" );
        eosio_assert( account != iter->acceptor, "to can't be acceptor" );
        auto appl = appeallants( get_self(), sym_raw );
        eosio_assert( appl.find( account.value ) == appl.end(), "to  can't be appeallant " );
    }
    require_auth( iter->acceptor );

    verify_address( iter->address_style, address );

    auto addresses = addrs( get_self(), sym_code.raw() );

    auto addr = addresses.template get_index<"addr"_n>();
    auto iter1 = addr.find( hash64( address ) );
    eosio_assert( iter1 == addr.end(), ( "this address " + address + " has been assigned to " + iter1->owner.to_string() ).c_str() );


    auto iter2 = addresses.find( to.value );
    if ( iter2 == addresses.end() ) {
        addresses.emplace( get_self(), [&]( auto& p ) {
            p.owner = to;
            p.address = address;
            p.assign_time = time_point_sec( now() );
            p.state = 0;
        } );
    } else {
        addresses.modify( iter2, same_payer, [&]( auto& p ) {
            p.address = address;
            p.assign_time = time_point_sec( now() );
            p.state = 0;
        } );
    }
}

void pegtoken::withdraw( name from, string to, asset quantity, string memo )
{
    require_auth( from );

    auto sym = quantity.symbol;

    auto account = from;
    auto sym_raw = quantity.symbol.code().raw();
    eosio_assert( account != get_self(), "from can't be contract" );
    auto stats_table = stats( get_self(), sym_raw );
    auto iter = stats_table.find( sym_raw );
    eosio_assert( iter != stats_table.end(), "token not exist" );
    {
        eosio_assert( account != iter->issuer, "from can't be issuer" );
        eosio_assert( account != iter->acceptor, "from can't be acceptor" );
        auto appl = appeallants( get_self(), sym_raw );
        eosio_assert( appl.find( account.value ) == appl.end(), "from can't be appeallant " );
    }


    eosio_assert( iter->active, "underwriter is not active" );

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must withdraw positive quantity" );
    //总额不足以支付矿工费
    eosio_assert( quantity > iter->miner_fee, ( "quantity\t" + quantity.to_string() + "\tnot greater than miner_fee" ).c_str() );

    asset service_fee = calculate_service_fee( quantity - iter->miner_fee, iter->service_fee_rate, iter->min_service_fee );
    asset residue = quantity - iter->miner_fee - service_fee;
    //总额不足以支付矿工费和服务费
    eosio_assert( residue.amount > 0, ( "quantity\t" + quantity.to_string() + "\tnot greater than the sum of miner_fee\t" + ( iter->miner_fee ).to_string() + "\tand service_fee\t" + service_fee.to_string() ).c_str() );
    eosio_assert( residue >= iter->min_limit, ( "residue\t" + residue.to_string() + "\tless than min_limit\t" + ( iter->min_limit ).to_string() ).c_str() );
    eosio_assert( residue <= iter->max_limit, ( "residue\t " + residue.to_string() + "\tgreater than max_limit\t" + ( iter->max_limit ).to_string() ).c_str() );

    STRING_LEN_CHECK( memo, 256 )
    STRING_LEN_CHECK( to, 64 )
    verify_address( iter->address_style, to );

    auto stt = statistics( get_self(), quantity.symbol.code().raw() );
    auto iter2 = stt.find( from.value );

    if ( iter2 == stt.end() ) {
        stt.emplace( get_self(), [&]( auto& p ) {
            p.owner = from;
            p.last_time = time_point_sec( now() );
            p.frequency = 1;
            p.total = quantity;
            p.update_time = p.last_time;
        } );
    } else {
        eosio_assert( iter2->last_time < time_point_sec( now() ) - iter->interval_limit, "operate twice in interval_limit" );
        eosio_assert( iter2->frequency < iter->frequency_limit, "exceed frequency_limit" );
        eosio_assert( iter2->total + quantity <= iter->total_limit, "exceed total_limit" );

        if ( iter2->last_time.utc_seconds / ONE_DAY != now() / ONE_DAY ) {
            stt.modify( iter2, same_payer, [&]( auto& p ) {
                p.last_time = time_point_sec( now() );
                p.frequency = 1;
                p.total = quantity;
                p.update_time = p.last_time;
            } );
        } else {
            stt.modify( iter2, same_payer, [&]( auto& p ) {
                p.last_time = time_point_sec( now() );
                p.frequency += 1;
                p.total += quantity;
            } );
        }
    }

    auto accs = accounts( get_self(), from.value );
    auto& owner = accs.get( quantity.symbol.code().raw(), "no balance object found" );
    eosio_assert( owner.balance >= quantity, "overdrawn balance" );

    SEND_INLINE_ACTION( *this, transfer, { { from, "active"_n } }, { from, iter->acceptor, quantity, "withdraw address:" + iter->issuer.to_string() + " memo: " + memo } );

    auto wds = withdraws( get_self(), quantity.symbol.code().raw() );
    wds.emplace( get_self(), [&]( auto& p ) {
        p.id = wds.available_primary_key();
        p.trx_id = get_trx_id();
        p.from = from;
        p.to = to;
        p.quantity = quantity;
        p.update_time = time_point_sec( now() );
        p.create_time = time_point_sec( now() );
        p.state = withdraw_state::INITIAL_STATE;
        p.enable = true;
        p.auditor = NIL_ACCOUNT;
    } );
}

void pegtoken::deposit( name to, asset quantity, string memo )
{

    STRING_LEN_CHECK( memo, 256 )

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );

    { ACCOUNT_CHECK( to ) };
    {
        auto account = to;
        auto sym_raw = quantity.symbol.code().raw();
        eosio_assert( account != get_self(), "to can't be contract" );
        auto stats_table = stats( get_self(), sym_raw );
        auto iter = stats_table.find( sym_raw );
        eosio_assert( iter != stats_table.end(), "token not exist" );
        {
            eosio_assert( account != iter->issuer, "to can't be issuer" );
            eosio_assert( account != iter->acceptor, "to can't be acceptor" );
            auto appl = appeallants( get_self(), sym_raw );
            eosio_assert( appl.find( account.value ) == appl.end(), "to can't be appeallant " );
        }
    }

    auto sym_raw = quantity.symbol.code().raw();
    auto stats_table = stats( get_self(), sym_raw );
    auto iter = stats_table.find( sym_raw );
    eosio_assert( iter != stats_table.end(), "token not exist" );
    require_auth( iter->acceptor );

    SEND_INLINE_ACTION( *this, transfer, { { iter->acceptor, "active"_n } }, { iter->acceptor, to, quantity, "deposit account:" + to.to_string() + " memo:" + memo } );

    auto depo = deposits( get_self(), quantity.symbol.code().raw() );
    depo.emplace( get_self(), [&]( auto& p ) {
        p.id = depo.available_primary_key();
        p.from = iter->acceptor;
        p.trx_id = get_trx_id();
        p.to = to.to_string();
        p.quantity = quantity;
        p.update_time = time_point_sec( now() );
        p.create_time = time_point_sec( now() );
        p.msg = memo;
    } );
}

void pegtoken::transfer( name from, name to, asset quantity, string memo )
{
    STRING_LEN_CHECK( memo, 256 )

    eosio_assert( from != to, "cannot transfer to self" );
    require_auth( from );

    { ACCOUNT_CHECK( to ) };

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );

    {
        auto sym_raw = quantity.symbol.code().raw();
        auto stats_table = stats( get_self(), sym_raw );
        auto iter = stats_table.find( sym_raw );
        eosio_assert( iter != stats_table.end(), "token not exist" );
        eosio_assert( iter->active, "underwriter is not active" );
    };

    auto payer = has_auth( to ) ? to : from;
    sub_balance( from, quantity );
    add_balance( to, quantity, payer );

    require_recipient( from );
    require_recipient( to );
}

void pegtoken::clear( symbol_code sym_code, uint64_t num )
{

    auto sym_raw = sym_code.raw();
    auto stats_table = stats( get_self(), sym_raw );
    auto iter = stats_table.find( sym_raw );
    eosio_assert( iter != stats_table.end(), "token not exist" );
    require_auth( iter->acceptor );

    {
        auto depo = deposits( get_self(), sym_code.raw() );
        auto delindex = depo.template get_index<"delindex"_n>();
        for ( auto i = 0; i < num; ++i ) {
            auto to_del = delindex.begin();
            if ( to_del == delindex.end()
                || to_del->create_time.utc_seconds + iter->delayday * ONE_DAY > now() ) {
                break;
            }
            delindex.erase( to_del );
        }
    }
    {
        auto withd = withdraws( get_self(), sym_code.raw() );
        auto delindex = withd.template get_index<"delindex"_n>();
        for ( auto i = 0; i < num; ++i ) {
            auto to_del = delindex.begin();
            if ( to_del == delindex.end()
                || to_del->create_time.utc_seconds + iter->delayday * ONE_DAY > now()
                || ( to_del->state != 2 && to_del->state != 3 )
                || to_del->quantity >= iter->min_limit ) {
                break;
            }
            delindex.erase( to_del );
        }
    }
}

void pegtoken::feedback( symbol_code sym_code, transaction_id_type trx_id, string remote_trx_id, string memo )
{

    STRING_LEN_CHECK( memo, 256 )

    auto sym_raw = sym_code.raw();
    auto stats_table = stats( get_self(), sym_raw );
    auto iter = stats_table.find( sym_raw );
    eosio_assert( iter != stats_table.end(), "token not exist" );
    require_auth( iter->acceptor );
    eosio_assert( iter->active, "underwriter is not active" );

    // TODO: check remote_trx_id

    auto withd = withdraws( get_self(), sym_code.raw() );
    auto trxids = withd.template get_index<"trxid"_n>();
    auto iter2 = trxids.find( withdraw_ts::trxid( trx_id ) );
    eosio_assert( iter2 != trxids.end(), "this trx id does not exist" );
    eosio_assert( iter2->state == INITIAL_STATE, "invalid state" );
    eosio_assert( iter2->enable == true, "cannot be processed" );

    // defer delete
    uint128_t sender_id = iter2->id;
    cancel_deferred( sender_id );
    transaction tsn;
    tsn.actions.push_back( { { get_self(), "active"_n }, get_self(), "rmwithdraw"_n,
        std::make_tuple( iter2->id, iter2->quantity.symbol.code() ) } );
    tsn.delay_sec = iter->delayday * ONE_DAY;
    tsn.send( sender_id, get_self(), true );
    trxids.modify( iter2, same_payer, [&]( auto& p ) {
        p.state = withdraw_state::FEED_BACK;
        p.remote_trx_id = remote_trx_id;
    } );
}

void pegtoken::rollback( symbol_code sym_code, transaction_id_type trx_id, string memo )
{
    auto state = 5;
    STRING_LEN_CHECK( memo, 256 )

    auto sym_raw = sym_code.raw();
    auto stats_table = stats( get_self(), sym_raw );
    auto iter = stats_table.find( sym_raw );
    eosio_assert( iter != stats_table.end(), "token not exist" );
    require_auth( iter->acceptor );
    eosio_assert( iter->active, "underwriter is not active" );

    auto withd = withdraws( get_self(), sym_code.raw() );
    auto trxids = withd.template get_index<"trxid"_n>();
    auto iter2 = trxids.find( withdraw_ts::trxid( trx_id ) );
    eosio_assert( iter2 != trxids.end(), "this trx id does not exist" );
    eosio_assert( iter2->state == INITIAL_STATE, "invalid state" );
    eosio_assert( iter2->enable == true, "cannot be processed" );

    auto acct = accounts( get_self(), iter->acceptor.value );
    auto const& owner = acct.get( sym_code.raw(), "no balance object found" );
    eosio_assert( owner.balance >= iter2->quantity, "acceptor has not enough balance" );

    // defer delete
    uint128_t sender_id = iter2->id;
    cancel_deferred( sender_id );
    transaction tsn;
    tsn.actions.push_back( { { get_self(), "active"_n }, get_self(), "rmwithdraw"_n,
        std::make_tuple( iter2->id, iter2->quantity.symbol.code() ) } );
    tsn.delay_sec = iter->delayday * ONE_DAY;
    tsn.send( sender_id, get_self(), true );

    trxids.modify( iter2, same_payer, [&]( auto& p ) {
        p.state = withdraw_state::ROLL_BACK;
    } );
}

void pegtoken::setacceptor( symbol_code sym_code, name acceptor )
{
    { ACCOUNT_CHECK( acceptor ) };

    auto sym_raw = sym_code.raw();
    auto stats_table = stats( get_self(), sym_raw );
    auto iter = stats_table.find( sym_raw );
    eosio_assert( iter != stats_table.end(), "token not exist" );
    require_auth( iter->issuer );

    eosio_assert( balance_check( sym_code, acceptor ), "acceptor`s balance should be 0" );
    eosio_assert( addr_check( sym_code, acceptor ), "acceptor`s address should be null" );

    auto acct = accounts( get_self(), acceptor.value );
    auto balance = acct.find( sym_code.raw() );
    eosio_assert( balance == acct.end() || balance->balance.amount == 0, "acceptor's balance should be 0" );

    auto accp = stats_table.template get_index<"acceptor"_n>();
    eosio_assert( accp.find( acceptor.value ) == accp.end(), "acceptor already in use" );

    stats_table.modify( iter, same_payer, [&]( auto& p ) {
        p.acceptor = acceptor;
    } );
}

void pegtoken::setdelay( symbol_code sym_code, uint64_t delayday )
{
    auto sym_raw = sym_code.raw();
    auto stats_table = stats( get_self(), sym_raw );
    auto iter = stats_table.find( sym_raw );
    eosio_assert( iter != stats_table.end(), "token not exist" );
    require_auth( iter->issuer );

    stats_table.modify( iter, same_payer, [&]( auto& p ) {
        p.delayday = delayday;
    } );
}

void pegtoken::lockall( symbol_code sym_code, name auditor )
{
    // FIXME: auth check
    auto sym_raw = sym_code.raw();
    require_auth( auditor );
    auto stats_table = stats( get_self(), sym_raw );
    auto iter = stats_table.find( sym_raw );
    eosio_assert( iter != stats_table.end(), "token not exist" );
    {
        auto auds = auditors( get_self(), sym_raw );
        eosio_assert( auds.find( auditor.value ) != auds.end(), "auditor not exist" );
    }

    eosio_assert( iter->active == true, "this token has been locked already" );
    stats_table.modify( iter, same_payer, [&]( auto& p ) { p.active = false; } );
}

void pegtoken::unlockall( symbol_code sym_code, name auditor )
{
    // FIXME: auth check
    auto sym_raw = sym_code.raw();
    require_auth( auditor );
    auto stats_table = stats( get_self(), sym_raw );
    auto iter = stats_table.find( sym_raw );
    eosio_assert( iter != stats_table.end(), "token not exist" );
    {
        auto auds = auditors( get_self(), sym_raw );
        eosio_assert( auds.find( auditor.value ) != auds.end(), "auditor not exist" );
    }

    eosio_assert( iter->active == false, "this token is not being locked" );
    stats_table.modify( iter, same_payer, [&]( auto& p ) { p.active = true; } );
}

void pegtoken::approve( symbol_code sym_code, name auditor, transaction_id_type trx_id, string memo )
{
    STRING_LEN_CHECK( memo, 256 )

    {
        auto sym_raw = sym_code.raw();
        require_auth( auditor );
        auto stats_table = stats( get_self(), sym_raw );
        auto iter = stats_table.find( sym_raw );
        eosio_assert( iter != stats_table.end(), "token not exist" );
        {
            auto auds = auditors( get_self(), sym_raw );
            eosio_assert( auds.find( auditor.value ) != auds.end(), "auditor not exist" );
        }
        eosio_assert( iter->active, "underwriter is not active" );
    }

    auto withd = withdraws( get_self(), sym_code.raw() );
    auto trxids = withd.template get_index<"trxid"_n>();
    auto iter2 = trxids.find( withdraw_ts::trxid( trx_id ) );
    eosio_assert( iter2 != trxids.end(), "invalid trx_id" );
    eosio_assert( iter2->auditor == NIL_ACCOUNT, "already been approved/unapproved" );
    trxids.modify( iter2, same_payer, [&]( auto& p ) {
        p.auditor = auditor;
        p.enable = true;
        p.msg = ( memo == "" ? p.msg : memo );
        p.update_time = time_point_sec( now() );
    } );
}

void pegtoken::unapprove( symbol_code sym_code, name auditor, transaction_id_type trx_id, string memo )
{
    {
        STRING_LEN_CHECK( memo, 256 );
        auto sym_raw = sym_code.raw();
        require_auth( auditor );
        auto stats_table = stats( get_self(), sym_raw );
        auto iter = stats_table.find( sym_raw );
        eosio_assert( iter != stats_table.end(), "token not exist" );
        {
            auto auds = auditors( get_self(), sym_raw );
            eosio_assert( auds.find( auditor.value ) != auds.end(), "auditor not exist" );
        }

        eosio_assert( iter->active, "underwriter is not active" );
    }

    auto withd = withdraws( get_self(), sym_code.raw() );
    auto trxids = withd.template get_index<"trxid"_n>();
    auto iter2 = trxids.find( withdraw_ts::trxid( trx_id ) );
    eosio_assert( iter2 != trxids.end(), "invalid trx_id" );
    eosio_assert( iter2->auditor == NIL_ACCOUNT, "already been approved/unapproved" );
    trxids.modify( iter2, same_payer, [&]( auto& p ) {
        p.auditor = auditor;
        p.enable = false;
        p.msg = ( memo == "" ? p.msg : memo );
    } );
}

void pegtoken::sendback( name auditor, transaction_id_type trx_id, name to, asset quantity, string memo )
{
    {
        STRING_LEN_CHECK( memo, 256 );
        ACCOUNT_CHECK( to );
        auto sym_raw = quantity.symbol.code().raw();
        require_auth( auditor );
        auto stats_table = stats( get_self(), sym_raw );
        auto iter = stats_table.find( sym_raw );
        eosio_assert( iter != stats_table.end(), "token not exist" );
        {
            auto auds = auditors( get_self(), sym_raw );
            eosio_assert( auds.find( auditor.value ) != auds.end(), "auditor not exist" );
        }
    }

    auto account = to;
    auto sym_raw = quantity.symbol.code().raw();
    eosio_assert( account != get_self(), "to can't be contract" );
    auto stats_table = stats( get_self(), sym_raw );
    auto iter = stats_table.find( sym_raw );
    eosio_assert( iter != stats_table.end(), "token not exist" );
    {
        eosio_assert( account != iter->issuer, "to can't be issuer" );
        eosio_assert( account != iter->acceptor, "to can't be acceptor" );
        auto appl = appeallants( get_self(), sym_raw );
        eosio_assert( appl.find( account.value ) == appl.end(), "to can't be appeallant " );
    }

    eosio_assert( iter->active, "underwriter is not active" );

    eosio_assert( quantity.amount > 0, "invalid quantity amount" );

    auto withd = withdraws( get_self(), quantity.symbol.code().raw() );
    auto trxids = withd.template get_index<"trxid"_n>();
    auto iter2 = trxids.find( withdraw_ts::trxid( trx_id ) );
    eosio_assert( iter2 != trxids.end(), "invalid trx_id" );
    eosio_assert( iter2->state == withdraw_state::ROLL_BACK, "invalid state" );
    eosio_assert( iter2->enable == true, "cannot be processed" );

    // defer delete
    uint128_t sender_id = iter2->id;
    cancel_deferred( sender_id );
    transaction tsn;
    tsn.actions.push_back( { { get_self(), "active"_n }, get_self(), "rmwithdraw"_n,
        std::make_tuple( iter2->id, iter2->quantity.symbol.code() ) } );
    tsn.delay_sec = iter->delayday * ONE_DAY;
    tsn.send( sender_id, get_self(), true );

    trxids.modify( iter2, same_payer, [&]( auto& p ) {
        p.state = withdraw_state::SEND_BACK;
        p.msg = ( memo == "" ? p.msg : memo );
        p.update_time = time_point_sec( now() );
    } );
}

void pegtoken::rmwithdraw( uint64_t id, symbol_code sym_code )
{
    require_auth2( get_self().value, ( "active"_n ).value );
    uint128_t sender_id = id;
    cancel_deferred( id );
    auto withd = withdraws( get_self(), sym_code.raw() );
    withd.erase( withd.find( id ) );
}

} // namespace eosio

EOSIO_DISPATCH(eosio::pegtoken, (create)(update)(setlimit)(setauditor)(setfee)(issue)(retire)(setpartner)(applyaddr)(assignaddr)(withdraw)(deposit)(transfer)(clear)(feedback)(rollback)(setacceptor)(setdelay)(lockall)(unlockall)(approve)(unapprove)(sendback)(rmwithdraw));

