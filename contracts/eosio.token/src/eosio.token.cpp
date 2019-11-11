/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio.token/eosio.token.hpp>

namespace eosio {

void token::create( name   issuer,
                    asset  maximum_supply )
{
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( maximum_supply.is_valid(), "invalid supply");
    check( maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( _self, [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
    });
}


void token::issue( name to, asset quantity, string memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

    require_auth( st.issuer );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must issue positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    check( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {
      SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} },
                          { st.issuer, to, quantity, memo }
      );
    }
}
////bos burn begin
void token::burn(name executer,name from, asset quantity, string memo) {
   auto sym = quantity.symbol;
   check(sym.is_valid(), "Invalid symbol name");
   check(memo.size() <= 256, "Memo must be less than 256 characters");

   auto sym_code_raw = sym.code().raw();

   stats statstable(_self, sym_code_raw);
   auto existing = statstable.find(sym_code_raw);
   check(existing != statstable.end(), "Token with that symbol name does not exist - Please create the token before burning");

   const auto& st = *existing;
   require_auth(executer);
   require_recipient(from);
   check(quantity.is_valid(), "Invalid quantity value");
   check(quantity.amount > 0, "Quantity value must be positive");

   check(st.supply.symbol == quantity.symbol, "Symbol precision mismatch");
   check(st.supply.amount >= quantity.amount, "Quantity value cannot exceed the available supply");
   check(st.max_supply.symbol == quantity.symbol, "Max supply symbol precision mismatch");
   check(st.max_supply.amount >= quantity.amount, "Quantity value cannot exceed the available max supply");

   statstable.modify(st, same_payer, [&](auto& s) {
      s.supply -= quantity;
      s.max_supply -= quantity; // this line is added compared to `token::retire`
   });

   // sub_balance(from, quantity);
}
////bos burn end

void token::retire( asset quantity, string memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing != statstable.end(), "token with symbol does not exist" );
    const auto& st = *existing;

    require_auth( st.issuer );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must retire positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply -= quantity;
    });

    sub_balance( st.issuer, quantity );
}

void token::transfer( name    from,
                      name    to,
                      asset   quantity,
                      string  memo )
{
   blacklist blacklisttable(_self, _self.value);
   check(blacklisttable.find(from.value) == blacklisttable.end(), "account is on the blacklist"); ///bos
   check(from != to, "cannot transfer to self");
   require_auth(from);
   check(is_account(to), "to account does not exist");
   auto sym = quantity.symbol.code();
   stats statstable(_self, sym.raw());
   const auto &st = statstable.get(sym.raw());

   require_recipient(from);
   require_recipient(to);

   check(quantity.is_valid(), "invalid quantity");
   check(quantity.amount > 0, "must transfer positive quantity");
   check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
   check(memo.size() <= 256, "memo has more than 256 bytes");

   auto payer = has_auth(to) ? to : from;

   sub_balance(from, quantity);
   add_balance(to, quantity, payer);
}

void token::sub_balance( name owner, asset value ) {
   accounts from_acnts( _self, owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   check( from.balance.amount >= value.amount, "overdrawn balance" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
         a.balance -= value;
      });
}

void token::add_balance( name owner, asset value, name ram_payer )
{
   accounts to_acnts( _self, owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

void token::open( name owner, const symbol& symbol, name ram_payer )
{
   require_auth( ram_payer );

   auto sym_code_raw = symbol.code().raw();

   stats statstable( _self, sym_code_raw );
   const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );
   check( st.supply.symbol == symbol, "symbol precision mismatch" );

   accounts acnts( _self, owner.value );
   auto it = acnts.find( sym_code_raw );
   if( it == acnts.end() ) {
      acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = asset{0, symbol};
      });
   }
}

void token::close( name owner, const symbol& symbol )
{
   require_auth( owner );
   accounts acnts( _self, owner.value );
   auto it = acnts.find( symbol.code().raw() );
   check( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   check( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   acnts.erase( it );
}

///bos begin
void token::addblacklist(const std::vector<name>& accounts)
{
   require_auth(_self);

   eosio_assert(blacklist_limit_size >= accounts.size(), "accounts' size must be less than 100.");
   static const std::string msg = std::string(" account does not exist");
   bool is_executed = false;
   for (auto acc : accounts){
      std::string m = acc.to_string() + msg;
      eosio_assert(is_account(acc), m.c_str());
      blacklist blacklisttable(_self, _self.value);
      auto it = blacklisttable.find(acc.value);
      if (it == blacklisttable.end()) {
         blacklisttable.emplace(_self, [&](auto &a) {
            a.account = acc;
            is_executed = true;
         });
      }
   }

   eosio_assert( is_executed, "all accounts were on blacklist." );
}

void token::rmblacklist(const std::vector<name>& accounts)
{
   require_auth(_self);

   eosio_assert( blacklist_limit_size>=accounts.size(), "accounts' size must be less than 100." );
   bool is_executed = false;
   for (auto acc : accounts){
      blacklist blacklisttable(_self, _self.value);
      auto it = blacklisttable.find(acc.value);
      if (it != blacklisttable.end()){
         blacklisttable.erase(it);
         is_executed = true;
      }
   }

   eosio_assert( is_executed, "all accounts were not on blacklist." );
}
///bos end

} /// namespace eosio

EOSIO_DISPATCH( eosio::token, (create)(issue)(transfer)(open)(close)(retire)(addblacklist)(rmblacklist)(burn) )
