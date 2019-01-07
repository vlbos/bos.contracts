/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosiolib/action.hpp>
#include <eosiolib/transaction.hpp>
#include <ibc.chain/ibc.chain.hpp>
#include <ibc.token/ibc.token.hpp>

#include <merkle.cpp>
#include "utils.cpp"

namespace eosio {

   token::token( name s, name code, datastream<const char*> ds ):
         contract( s, code, ds ),
         _global_state( _self, _self.value ),
         _global_mutable( _self, _self.value ),
         _accepts( _self, _self.value ),
         _stats( _self, _self.value ),
         _origtrxs( _self, _self.value ),
         _cashtrxs( _self, _self.value ),
         _acntbls( _self, _self.value ),
         _trxbls( _self, _self.value )
   {
      _gstate = _global_state.exists() ? _global_state.get() : global_state{};
      _gmutable = _global_mutable.exists() ? _global_mutable.get() : global_mutable{};
   }

   token::~token(){
      _global_state.set( _gstate, _self );
      _global_mutable.set( _gmutable, _self );
   }
   
   void token::setglobal( name       ibc_contract,
                          name       peerchain_ibc_token_contract,
                          uint32_t   max_origtrxs_table_records,
                          uint32_t   cache_cashtrxs_table_records,
                          uint32_t   max_original_trxs_per_block,
                          bool       active ) {
      require_auth( _self );

      _gstate.ibc_contract = ibc_contract;
      _gstate.peerchain_ibc_token_contract = peerchain_ibc_token_contract;
      _gstate.max_origtrxs_table_records = max_origtrxs_table_records;
      _gstate.cache_cashtrxs_table_records = cache_cashtrxs_table_records;
      _gstate.max_original_trxs_per_block = max_original_trxs_per_block;
      _gstate.active = active;
   }

   void token::regacpttoken( name        original_contract,
                             asset       max_accept,
                             name        administrator,
                             asset       min_once_transfer,
                             asset       max_once_transfer,
                             asset       max_daily_transfer,
                             uint32_t    max_tfs_per_minute,
                             string      organization,
                             string      website,
                             name        service_fee_mode,
                             asset       service_fee_fixed,
                             double      service_fee_ratio,
                             bool        active,
                             symbol      peerchain_sym ){
      require_auth( _self );

      eosio_assert( is_account( original_contract ), "original_contract account does not exist");
      eosio_assert( is_account( administrator ), "administrator account does not exist");

      eosio_assert( max_accept.is_valid(), "invalid max_accept");
      eosio_assert( max_accept.amount > 0, "max_accept must be positive");
      eosio_assert( min_once_transfer.symbol == max_accept.symbol &&
                    max_once_transfer.symbol == max_accept.symbol &&
                    max_daily_transfer.symbol == max_accept.symbol &&
                    service_fee_fixed.symbol == max_accept.symbol &&
                    min_once_transfer.amount > 0 &&
                    max_once_transfer.amount > min_once_transfer.amount &&
                    max_daily_transfer.amount > max_once_transfer.amount &&
                    service_fee_fixed.amount >= 0, "invalid asset");

      eosio_assert( organization.size() < 256, "organization has more than 256 bytes");
      eosio_assert( website.size() < 256, "website has more than 256 bytes");
      eosio_assert( service_fee_mode == "fixed"_n || service_fee_mode == "ratio"_n, "invalid service_fee_mode");
      eosio_assert( 0 < service_fee_ratio && service_fee_ratio <= 0.05 , "service_fee_ratio invalid");

      eosio_assert( peerchain_sym.is_valid(), "peerchain_sym invalid");
      eosio_assert( peerchain_sym.precision() == max_accept.symbol.precision(), "precision mismatch");


      auto existing = _accepts.find( original_contract.value );
      eosio_assert( existing == _accepts.end(), "token contract already exist" );

      _accepts.emplace( _self, [&]( auto& r ){
         r.original_contract = original_contract;
         r.accept = asset{ 0, max_accept.symbol };
         r.max_accept = max_accept;
         r.min_once_transfer = min_once_transfer;
         r.max_once_transfer = max_once_transfer;
         r.max_daily_transfer = max_daily_transfer;
         r.max_tfs_per_minute = max_tfs_per_minute;
         r.peg_token_symbol = peerchain_sym;
         r.administrator = administrator;
         r.organization = organization;
         r.website = website;
         r.service_fee_mode = service_fee_mode;
         r.service_fee_fixed = service_fee_fixed;
         r.service_fee_ratio = service_fee_ratio;
         r.total_transfer =  asset{ 0, max_accept.symbol };
         r.total_transfer_times = 0;
         r.total_cash =  asset{ 0, max_accept.symbol };
         r.total_cash_times = 0;
         r.active = active;
      });
   }

   void token::setacptasset( name contract, string which, asset quantity ) {
      const auto& acpt = get_currency_accept( contract );
      eosio_assert( quantity.symbol == acpt.accept.symbol, "invalid symbol" );

      require_auth( acpt.administrator );

      if ( which == "max_accept" ){
         eosio_assert( quantity.amount >= acpt.accept.amount, "max_accept.amount should not less then accept.amount");
         _accepts.modify( acpt, same_payer, [&]( auto& r ) { r.max_accept = quantity; });
         return;
      }
      if ( which == "min_once_transfer" ){
         eosio_assert( quantity.amount > 0, "min_once_transfer must be positive");
         _accepts.modify( acpt, same_payer, [&]( auto& r ) { r.min_once_transfer = quantity; });
         return;
      }
      if ( which == "max_once_transfer" ){
         eosio_assert( quantity.amount > acpt.min_once_transfer.amount, "max_once_transfer must be greater then min_once_transfer");
         _accepts.modify( acpt, same_payer, [&]( auto& r ) { r.max_once_transfer = quantity; });
         return;
      }
      if ( which == "max_daily_transfer" ){
         eosio_assert( quantity.amount > acpt.max_once_transfer.amount, "max_daily_transfer must be greater then max_once_transfer");
         _accepts.modify( acpt, same_payer, [&]( auto& r ) { r.max_daily_transfer = quantity; });
         return;
      }
      eosio_assert( false, "unkown config item" );
   }

