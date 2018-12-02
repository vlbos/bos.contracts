/**
 *  @file
 *  @copyright defined in bos/LICENSE.txt
 */

#include <bos.pegtoken/bos.pegtoken.hpp>
#include <eosiolib/transaction.hpp>

namespace eosio {

   time_point current_time_point() {
      const static time_point ct{ microseconds{ static_cast<int64_t>( current_time() ) } };
      return ct;
   }

   capi_checksum256 get_trx_id(){
      capi_checksum256 trx_id;
      std::vector<char> trx_bytes;
      size_t trx_size = transaction_size();
      trx_bytes.resize(trx_size);
      read_transaction(trx_bytes.data(), trx_size);
      sha256( trx_bytes.data(), trx_size, &trx_id );
      return trx_id;
   }

   string checksum256_to_string( capi_checksum256 src ){
      // TODO
      return string();
   }

   uint64_t pegtoken::hash64( string s ){
      capi_checksum256  hash256;
      uint64_t res = 0;
      sha256( s.c_str(), s.length(), &hash256 );
      std::memcpy(&res, hash256.hash, 8);
      return res;
   }

   void pegtoken::verify_address( name style, string addr){
      if ( style == "bitcoin"_n ){

      } else if ( style == "ethereum"_n ){

      } else if ( style == "eosio"_n ){

      } else if ( style == "other"_n ){
      } else {
         eosio_assert(false, "address style must be one of bitcoin, ethereum, eosio or other" );
      }
   }

   void pegtoken::verify_maximum_supply(asset maximum_supply){
      auto sym = maximum_supply.symbol;
      eosio_assert( sym.is_valid(), "invalid symbol name" );
      eosio_assert( maximum_supply.is_valid(), "invalid supply");
      eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

      stats statstable( _self, sym.code().raw() );
      auto existing = statstable.find( sym.code().raw() );
      if( existing != statstable.end() ){
         const auto& st = *existing;
         eosio_assert( maximum_supply.symbol == st.max_supply.symbol, "maximum_supply symbol mismatch with existing token symbol" );
         eosio_assert( maximum_supply.amount >= st.supply.amount, "maximum_supply amount should not less then total supplied amount" );

      }
   }

   void pegtoken::create( name    issuer,
                          name    auditor,
                          asset   maximum_supply,
                          asset   large_asset,
                          name    address_style,
                          string  organization,
                          string  website,
                          string  miner_fee,
                          string  service_fee,
                          string  unified_recharge_address,
                          bool    active )
   {
      require_auth( _self );

      eosio_assert( is_account( issuer ), "issuer account does not exist");
      eosio_assert( is_account( auditor ), "auditor account does not exist");

      auto sym = maximum_supply.symbol;
      eosio_assert( sym.is_valid(), "invalid symbol name" );
      eosio_assert( maximum_supply.is_valid(), "invalid supply");
      eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

      eosio_assert( large_asset.symbol == maximum_supply.symbol, "large_asset's symbol mismatch with maximum_supply's symbol" );
      eosio_assert( large_asset.is_valid(), "invalid large_asset");
      eosio_assert( large_asset.amount > 0, "large_asset must be positive");
      eosio_assert( large_asset.amount < maximum_supply.amount, "large_asset should less then maximum_supply");

      eosio_assert( address_style == "bitcoin"_n || address_style == "ethereum"_n ||
                    address_style == "eosio"_n || address_style == "other"_n,
                    "address_style must be one of bitcoin, ethereum, eosio or other" );

      eosio_assert( organization.size() <= 256, "organization has more than 256 bytes" );
      eosio_assert( website.size() <= 256, "website has more than 256 bytes" );
      eosio_assert( miner_fee.size() <= 256, "miner_fee has more than 256 bytes" );
      eosio_assert( service_fee.size() <= 256, "service_fee has more than 256 bytes" );
      eosio_assert( unified_recharge_address.size() <= 256, "unified_recharge_address has more than 256 bytes" );

      if ( unified_recharge_address != "" ){
         verify_address( address_style, unified_recharge_address );
      }

      stats statstable( _self, sym.code().raw() );
      auto existing = statstable.find( sym.code().raw() );
      eosio_assert( existing == statstable.end(), "token with symbol already exists" );

      statstable.emplace( _self, [&]( auto& s ) {
         s.supply.symbol = maximum_supply.symbol;
         s.max_supply    = maximum_supply;
         s.large_asset   = large_asset;
         s.issuer        = issuer;
         s.auditor       = auditor;
         s.address_style = address_style;
         s.organization  = organization;
         s.website       = website;
         s.miner_fee     = miner_fee;
         s.service_fee   = service_fee;
         s.unified_recharge_address   = unified_recharge_address;
         s.active        = active;
         s.issue_seq_num = 0;
      });
   }

