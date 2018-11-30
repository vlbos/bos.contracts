/**
 *  @file
 *  @copyright defined in bos/LICENSE.txt
 */

#include <bos.pegtoken/bos.pegtoken.hpp>

namespace eosio {

   time_point current_time_point() {
      const static time_point ct{ microseconds{ static_cast<int64_t>( current_time() ) } };
      return ct;
   }

   uint64_t pegtoken::hash64( string s ){
      capi_checksum256  hash256;
      uint64_t res = 0;
      sha256( s.c_str(), s.length(), &hash256 );
      std::memcpy(&res, hash256.hash, 8);
      return res;
   }

   void pegtoken::init( symbol sym_base, string repeatable){
      require_auth( _self );

      eosio_assert( sym_base.is_valid(), "invalid symbol name" );
      eosio_assert( repeatable == "repeatable" || repeatable == "non-repeatable",
                    "repeatable must be repeatable or non-repeatable");

      global_singleton _global(_self, _self.value);
      eosio_assert( ! _global.exists(), "the contract has been initialized");

      global_ts _gstate{};
      _gstate.sym_base = sym_base;
      _gstate.repeatable = (repeatable == "repeatable")? true: false;
      _global.set( _gstate, _self );
   }

   const pegtoken::global_ts pegtoken::get_global(){
      global_singleton _global(_self, _self.value);
      eosio_assert( _global.exists(), "the contract needs initialization");
      return _global.get();
   }

   void pegtoken::verify_maximum_supply(asset maximum_supply){
      const auto& _gstate = get_global();
      auto sym_code = maximum_supply.symbol.code();
      eosio_assert( sym_code.is_valid(), "invalid symbol name" );
      eosio_assert( maximum_supply.is_valid(), "invalid supply");
      eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");
      eosio_assert( maximum_supply.symbol.precision() == _gstate.sym_base.precision(),
                    ("symbol precision must be " + std::to_string(_gstate.sym_base.precision())).c_str() );
      string symbol_code_str = maximum_supply.symbol.code().to_string();
      string sym_base_code_str = _gstate.sym_base.code().to_string();
      eosio_assert( symbol_code_str.length() > sym_base_code_str.length(),
                    ("symbol code length must be greater then " + std::to_string(sym_base_code_str.length()) +
                     " and start with " + sym_base_code_str).c_str() );
      eosio_assert( symbol_code_str.compare( 0, sym_base_code_str.length(), sym_base_code_str) == 0,
                    ("symbol code must start with " + sym_base_code_str).c_str() );

   }

   void pegtoken::create( name    issuer,
                        asset   maximum_supply,
                        string  organization,
                        string  website,
                        string  miner_fee,
                        string  service_fee,
                        string  unified_recharge_address,
                        string  state )
   {
      require_auth( _self );
      verify_maximum_supply( maximum_supply );

      eosio_assert( organization.size() <= 256, "organization has more than 256 bytes" );
      eosio_assert( website.size() <= 256, "website has more than 256 bytes" );
      eosio_assert( miner_fee.size() <= 256, "miner_fee has more than 256 bytes" );
      eosio_assert( service_fee.size() <= 256, "service_fee has more than 256 bytes" );
      eosio_assert( unified_recharge_address.size() <= 256, "unified_recharge_address has more than 256 bytes" );
      eosio_assert( state.size() <= 256, "state has more than 256 bytes" );
      // TODO assert organization and website not empty
      // TODO validate unified_recharge_address and state

      auto sym_code = maximum_supply.symbol.code();
      stats statstable( _self, sym_code.raw() );
      auto existing = statstable.find( sym_code.raw() );
      eosio_assert( existing == statstable.end(), "token with symbol already exists" );

      statstable.emplace( _self, [&]( auto& s ) {
         s.supply.symbol = maximum_supply.symbol;
         s.max_supply    = maximum_supply;
         s.issuer        = issuer;
         s.organization  = organization;
         s.website       = website;
         s.miner_fee     = miner_fee;
         s.service_fee   = service_fee;
         s.unified_recharge_address   = unified_recharge_address;
         s.state         = state;
      });

      symbols codestable( _self, _self.value );
      codestable.emplace( _self, [&]( auto& c ) {
         c.sym_code = maximum_supply.symbol.code();
      });
   }

   void pegtoken::setmaxsupply( asset maximum_supply )
   {
      require_auth( _self );
      verify_maximum_supply( maximum_supply );

      auto sym_code = maximum_supply.symbol.code();
      stats statstable( _self, sym_code.raw() );
      auto existing = statstable.find( sym_code.raw() );
      eosio_assert( existing != statstable.end(), "token with symbol not exists" );
      const auto& st = *existing;

      eosio_assert( maximum_supply.amount >= st.supply.amount, "max_supply amount must not less then supply amount" );
      eosio_assert( maximum_supply.amount != st.max_supply.amount, "the amount equal to current max_supply amount" );
      statstable.modify( st, same_payer, [&]( auto& s ) { s.max_supply = maximum_supply; });
   }