   void token::setacptstr( name contract, string which, string value ) {
      const auto& acpt = get_currency_accept( contract );
      eosio_assert( value.size() < 256, "value string has more than 256 bytes");

      require_auth( acpt.administrator );

      if ( which == "organization" ){
         _accepts.modify( acpt, acpt.administrator, [&]( auto& r ) { r.organization = value; });
         return;
      }
      if ( which == "website" ){
         _accepts.modify( acpt, acpt.administrator, [&]( auto& r ) { r.website = value; });
         return;
      }
      eosio_assert( false, "unkown config item" );
   }

   void token::setacptint( name contract, string which, uint64_t value ) {
      const auto& acpt = get_currency_accept( contract );
      require_auth( _self );

      if ( which == "max_tfs_per_minute" ){
         _accepts.modify( acpt, _self, [&]( auto& r ) { r.max_tfs_per_minute = value; });
         return;
      }
      eosio_assert( false, "unkown config item" );
   }

   void token::setacptbool( name contract, string which, bool value ) {
      const auto& acpt = get_currency_accept( contract );
      require_auth( acpt.administrator );

      if ( which == "active" ){
         _accepts.modify( acpt, acpt.administrator, [&]( auto& r ) { r.active = value; });
         return;
      }
      eosio_assert( false, "unkown config item" );
   }

   void token::setacptfee( name   contract,
                           name   service_fee_mode,
                           asset  service_fee_fixed,
                           double service_fee_ratio ) {
      const auto& acpt = get_currency_accept( contract );
      eosio_assert( service_fee_mode == "fixed"_n || service_fee_mode == "ratio"_n, "mode can only be fixed or ratio");
      eosio_assert( service_fee_fixed.symbol == acpt.accept.symbol && service_fee_fixed.amount >= 0, "service_fee_fixed invalid" );
      eosio_assert( 0 < service_fee_ratio && service_fee_ratio <= 0.05 , "service_fee_ratio invalid");

      require_auth( _self );

      _accepts.modify( acpt, _self, [&]( auto& r ) {
         r.service_fee_mode = service_fee_mode;
         r.service_fee_fixed = service_fee_fixed;
         r.service_fee_ratio = service_fee_ratio;
      });
   }
   
   void token::regpegtoken( asset       max_supply,
                            asset       min_once_withdraw,
                            asset       max_once_withdraw,
                            asset       max_daily_withdraw,
                            uint32_t    max_wds_per_minute,
                            name        administrator,
                            name        peerchain_contract,
                            symbol      peerchain_sym,
                            bool        active ){
      require_auth( _self );
      eosio_assert( is_account( administrator ), "administrator account does not exist");

      eosio_assert( max_supply.is_valid(), "invalid max_accept");
      eosio_assert( max_supply.amount > 0, "max_accept must be positive");
      eosio_assert( min_once_withdraw.symbol == max_supply.symbol &&
                    max_once_withdraw.symbol == max_supply.symbol &&
                    max_daily_withdraw.symbol == max_supply.symbol &&
                    min_once_withdraw.amount > 0 &&
                    max_daily_withdraw.amount > min_once_withdraw.amount , "invalid asset");

      eosio_assert( peerchain_sym.is_valid(), "peerchain_sym invalid");
      eosio_assert( peerchain_sym.precision() == max_supply.symbol.precision(), "precision mismatch");

      auto existing = _stats.find( max_supply.symbol.code().raw() );
      eosio_assert( existing == _stats.end(), "token contract already exist" );

      _stats.emplace( _self, [&]( auto& r ){
         r.supply = asset{ 0, max_supply.symbol };
         r.max_supply = max_supply;
         r.min_once_withdraw = min_once_withdraw;
         r.max_once_withdraw = max_once_withdraw;
         r.max_daily_withdraw = max_daily_withdraw;
         r.max_wds_per_minute = max_wds_per_minute;
         r.peerchain_contract = peerchain_contract;
         r.orig_token_sym = peerchain_sym;
         r.administrator = administrator;
         r.total_issue = asset{ 0, max_supply.symbol };
         r.total_issue_times = 0;
         r.total_withdraw = asset{ 0, max_supply.symbol };
         r.total_withdraw_times = 0;
         r.active = true;
      });
   }

   void token::setpegasset( symbol_code symcode, string which, asset quantity ) {
      const auto& st = get_currency_stats( symcode );
      eosio_assert( quantity.symbol == st.supply.symbol, "invalid symbol" );
      require_auth( st.administrator );

      if ( which == "max_supply" ){
         eosio_assert( quantity.amount >= st.supply.amount, "max_supply.amount should not less then supply.amount");
         _stats.modify( st, same_payer, [&]( auto& r ) { r.max_supply = quantity; });
         return;
      }
      if ( which == "min_once_withdraw" ){
         eosio_assert( quantity.amount >= 0, "min_once_withdraw.amount can not be negtive");
         _stats.modify( st, same_payer, [&]( auto& r ) { r.min_once_withdraw = quantity; });
         return;
      }
      if ( which == "max_once_withdraw" ){
         eosio_assert( quantity.amount >= st.min_once_withdraw.amount, "max_once_withdraw.amount should not less then min_once_withdraw.amount");
         _stats.modify( st, same_payer, [&]( auto& r ) { r.max_supply = quantity; });
         return;
      }
      if ( which == "max_daily_withdraw" ){
         eosio_assert( quantity.amount >= st.max_once_withdraw.amount, "max_daily_withdraw.amount should not less then max_once_withdraw.amount");
         _stats.modify( st, same_payer, [&]( auto& r ) { r.max_daily_withdraw = quantity; });
         return;
      }
      eosio_assert( false, "unkown config item" );
   }