   void pegtoken::setmaxsupply( asset maximum_supply )
   {
      require_auth( _self );

      auto sym = maximum_supply.symbol;
      eosio_assert( sym.is_valid(), "invalid symbol name" );
      eosio_assert( maximum_supply.is_valid(), "invalid supply");
      eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

      stats statstable( _self, sym.code().raw() );
      auto existing = statstable.find( sym.code().raw() );
      eosio_assert( existing != statstable.end(), "token with symbol dose not exists" );
      const auto& st = *existing;

      eosio_assert( maximum_supply.symbol == st.supply.symbol, "maximum_supply symbol mismatch with existing token symbol" );
      eosio_assert( maximum_supply.amount >= st.supply.amount, "maximum_supply amount should not less then total supplied amount" );
      eosio_assert( maximum_supply.amount != st.max_supply.amount, "the amount equal to current max_supply amount" );

      statstable.modify( st, same_payer, [&]( auto& s ) { s.max_supply = maximum_supply; });
   }

   void pegtoken::setlargeast( asset large_asset )
   {
      auto sym = large_asset.symbol;
      eosio_assert( sym.is_valid(), "invalid symbol name" );
      eosio_assert( large_asset.is_valid(), "invalid supply");
      eosio_assert( large_asset.amount > 0, "max-supply must be positive");

      stats statstable( _self, sym.code().raw() );
      auto existing = statstable.find( sym.code().raw() );
      eosio_assert( existing != statstable.end(), "token with symbol dose not exists" );
      const auto& st = *existing;

      require_auth( st.auditor );

      statstable.modify( st, same_payer, [&]( auto& s ) { s.large_asset = large_asset; });
   }

   void pegtoken::lockall( symbol_code sym_code ){
      stats statstable( _self, sym_code.raw() );
      auto existing = statstable.find( sym_code.raw() );
      eosio_assert( existing != statstable.end(), "token with symbol dose not exists" );
      const auto& st = *existing;

      eosio_assert( has_auth( st.issuer ) || has_auth( st.auditor ) , "this action can excute only by issuer or auditor" );
      eosio_assert( st.active == true , "this token has been locked already" );

      statstable.modify( st, same_payer, [&]( auto& s ) { s.active = false; });
   }

   void pegtoken::unlockall( symbol_code sym_code ){
      stats statstable( _self, sym_code.raw() );
      auto existing = statstable.find( sym_code.raw() );
      eosio_assert( existing != statstable.end(), "token with symbol dose not exists" );
      const auto& st = *existing;

      require_auth( st.auditor );
      eosio_assert( st.active == false , "this token unlocked, nothing to do" );

      statstable.modify( st, same_payer, [&]( auto& s ) { s.active = true; });
   }