   void pegtoken::update( symbol_code sym_code, string parameter, string value ){
      eosio_assert( parameter.size() <= 64, "parameter has more than 64 bytes" );
      eosio_assert( value.size() <= 256, "value has more than 256 bytes" );

      stats statstable( _self, sym_code.raw() );
      auto existing = statstable.find( sym_code.raw() );
      eosio_assert( existing != statstable.end(), "token with symbol not exists" );
      const auto& st = *existing;

      require_auth( st.issuer );

      if( parameter == "organization" ){
         statstable.modify( st, same_payer, [&]( auto& s ) { s.organization = value; });
      } else if( parameter == "website" ){
         statstable.modify( st, same_payer, [&]( auto& s ) { s.website = value; });
      } else if( parameter == "miner_fee" ){
         statstable.modify( st, same_payer, [&]( auto& s ) { s.miner_fee = value; });
      } else if( parameter == "service_fee" ){
         statstable.modify( st, same_payer, [&]( auto& s ) { s.service_fee = value; });
      } else if( parameter == "unified_recharge_address" ){
         // TODO verify unified_recharge_address
         statstable.modify( st, same_payer, [&]( auto& s ) { s.unified_recharge_address = value; });
      } else if( parameter == "state" ){
         // TODO verify state
         statstable.modify( st, same_payer, [&]( auto& s ) { s.state = value; });
      } else {
         eosio_assert(false, "unknown parameter" );
      }
   }

   void pegtoken::assignaddr( symbol_code sym_code, name to, string address ){
      eosio_assert( address.size() <= 64, "address too long" );
      // TODO verify address
      eosio_assert( is_account( to ), "to account does not exist");

      stats statstable( _self, sym_code.raw() );
      auto existing = statstable.find( sym_code.raw() );
      eosio_assert( existing != statstable.end(), "token with symbol does not exist" );
      const auto& st = *existing;

      require_auth( st.issuer );

      const auto& _gstate = get_global();

      // check if this address already exist in relevant address table.
      if( _gstate.repeatable ){
         addresses t( _self, sym_code.raw() );
         auto idx = t.get_index<"address"_n>();
         auto existing = idx.find( hash64(address) );
         eosio_assert( existing == idx.end(), ("this address " + address + " has been assigned").c_str() );
      } else {
         symbols codestable( _self, _self.value );
         for( auto itr = codestable.cbegin(); itr != codestable.cend(); ++itr){
            addresses t( _self, itr->sym_code.raw() );
            auto idx = t.get_index<"address"_n>();
            auto existing = idx.find( hash64(address) );
            eosio_assert( existing == idx.end(), ("this address " + address + " has been assigned").c_str() );
         }
      }

      addresses addtable( _self, sym_code.raw() );
      auto existing2 = addtable.find( to.value );
      if( existing2 == addtable.end() ){
         addtable.emplace( _self, [&]( auto& a ) {
            a.owner = to;
            a.address = address;
            a.create_time = current_time_point();
            a.last_update = a.create_time;
         });
      } else {
         addtable.modify( existing2, same_payer, [&]( auto& a ) {
            a.owner = to;
            a.address = address;
            a.last_update = current_time_point();
         });
      }
   }