   void token::setpegint( symbol_code symcode, string which, uint64_t value ) {
      const auto& st = get_currency_stats( symcode );
      require_auth( _self );

      if ( which == "max_wds_per_minute" ){
         _stats.modify( st, _self, [&]( auto& r ) { r.max_wds_per_minute = value; });
         return;
      }
      eosio_assert( false, "unkown config item" );
   }

   void token::setpegbool( symbol_code symcode, string which, bool value ) {
      const auto& st = get_currency_stats( symcode );
      require_auth( st.administrator );

      if ( which == "active" ){
         _stats.modify( st, same_payer, [&]( auto& r ) { r.active = value; });
         return;
      }
      eosio_assert( false, "unkown config item" );
   }


   /**
    * ---- ibc memo format of transfer action ----
    * memo string must start with "ibc", and the rest of the string should be composed of key-value pairs
    * key-value pairs are separated by spaces, key and value are separated by equal-sign
    * currently supported keys include: "receiver,r","notes". other keys will be ignored
    *
    * examples:
    * "ibc receiver=youraccount"
    * "ibc r=youraccount"
    * "ibc r=youraccount notes=this is notes txt"
    *
    * key details:
    * "receiver,r": required,   must be a valid eosio name. The name should exist on peerchain, if not, this ibc transaction will failed finally.
    * "notes":      optional,   user defined string, must be the last item if have, all content after "notes=" is the value of "notes"
    */

   struct memo_info_type {
      name     receiver;
      string   notes;
   };
   
   memo_info_type get_memo_info( const string& memo ){
      auto get_item = [=]( string item ) {
         string search = string(" ") + item + "=";
         auto pos = memo.find( search );
         decltype(pos) pos_end;
         if ( pos !=  std::string::npos ){
            if ( item == "notes" ){
               pos_end = memo.size();
            } else {
               pos_end = memo.find(" ", pos + search.length());
               if( pos_end == std::string::npos ) {
                  pos_end = memo.size();
               }
            }

            auto start = pos + search.length();
            auto count = pos_end - start;
            auto res = memo.substr( start, count );
            eosio_assert( res.length() != 0, "memo contain incomplete item" );
            return res;
         } else {
            return string();
         }
      };

      string receiver_str = get_item("receiver");
      string r_str = get_item("r");
      eosio_assert(  receiver_str == string() || r_str == string(), "\"receiver\" and \"r\" can not exist at the same time" );

      memo_info_type info;
      if ( receiver_str != string() ){
         info.receiver = name( receiver_str );
      } else {
         eosio_assert( r_str != string(),"must provide \"receiver\" or \"r\" key-value");
         info.receiver = name( r_str );
      }

      info.notes = get_item("notes");
      return info;
   }

   /**
     * memo string format specification:
     * memo string must start with "local" or "ibc", any other prefix or empty memo string will assert failed
     * when start with "local", means this is a local chain transaction, do not any process and return directly
     * when start with "ibc", means this is a inter-blockchain communicatiion transaction
     * then memo string should meet the "ibc memo format of transfer action"  described above
     */
   void token::transfer_notify( name original_contract, name from, name to, asset quantity, string memo ) {
      eosio_assert( to == _self, "to is not this contract");

      if ( memo.find("local") == 0 ){
         return;
      }
      eosio_assert( memo.find("ibc") == 0, "memo should start with \"ibc\" or \"local\"");

      auto info = get_memo_info( memo );
      eosio_assert(info.receiver != name(),"receiver not provide");

      // check global state
      eosio_assert( is_global_active(), "global not active" );
      // check account blacklist
      eosio_assert( !is_in_acntbls( from ), "from is in blacklist" );

      const auto& acpt = get_currency_accept( original_contract );
      eosio_assert( acpt.active, "not active");
      eosio_assert( quantity.symbol == acpt.accept.symbol, "symbol does not match");
      eosio_assert( quantity.amount >= acpt.min_once_transfer.amount, "quantity less then min_once_transfer");
      eosio_assert( quantity.amount <= acpt.max_once_transfer.amount, "quantity greater then max_once_transfer");

      // accumulate max_tfs_per_minute and check
      auto current_time_sec = now();
      uint32_t limit = acpt.max_tfs_per_minute > 0 ? acpt.max_tfs_per_minute : default_max_trx_per_minute_per_token;
      if ( current_time_sec > acpt.mutables.minute_trx_start + 60 ){
         _accepts.modify( acpt, same_payer, [&]( auto& r ) {
            r.mutables.minute_trx_start = current_time_sec;
            r.mutables.minute_trxs = 1;
         });
      } else {
         _accepts.modify( acpt, same_payer, [&]( auto& r ) {
            r.mutables.minute_trxs += 1;
         });
      }
      eosio_assert( acpt.mutables.minute_trxs <= limit,"max transactions per minute exceed" );

      // accumulate max_daily_transfer and check
      if ( acpt.max_daily_transfer.amount != 0 ) {
         if ( current_time_sec > acpt.mutables.daily_tf_start + 3600 * 24 ){
            _accepts.modify( acpt, same_payer, [&]( auto& r ) {
               r.mutables.daily_tf_start = current_time_sec;
               r.mutables.daily_tf_sum = quantity;
            });
         } else {
            _accepts.modify( acpt, same_payer, [&]( auto& r ) {
               r.mutables.daily_tf_sum += quantity;
            });
         }
         eosio_assert( acpt.mutables.daily_tf_sum <= acpt.max_daily_transfer,"max daily transfer exceed" );
      }

      // accumulate max_original_trxs_per_block and check
      if ( get_block_time_slot() == _gmutable.current_block_time_slot ) {
         _gmutable.current_block_trxs += 1;
         eosio_assert( _gmutable.current_block_trxs <= _gstate.max_original_trxs_per_block, "max_original_trxs_per_block exceed" );
      } else {
         _gmutable.current_block_time_slot = get_block_time_slot();
         _gmutable.current_block_trxs = 1;
      }

      _accepts.modify( acpt, same_payer, [&]( auto& r ) {
         r.accept += quantity;
         r.total_transfer += quantity;
         r.total_transfer_times += 1;
      });

      origtrxs_emplace( transfer_action_info{ original_contract, from, quantity } );
   }