   void pegtoken::update( symbol_code sym_code, string parameter, string value ){
      stats statstable( _self, sym_code.raw() );
      auto existing = statstable.find( sym_code.raw() );
      eosio_assert( existing != statstable.end(), "token with symbol dose not exists" );
      const auto& st = *existing;

      eosio_assert( parameter.size() > 0, "parameter can not empty" );
      eosio_assert( parameter.size() <= 64, "parameter has more than 64 bytes" );
      eosio_assert( value.size() >0, "value can not empty" );
      eosio_assert( value.size() <= 256, "value has more than 256 bytes" );

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
         verify_address( st.address_style, value );
         statstable.modify( st, same_payer, [&]( auto& s ) { s.unified_recharge_address = value; });
      }  else {
         eosio_assert(false, "this action does not support this parameter" );
      }
   }

   void pegtoken::applicant( symbol_code sym_code, name action, name applicant ){
      stats statstable( _self, sym_code.raw() );
      auto existing = statstable.find( sym_code.raw() );
      eosio_assert( existing != statstable.end(), "token with symbol does not exist" );
      const auto& st = *existing;

      eosio_assert( has_auth( st.issuer ) || has_auth( st.auditor ) , "this action can excute only by issuer or auditor" );

      applicants table( _self, sym_code.raw() );
      auto it = table.find( applicant.value );
      
      if ( action == "add"_n ){
         eosio_assert( it == table.end(), "applicant already exist, nothing to do" );
         table.emplace( _self, [&]( auto& r ) {
            r.applicant = applicant;
         });
      } else if ( action == "remove"_n ){
         eosio_assert( it != table.end(), "applicant dose not exist, nothing to do" );
         table.erase( it );
      } else {
         eosio_assert(false, "action must be add or remove" );
      }
   }

   void pegtoken::applyaddr( name applicant, name to, symbol_code sym_code ){
      applicants table( _self, sym_code.raw() );
      auto it = table.find( applicant.value );
      eosio_assert( it != table.end(), "applicant dose not exist" );

      require_auth( applicant );

      addresses addrtable( _self, sym_code.raw() );
      auto existing2 = addrtable.find( to.value );
      eosio_assert( existing2 == addrtable.end(), "to account has applied for address already" );

      addrtable.emplace( _self, [&]( auto& a ) {
         a.owner        = to;
         a.state        = 1;
      });
   }

   void pegtoken::assignaddr( symbol_code sym_code, name to, string address ){
      eosio_assert( address.size() <= 64, "address too long" );
      eosio_assert( is_account( to ), "to account does not exist");

      stats statstable( _self, sym_code.raw() );
      auto existing = statstable.find( sym_code.raw() );
      eosio_assert( existing != statstable.end(), "token with symbol does not exist" );
      const auto& st = *existing;

      verify_address( st.address_style, address);
      
      require_auth( st.issuer );
      
      addresses addrtable( _self, sym_code.raw() );
      auto idx = addrtable.get_index<"address"_n>();
      auto existing2 = idx.find( hash64(address) );
      eosio_assert( existing2 == idx.end(), ("this address " + address + " has been assigned to " + existing2->owner.to_string()).c_str() );

      auto it = addrtable.find( to.value );
      if( it == addrtable.end() ){
         addrtable.emplace( _self, [&]( auto& a ) {
            a.owner = to;
            a.address = address;
            a.assign_time = current_time_point();
         });
      } else {
         addrtable.modify( it, same_payer, [&]( auto& a ) {
            a.owner        = to;
            a.address      = address;
            a.assign_time  = current_time_point();
            a.state        = 2;
         });
      }
   }

   void pegtoken::issue( uint64_t seq_num, name to, asset quantity, string memo )
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

       eosio_assert( seq_num == st.issue_seq_num + 1, "error issue sequence number" );
      statstable.modify( st, same_payer, [&]( auto& s ) { s.issue_seq_num += 1; });

       if( quantity.amount >= st.large_asset.amount ){
          issues issue_table( _self, sym.code().raw() );
          issue_table.emplace( _self, [&]( auto& r ) {
             r.seq_num  = seq_num;
             r.to       = to;
             r.quantity = quantity;
             r.memo     = memo;
          });
       } else {
          statstable.modify( st, same_payer, [&]( auto& s ) { s.supply += quantity; });
          add_balance( st.issuer, quantity, st.issuer );
          if( to != st.issuer ) {
             SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} },
                                 { st.issuer, to, quantity, memo }
             );
          }
       }
   }

   void pegtoken::issue_handle( symbol_code sym_code, uint64_t issue_seq_num, bool pass){
      stats statstable( _self, sym_code.raw() );
      auto existing = statstable.find( sym_code.raw() );
      eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
      const auto& st = *existing;

      require_auth( st.auditor );

      issues issue_table( _self, sym_code.raw() );
      auto it = issue_table.find( issue_seq_num );
      eosio_assert( it != issue_table.end(), "issue with seq number does not exist" );
      const auto& issue = *it;

      if ( pass ){
         statstable.modify( st, same_payer, [&]( auto& s ) { s.supply += issue.quantity; });
         if( issue.to != st.issuer ) {
            add_balance( st.auditor, issue.quantity, st.auditor );
            SEND_INLINE_ACTION( *this, transfer, { {st.auditor, "active"_n} },
                                { st.auditor, issue.to, issue.quantity, issue.memo }
            );
         } else {
            add_balance( st.issuer, issue.quantity, st.auditor );
         }
         issue_table.erase( it );
      } else {
         issue_table.erase( it );
      }
   }

   void pegtoken::approve( symbol_code sym_code, uint64_t issue_seq_num ){
      issue_handle( sym_code, issue_seq_num, true );
   }

   void pegtoken::unapprove( symbol_code sym_code, uint64_t issue_seq_num ){
      issue_handle( sym_code, issue_seq_num, false );
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

   void pegtoken::withdraw( name from, string to_address, asset quantity, string memo ){
      require_auth( from );

      auto sym = quantity.symbol;
      eosio_assert( sym.is_valid(), "invalid symbol name" );
      eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

      stats statstable( _self, sym.code().raw() );
      auto existing = statstable.find( sym.code().raw() );
      eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
      const auto& st = *existing;

      verify_address( st.address_style, to_address);

      eosio_assert( quantity.is_valid(), "invalid quantity" );
      eosio_assert( quantity.amount > 0, "must issue positive quantity" );
      eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

      accounts acnts( _self, from.value );
      const auto& owner = acnts.get( quantity.symbol.code().raw(), "no balance object found" );
      eosio_assert( owner.balance.amount >= quantity.amount, "overdrawn balance" );

      SEND_INLINE_ACTION( *this, transfer, { { from, "active"_n} }, { from, st.issuer, quantity, "withdraw address:" + to_address + " memo: " + memo } );

      withdraws withdraw_table( _self, quantity.symbol.code().raw() );

      auto trx_id = get_trx_id();
      withdraw_table.emplace( _self, [&]( auto& w ){
         w.id     = withdraw_table.available_primary_key();
         w.trx_id = trx_id;
         w.from   = from;
         w.to     = to_address;
         w.quantity = quantity;
         w.feedback_state  = 0;
      });
   }

   void pegtoken::feedback( symbol_code sym_code, uint64_t id, uint64_t state, string trx_id, string memo ){
      eosio_assert( state == 1 || state == 2, "state can only be 1 or 2" );
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
         w.feedback_state  = state;
         w.feedback_trx_id = trx_id;
         w.feedback_msg    = memo;
         w.feedback_time   = current_time_point();
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

      auto trx_id = get_trx_id();
      withdraw_table.modify( wt, same_payer, [&]( auto& w ) {
         w.feedback_state  = 5;
         w.feedback_trx_id = checksum256_to_string( trx_id );
         w.feedback_msg    = memo;
         w.feedback_time   = current_time_point();
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

EOSIO_DISPATCH( eosio::pegtoken, (create)(setmaxsupply)(setlargeast)(lockall)(unlockall)(update)(applicant)(applyaddr)(assignaddr)(issue)(approve)(unapprove)(transfer)(withdraw)(feedback)(rollback)(open)(close)(retire) )