   void pegtoken::issue( name to, asset quantity, string memo )
   {
       auto sym = quantity.symbol;
       eosio_assert( sym.is_valid(), "invalid symbol name" );
       eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

       stats statstable( _self, sym.code().raw() );
       auto existing = statstable.find( sym.code().raw() );
       eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
       const auto& st = *existing;

       require_auth( st.issuer );
       eosio_assert( quantity.is_valid(), "invalid quantity" );
       eosio_assert( quantity.amount > 0, "must issue positive quantity" );

       eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
       eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

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

   void pegtoken::retire( asset quantity, string memo )
   {
       auto sym = quantity.symbol;
       eosio_assert( sym.is_valid(), "invalid symbol name" );
       eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

       stats statstable( _self, sym.code().raw() );
       auto existing = statstable.find( sym.code().raw() );
       eosio_assert( existing != statstable.end(), "token with symbol does not exist" );
       const auto& st = *existing;

       require_auth( st.issuer );
       eosio_assert( quantity.is_valid(), "invalid quantity" );
       eosio_assert( quantity.amount > 0, "must retire positive quantity" );

       eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

       statstable.modify( st, same_payer, [&]( auto& s ) {
          s.supply -= quantity;
       });

       sub_balance( st.issuer, quantity );
   }

   void pegtoken::transfer( name    from,
                          name    to,
                          asset   quantity,
                          string  memo )
   {
       eosio_assert( from != to, "cannot transfer to self" );
       require_auth( from );
       eosio_assert( is_account( to ), "to account does not exist");
       auto sym = quantity.symbol.code();
       stats statstable( _self, sym.raw() );
       const auto& st = statstable.get( sym.raw() );

       require_recipient( from );
       require_recipient( to );

       eosio_assert( quantity.is_valid(), "invalid quantity" );
       eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
       eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
       eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

       auto payer = has_auth( to ) ? to : from;

       sub_balance( from, quantity );
       add_balance( to, quantity, payer );
   }

   void pegtoken::withdraw( name from, string to, asset quantity, string memo ){
      require_auth( from );
      // TODO verify address: to

      auto sym = quantity.symbol;
      eosio_assert( sym.is_valid(), "invalid symbol name" );
      eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

      stats statstable( _self, sym.code().raw() );
      auto existing = statstable.find( sym.code().raw() );
      eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
      const auto& st = *existing;

      eosio_assert( quantity.is_valid(), "invalid quantity" );
      eosio_assert( quantity.amount > 0, "must issue positive quantity" );
      eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

      accounts acnts( _self, from.value );
      const auto& owner = acnts.get( quantity.symbol.code().raw(), "no balance object found" );
      eosio_assert( owner.balance.amount >= quantity.amount, "overdrawn balance" );

      SEND_INLINE_ACTION( *this, transfer, { { from, "active"_n} }, { from, st.issuer, quantity, "withdraw address:" + to + " memo: " + memo } );

      withdraws withdraw_table( _self, quantity.symbol.code().raw() );
      withdraw_table.emplace( _self, [&]( auto& w ){
         w.id = withdraw_table.available_primary_key();
         w.from = from;
         w.to = to;
         w.quantity = quantity;
         w.create_time = current_time_point();
         w.feedback_time = time_point_sec();
         w.state = 0;
      });
   }

   void pegtoken::feedback( symbol_code sym_code, uint64_t id, uint8_t state, string memo ){
      // TODO verify state
      eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

      stats statstable( _self, sym_code.raw() );
      auto existing = statstable.find( sym_code.raw() );
      eosio_assert( existing != statstable.end(), "token with symbol not exists" );
      const auto& st = *existing;

      require_auth( st.issuer );

      withdraws withdraw_table( _self, sym_code.raw() );
      auto existing2 = withdraw_table.find( id );
      eosio_assert( existing2 != withdraw_table.end(), "this id does not exist" );
      const auto& wt = *existing2;

      withdraw_table.modify( wt, same_payer, [&]( auto& w ) {
         w.state = state;
         w.feedback_time = current_time_point();
         w.feedback_msg = memo;
      });

      // TODO clear older history which older then three days.
   }

   void pegtoken::rollback( symbol_code sym_code, uint64_t id, string memo ){
      eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

      stats statstable( _self, sym_code.raw() );
      auto existing = statstable.find( sym_code.raw() );
      eosio_assert( existing != statstable.end(), "token with symbol not exists" );
      const auto& st = *existing;

      require_auth( st.issuer );

      withdraws withdraw_table( _self, sym_code.raw() );
      auto existing2 = withdraw_table.find( id );
      eosio_assert( existing2 != withdraw_table.end(), "this id does not exist" );
      const auto& wt = *existing2;

      accounts acnts( _self, st.issuer.value );
      const auto& owner = acnts.get( sym_code.raw(), "no balance object found" );
      eosio_assert( owner.balance.amount >= wt.quantity.amount, "issuer has not enough balance" );

      SEND_INLINE_ACTION( *this, transfer, { { st.issuer , "active"_n} }, { st.issuer, wt.from, wt.quantity, memo } );

      withdraw_table.modify( wt, same_payer, [&]( auto& w ) {
         w.state = 5;
         w.feedback_time = current_time_point();
         w.feedback_msg = memo;
      });
   }

   void pegtoken::sub_balance( name owner, asset value ) {
      accounts from_acnts( _self, owner.value );

      const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
      eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );

      from_acnts.modify( from, owner, [&]( auto& a ) {
            a.balance -= value;
         });
   }

   void pegtoken::add_balance( name owner, asset value, name ram_payer )
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

   void pegtoken::open( name owner, const symbol& symbol, name ram_payer )
   {
      require_auth( ram_payer );

      auto sym_code_raw = symbol.code().raw();

      stats statstable( _self, sym_code_raw );
      const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );
      eosio_assert( st.supply.symbol == symbol, "symbol precision mismatch" );

      accounts acnts( _self, owner.value );
      auto it = acnts.find( sym_code_raw );
      if( it == acnts.end() ) {
         acnts.emplace( ram_payer, [&]( auto& a ){
           a.balance = asset{0, symbol};
         });
      }
   }

   void pegtoken::close( name owner, const symbol& symbol )
   {
      require_auth( owner );
      accounts acnts( _self, owner.value );
      auto it = acnts.find( symbol.code().raw() );
      eosio_assert( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
      eosio_assert( it->balance.amount == 0, "Cannot close because the balance is not zero." );
      acnts.erase( it );
   }

} /// namespace eosio

EOSIO_DISPATCH( eosio::pegtoken, (init)(create)(setmaxsupply)(update)(assignaddr)(withdraw)(feedback)(rollback)(issue)(transfer)(open)(close)(retire) )