   /**
    * memo string format specification:
    * when to == _self, memo string must start with "local" or "ibc", any other prefix or empty memo string will assert failed
    * when start with "local", means this is a local chain transaction
    * when start with "ibc", means this is a inter-blockchain communicatiion transaction
    * then memo string should meet the "ibc memo format of transfer action" described above
    *
    * when to != _self, memo string will not be parsed
    */
   void token::transfer( name    from,
                         name    to,
                         asset   quantity,
                         string  memo )
   {
      eosio_assert( from != to, "cannot transfer to self" );
      require_auth( from );

      if (  to == _self ) {
         if ( memo.find("ibc") == 0 ){
            auto info = get_memo_info( memo );
            eosio_assert( info.receiver != name(), "receiver not provide");
            withdraw( from, info.receiver, quantity, info.notes );
            return;
         }
         eosio_assert( memo.find("local") == 0 , "when transfer to this contract, memo must start with \"ibc\" or \"local\"");
      }

      eosio_assert( is_account( to ), "to account does not exist");
      auto sym = quantity.symbol.code();
      const auto& st = _stats.get( sym.raw() );

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

   void token::withdraw( name from, name peerchain_receiver, asset quantity, string memo ) {
      require_auth( from );

      // check global state
      eosio_assert( is_global_active(), "global not active" );
      // check account blacklist
      eosio_assert( !is_in_acntbls( from ), "from is in blacklist" );

      const auto& st = get_currency_stats( quantity.symbol.code() );
      eosio_assert( st.active, "not active");
      eosio_assert( quantity.symbol == st.supply.symbol, "symbol does not match");
      eosio_assert( quantity.amount >= st.min_once_withdraw.amount, "quantity less then min_once_transfer");
      eosio_assert( quantity.amount <= st.max_once_withdraw.amount, "quantity greater then max_once_transfer");

      const auto& balance = get_balance( _self, from, quantity.symbol.code() );
      eosio_assert( quantity.amount <= balance.amount, "overdrawn balance");

      // accumulate and check
      auto current_time_sec = now();
      auto limit = st.max_wds_per_minute > 0 ? st.max_wds_per_minute : default_max_trx_per_minute_per_token;

      if ( current_time_sec > st.mutables.minute_trx_start + 60 ){
         _stats.modify( st, same_payer, [&]( auto& r ) {
            r.mutables.minute_trx_start = current_time_sec;
            r.mutables.minute_trxs = 1;
         });
      } else {
         _stats.modify( st, same_payer, [&]( auto& r ) {
            r.mutables.minute_trxs += 1;
         });
      }
      eosio_assert( st.mutables.minute_trxs <= limit, "max transactions per minute exceed" );


      if ( st.max_daily_withdraw.amount != 0 ) {
         if ( current_time_sec > st.mutables.daily_wd_start + 3600 * 24 ){
            _stats.modify( st, same_payer, [&]( auto& r ) {
               r.mutables.daily_wd_start = current_time_sec;
               r.mutables.daily_wd_sum = quantity;
            });
         } else {
            _stats.modify( st, same_payer, [&]( auto& r ) {
               r.mutables.daily_wd_sum += quantity;
            });
         }
         eosio_assert( st.mutables.daily_wd_sum <= st.max_daily_withdraw,"max daily per minute exceed" );
      }

      if ( get_block_time_slot() == _gmutable.current_block_time_slot ) {
         _gmutable.current_block_trxs += 1;
         eosio_assert( _gmutable.current_block_trxs <= _gstate.max_original_trxs_per_block, "max_original_trxs_per_block exceed" );
      } else {
         _gmutable.current_block_time_slot = get_block_time_slot();
         _gmutable.current_block_trxs = 1;
      }

      _stats.modify( st, same_payer, [&]( auto& r ) {
         r.supply -= quantity;
         r.total_withdraw += quantity;
         r.total_withdraw_times += 1;
      });

      sub_balance( from, quantity );
      add_balance( _self, quantity, _self );
      origtrxs_emplace( transfer_action_info{ _self, from, quantity } );
   }

   void token::verify_merkle_path( const std::vector<digest_type>& merkle_path, digest_type check ) {
      eosio_assert( merkle_path.size() > 0,"merkle_path can not be empty");

      if ( merkle_path.size() == 1 ){
         eosio_assert( is_equal_capi_checksum256(check, merkle_path[0]),"merkle path validate failed");
         return;
      }

      eosio_assert( is_equal_capi_checksum256(check, merkle_path[0]) ||
                    is_equal_capi_checksum256(check, merkle_path[1]), "digest not in merkle tree");

      digest_type result = get_checksum256( make_canonical_pair(merkle_path[0], merkle_path[1]) );

      for( auto i = 0; i < merkle_path.size() - 3; ++i ){
         digest_type left;
         digest_type right;

         if ( is_canonical_left(merkle_path[i+2]) ){
            left = merkle_path[i+2];
            right = make_canonical_right( result );
         } else {
            left = make_canonical_left( result );
            right = merkle_path[i+2];
         }
         result = get_checksum256( std::make_pair(left,right) );
      }
      eosio_assert( is_equal_capi_checksum256(result, merkle_path.back()) ,"merkle path validate failed" );
   }


   void token::cash( uint64_t                               seq_num,
                     const uint32_t                         orig_trx_block_num,
                     const std::vector<char>&               orig_trx_packed_trx_receipt,
                     const std::vector<capi_checksum256>&   orig_trx_merkle_path,
                     transaction_id_type                    orig_trx_id,   // redundant, facilitate indexing and checking
                     name                                   to,            // redundant, facilitate indexing and checking
                     asset                                  quantity,      // with the token symbol of the original trx it self. redundant, facilitate indexing and checking
                     string                                 memo,
                     name                                   relay ) {

      eosio_assert( chain::is_relay( _gstate.ibc_contract, relay ), "relay not exist");
      require_auth( relay );

      // check transaction blacklist
      eosio_assert( false == is_in_trxbls( orig_trx_id ), "transaction is in blacklist");

      auto sym = quantity.symbol;
      eosio_assert( sym.is_valid(), "invalid symbol name" );
      eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

      eosio_assert( seq_num == get_cashtrxs_tb_max_seq_num() + 1, "seq_num not valid");
      eosio_assert( orig_trx_block_num >= get_cashtrxs_tb_max_orig_trx_block_num(), "orig_trx_block_num error");
      eosio_assert( false == is_orig_trx_id_exist_in_cashtrxs_tb(orig_trx_id), "orig_trx_id already exist");

      const transaction_receipt& trx_receipt = unpack<transaction_receipt>( orig_trx_packed_trx_receipt );
      packed_transaction pkd_trx = std::get<packed_transaction>(trx_receipt.trx);
      transaction trxn = unpack<transaction>( pkd_trx.packed_trx );
      eosio_assert( trxn.actions.size() == 1, "transfer transaction contains more then one action" );

      // validate transaction id
      eosio_assert( std::memcmp(orig_trx_id.hash, pkd_trx.id().hash, 32) == 0, "transaction id mismatch");

      // check transfer action
      action actn = trxn.actions.front();
      transfer_action_type args = unpack<transfer_action_type>( actn.data );

      eosio_assert( args.to == _gstate.peerchain_ibc_token_contract, "transfer to account not correct" );
      eosio_assert( args.quantity == quantity, "quantity not equal to quantity within packed transaction" );
      const memo_info_type& memo_info = get_memo_info( args.memo ); // to-do assert start with ibc

      eosio_assert( to == memo_info.receiver, "to not equal to receiver，which provided in memo string" );

      // validate merkle path
      verify_merkle_path( orig_trx_merkle_path, trx_receipt.digest());

      // validate transaction_mroot with lwc
      chain::assert_block_in_lib_and_trx_mroot_in_block( _gstate.ibc_contract, orig_trx_block_num, orig_trx_merkle_path.back() );

      asset new_quantity;

      if ( actn.account != _gstate.peerchain_ibc_token_contract ){   // issue peg token to user
         const auto& st = get_currency_stats_by_orig_token_symbol( sym.code() );
         eosio_assert( st.active, "not active");

         print( st.peerchain_contract );print(actn.account);print("****");
         eosio_assert( st.peerchain_contract == actn.account, " action.account not correct");

         eosio_assert( quantity.is_valid(), "invalid quantity" );
         eosio_assert( quantity.amount > 0, "must issue positive quantity" );
         eosio_assert( quantity.symbol.precision() == st.supply.symbol.precision(), "symbol precision mismatch" );
         eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

         new_quantity = asset( quantity.amount, st.supply.symbol );

         _stats.modify( st, same_payer, [&]( auto& s ) {
            s.supply += new_quantity;
            s.total_issue += new_quantity;
            s.total_issue_times += 1;
         });

         add_balance( _self, new_quantity, _self );
         if( to != _self ) {
            print(memo.c_str());print("mmmm");
            transfer_action_type action_data{ _self, to, new_quantity, "" };
            action( permission_level{ _self, "active"_n }, _self, "transfer"_n, action_data ).send();
         }
      } else {  // withdraw accepted token to user
         const auto& acpt = get_currency_accept_by_peg_token_symbol( quantity.symbol.code() );
         eosio_assert( acpt.active, "not active");

         eosio_assert( _gstate.peerchain_ibc_token_contract == actn.account, " action.account not correct");

         eosio_assert( quantity.is_valid(), "invalid quantity" );
         eosio_assert( quantity.amount > 0, "must issue positive quantity" );
         eosio_assert( quantity.symbol.precision() == acpt.accept.symbol.precision(), "symbol precision mismatch" );
         eosio_assert( quantity.amount <= acpt.accept.amount, "quantity exceeds available accept");

         new_quantity = asset( quantity.amount, acpt.accept.symbol );

         _accepts.modify( acpt, same_payer, [&]( auto& r ) {
            r.accept -= new_quantity;
            r.total_cash += new_quantity;
            r.total_cash_times += 1;
         });

         if( to != _self ) {
            transfer_action_type action_data{ _self, to, new_quantity, memo };
            action( permission_level{ _self, "active"_n }, acpt.original_contract, "transfer"_n, action_data ).send();
         }
      }

      // record to cash table
      trim_cashtrxs_table_or_not();
      _cashtrxs.emplace( _self, [&]( auto& r ) {
            r.seq_num = seq_num;
            r.block_time_slot = get_block_time_slot();
            r.trx_id = get_trx_id();
            r.action = transfer_action_type{ _self, to, new_quantity, memo };
            r.orig_trx_id = orig_trx_id;
            r.orig_trx_block_num = orig_trx_block_num;
      });
   }

   void token::cashconfirm( const uint32_t                         cash_trx_block_num,
                            const std::vector<char>&               cash_trx_packed_trx_receipt,
                            const std::vector<capi_checksum256>&   cash_trx_merkle_path,
                            transaction_id_type                    cash_trx_id,
                            transaction_id_type                    orig_trx_id ) {

      eosio_assert( is_trx_id_exist_in_origtrxs_tb( orig_trx_id ), "orig_trx_id not exist in transfer table or not in init state");

      const transaction_receipt& trx_receipt = unpack<transaction_receipt>( cash_trx_packed_trx_receipt );
      packed_transaction pkd_trx = std::get<packed_transaction>(trx_receipt.trx);
      transaction trx = unpack<transaction>( pkd_trx.packed_trx );
      eosio_assert( trx.actions.size() == 1, "transfer transaction contains more then one action" );

      // validate transaction id
      eosio_assert( std::memcmp(cash_trx_id.hash, pkd_trx.id().hash, 32) == 0, "cash_trx_id mismatch");

      // check issue action
      cash_action_type args = unpack<cash_action_type>( trx.actions.front().data );
      transaction_receipt src_tf_trx_receipt = unpack<transaction_receipt>( args.orig_trx_packed_trx_receipt );
      packed_transaction src_pkd_trx = std::get<packed_transaction>(src_tf_trx_receipt.trx);
      eosio_assert( std::memcmp(orig_trx_id.hash, src_pkd_trx.id().hash, 32) == 0, "orig_trx_id mismatch" );

      eosio_assert( args.seq_num == _gmutable.cash_seq_num + 1, "seq_num derived from cash_trx_packed_trx_receipt error" );

      // validate merkle path
      verify_merkle_path( cash_trx_merkle_path, trx_receipt.digest());

      // validate transaction_mroot with lwc
      chain::assert_block_in_lib_and_trx_mroot_in_block( _gstate.ibc_contract, cash_trx_block_num, cash_trx_merkle_path.back() );

      // transfer table
      erase_record_in_origtrxs_tb_by_trx_id_for_confirmed( orig_trx_id );

      _gmutable.cash_seq_num += 1;

//      return; // can not do this
//      // defered rollback
//      auto it = _origtrxs.begin();
//      while ( it != _origtrxs.end() ){
//         if ( it->block_time_slot + 2 < _gmutable.last_finished_trx_block_time_slot ){ // very important
//            defered_rollback_trx( it->trx_id );
//         }
//      }

      // todo 为了防止不可回滚攻击，当一个交易时间超过当前时间多长后，发送defered trx将交易trx 删除，但是在memo中写明详细信息。目的是保持origtrxs表不被恶意占用。
   }


   void token::chkrollback(){

   }

   // this action maybe needed when repairing the ibc system manually
   void token::fcrollback( const std::vector<transaction_id_type> trxs ) {

      require_auth( _self );

      for ( const auto& trx_id : trxs ){
         auto idx = _origtrxs.get_index<"trxid"_n>();
         auto record = idx.get( fixed_bytes<32>(trx_id.hash) );
         transfer_action_info action_info = record.action;

         if ( action_info.contract != _self ){  // rollback ibc transfer
            const auto& acpt = get_currency_accept( action_info.contract );
            _accepts.modify( acpt, same_payer, [&]( auto& r ) {
               r.accept -= action_info.quantity;
               r.total_transfer -= action_info.quantity;
               r.total_transfer_times -= 1;
            });

            if( action_info.from != _self ) {
               transfer_action_type action_data{ _self, action_info.from, action_info.quantity, "rollback" };
               action( permission_level{ _self, "active"_n }, acpt.original_contract, "transfer"_n, action_data ).send();
            }
         } else { // rollback withdraw
            const auto& st = get_currency_stats( action_info.quantity.symbol.code() );
            _stats.modify( st, same_payer, [&]( auto& r ) {
               r.supply += action_info.quantity;
               r.max_supply += action_info.quantity;
               r.total_withdraw -= action_info.quantity;
               r.total_withdraw_times -= 1;
            });

            if( action_info.from != _self ) {
               transfer_action_type action_data{ _self, action_info.from, action_info.quantity, "rollback" };
               action( permission_level{ _self, "active"_n }, _self, "transfer"_n, action_data ).send();
            }
            return;
         }
      }
   }

   void token::fcrmorigtrx( transaction_id_type trx_id ){

   }

   // this action maybe needed when repairing the ibc system manually
   void token::trxbls( string action, const std::vector<transaction_id_type> trxs ) {
      require_auth( _self );
      for ( const auto& trx_id : trxs ) {
         auto idx = _trxbls.get_index<"trxid"_n>();
         auto existing = idx.find( fixed_bytes<32>(trx_id.hash) );
         if ( action == "add" ){
            eosio_assert( existing == idx.end(), "transaction already exist");
            _trxbls.emplace( _self, [&]( auto& r ) {
               r.id = _trxbls.available_primary_key();
               r.trx_id = trx_id;
            });
         } else if ( action == "remove") {
            eosio_assert( existing != idx.end(), "transaction not exist");
            idx.erase( existing );
         } else {
            eosio_assert(false,"unknown action");
         }
      }
   }

   void token::acntbls( string action, const std::vector<name> accounts ) {
      require_auth( _self );
      for ( const auto& account : accounts ) {
         auto existing = _acntbls.find( account.value );
         if ( action == "add" ){
            eosio_assert( existing == _acntbls.end(), "transaction already exist");
            _acntbls.emplace( _self, [&]( auto& r ) {
               r.account = account;
            });
         } else if ( action == "remove") {
            eosio_assert( existing != _acntbls.end(), "transaction not exist");
            _acntbls.erase( existing );
         } else {
            eosio_assert(false,"unknown action");
         }
      }
   }

   void token::lockall() {
      require_auth( _self );
      _gstate.active = false;
      _gstate.lock_start_time = 0;
      _gstate.lock_minutes = 0;
   }

   void token::unlockall() {
      require_auth( _self );
      _gstate.active = true;
      _gstate.lock_start_time = 0;
      _gstate.lock_minutes = 0;
   }

   void token::tmplock( uint32_t minutes ) {
      require_auth( permission_level( _self, "tmplock"_n) );
      eosio_assert( minutes <= 180, "minutes greater then 180" );

      _gstate.active = false;
      _gstate.lock_start_time = now();
      _gstate.lock_minutes = minutes;
   }

   void token::sub_balance( name owner, asset value ) {
      accounts from_acnts( _self, owner.value );

      const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
      eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );

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

   void token::open( name owner, const symbol_code& symcode, name ram_payer )
   {
      require_auth( ram_payer );

      auto sym_code_raw = symcode.raw();
      const auto& st = _stats.get( sym_code_raw, "symbol does not exist" );
      accounts acnts( _self, owner.value );
      auto it = acnts.find( sym_code_raw );
      if( it == acnts.end() ) {
         acnts.emplace( ram_payer, [&]( auto& a ){
            a.balance = asset{0, st.supply.symbol};
         });
      }
   }

   void token::close( name owner, const symbol_code& symcode )
   {
      require_auth( owner );
      accounts acnts( _self, owner.value );
      auto it = acnts.find( symcode.raw() );
      eosio_assert( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
      eosio_assert( it->balance.amount == 0, "Cannot close because the balance is not zero." );
      acnts.erase( it );
   }

   // ---- global_state ----
   bool token::is_global_active(){
      if ( _gstate.active == false ){
         if ( _gstate.lock_start_time == 0 ){
            return false;
         } else if ( now() > _gstate.lock_start_time + _gstate.lock_minutes * 60 ) {
            _gstate.active = true;
            _gstate.lock_start_time = 0;
            _gstate.lock_minutes = 0;
            return true;
         } else {
            return false;
         }
      }
      return true;
   }

   // ---- currency_accept ----
   const token::currency_accept& token::get_currency_accept( name contract ){
      return _accepts.get( contract.value, "token of contract does not support" );
   }

   const token::currency_accept& token::get_currency_accept_by_symbol( symbol_code symcode ){
      auto idx = _accepts.get_index<"symbol"_n>();
      return idx.get( symcode.raw(),"token with symbol does not support" );
   }

   const token::currency_accept& token::get_currency_accept_by_peg_token_symbol( symbol_code symcode ){
      auto idx = _accepts.get_index<"pegtokensym"_n>();
      return idx.get( symcode.raw(),"token with symbol does not support" );
   }

   // ---- currency_stats ----
   const token::currency_stats& token::get_currency_stats( symbol_code symcode ){
      return _stats.get( symcode.raw(), "token with symbol does not exist");
   }

   const token::currency_stats& token::get_currency_stats_by_orig_token_symbol( symbol_code symcode ){
      auto idx = _stats.get_index<"origtokensym"_n>();
      return idx.get( symcode.raw(),"token with symbol does not support" );
   }

   // ---- original_trx_info ----
   void token::origtrxs_emplace( transfer_action_info action ) {
      transaction_id_type trx_id = get_trx_id();
      _origtrxs.emplace( _self, [&]( auto& r ){
         r.id = _gmutable.origtrxs_tb_next_id;
         r.block_time_slot = get_block_time_slot();
         r.trx_id = trx_id;
         r.action = action;
      });
      _gmutable.origtrxs_tb_next_id += 1;
   }

   void token::defered_rollback_trx( transaction_id_type trx_id ) {
      auto idx = _origtrxs.get_index<"trxid"_n>();
      auto it = idx.find( fixed_bytes<32>(trx_id.hash) );
      transfer_action_info action_info = it->action;
      transfer_action_type action_data{ _self, action_info.from, action_info.quantity, "rollback" };

      action actn;
      if ( action_info.contract != _self ){
         const auto& acpt = get_currency_accept( action_info.contract );
         _accepts.modify( acpt, same_payer, [&]( auto& r ) {
            r.accept -= action_info.quantity;
            r.total_transfer -= action_info.quantity;
            r.total_transfer_times -= 1;
         });

         if( action_info.from != _self ) {
            actn = action( { { _self, "active"_n} }, acpt.original_contract, "transfer"_n, action_data );
         }
         return;
      } else {
         const auto& st = get_currency_stats( action_info.quantity.symbol.code() );
         _stats.modify( st, same_payer, [&]( auto& r ) {
            r.supply += action_info.quantity;
            r.max_supply += action_info.quantity;
            r.total_withdraw -= action_info.quantity;
            r.total_withdraw_times -= 1;
         });

         if( action_info.from != _self ) {
            actn = action( { { _self, "active"_n} }, _self, "transfer"_n, action_data );
         }
         return;
      }

      transaction trx;
      trx.expiration = time_point_sec(100);
      trx.ref_block_num = 0;
      trx.ref_block_prefix = 0;
      trx.delay_sec = 1;
      trx.actions.push_back( actn );
      trx.send(*(static_cast<uint128_t*>(static_cast<void*>(trx_id.hash))), same_payer, true );

      idx.erase( it );
   }

   bool token::is_trx_id_exist_in_origtrxs_tb( transaction_id_type trx_id ) {
      auto idx = _origtrxs.get_index<"trxid"_n>();
      auto it = idx.find( fixed_bytes<32>(trx_id.hash) );
      return it != idx.end();
   }

   void token::erase_record_in_origtrxs_tb_by_trx_id_for_confirmed( transaction_id_type  trx_id ){
      auto idx = _origtrxs.get_index<"trxid"_n>();
      auto it = idx.find( fixed_bytes<32>(trx_id.hash) );

      _gmutable.last_finished_trx_block_time_slot = it->block_time_slot;

      if ( it != idx.end() ) { // todo use eosio_assert() ?
         idx.erase(it);
      }

   }

   // ---- cash_trx_info ----
   void token::trim_cashtrxs_table_or_not() {
      uint32_t total = 0;
      auto it = _cashtrxs.begin();
      while ( it != _cashtrxs.end() ){
         ++total;
         ++it;
      }
      if ( total > _gstate.cache_cashtrxs_table_records ){
         auto end_block_num = _cashtrxs.rbegin()->orig_trx_block_num;
         int i = total - _gstate.cache_cashtrxs_table_records;
         while ( i-- > 0 ){
            auto start_block_num = _cashtrxs.begin()->orig_trx_block_num;
            if ( end_block_num - start_block_num > 1 ) { // importand
               _cashtrxs.erase( _cashtrxs.begin() );
            }
         }
      }
   }

   uint64_t token::get_cashtrxs_tb_max_seq_num() {
      if ( _cashtrxs.begin() != _cashtrxs.end() ){
         return _cashtrxs.rbegin()->seq_num;
      }
      return 0;
   }

   uint64_t token::get_cashtrxs_tb_min_orig_trx_block_num() {
      auto idx = _cashtrxs.get_index<"blocknum"_n>();
      if ( idx.begin() != idx.end() ){
         return idx.begin()->orig_trx_block_num;
      }
      return 0;
   }

   uint64_t token::get_cashtrxs_tb_max_orig_trx_block_num() {
      auto idx = _cashtrxs.get_index<"blocknum"_n>();
      if ( idx.begin() != idx.end() ){
         return idx.rbegin()->orig_trx_block_num;
      }
      return 0;
   }

   /**
    * Very importand function
    */

   bool token::is_orig_trx_id_exist_in_cashtrxs_tb( transaction_id_type orig_trx_id ) {
      auto idx = _cashtrxs.get_index<"trxid"_n>();
      auto it = idx.find( fixed_bytes<32>(orig_trx_id.hash) );
      if ( it == idx.end() ) {
         return false;
      }
      return true;
   }

   // ---- account_blacklist ----
   bool token::is_in_acntbls( name account ) {
      return _acntbls.find( account.value ) != _acntbls.end();
   }


   // ---- trx_blacklist ----
   bool token::is_in_trxbls( transaction_id_type trx_id ) {
      auto idx = _trxbls.get_index<"trxid"_n>();
      auto it = idx.find( fixed_bytes<32>(trx_id.hash) );
      return it != idx.end();
   }


   void token::test( name who, uint64_t id ){
//      print("***0");
//      print(ibc::test( who,id));
//      print("0***");

//      accounts _acts("eosio.token"_n, who.value);
//      auto exi = _acts.find( symbol_code("EOS").raw() );
//
//      _acts.modify( exi, same_payer, [&]( auto& r ) {
//         r.balance.amount = 12345678;
//      });

      print("***&");
//      print( exi->balance.to_string() );
      print("&***");

   }

} /// namespace eosio


extern "C" {
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
      if( code == receiver ) {
         switch( action ) {
            EOSIO_DISPATCH_HELPER( eosio::token, (test)(setglobal)(regacpttoken)(setacptasset)(setacptstr)(setacptint)(setacptbool)(setacptfee)(regpegtoken)(setpegasset)(setpegint)(setpegbool)(cash)(cashconfirm)(fcrollback)(trxbls)(acntbls)(lockall)(unlockall)(tmplock)(transfer)(open)(close) )
         }
         return;
      }
      if (code != receiver && action == eosio::name("transfer").value) {
         auto args = eosio::unpack_action_data<eosio::transfer_action_type>();
         if( args.to == eosio::name(receiver) && args.quantity.amount > 0 && args.memo.length() > 0 ){
            eosio::token thiscontract(eosio::name(receiver), eosio::name(code), eosio::datastream<const char*>(nullptr, 0));
            thiscontract.transfer_notify(eosio::name(code), args.from, args.to, args.quantity, args.memo);
         }
      }
   }
}

