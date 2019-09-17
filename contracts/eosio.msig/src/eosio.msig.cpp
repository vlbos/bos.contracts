#include <eosio.msig/eosio.msig.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/permission.hpp>
#include <eosiolib/crypto.hpp>

namespace eosio {

time_point current_time_point() {
   const static time_point ct{ microseconds{ static_cast<int64_t>( current_time() ) } };
   return ct;
}

void multisig::propose( ignore<name> proposer,
                        ignore<name> proposal_name,
                        ignore<std::vector<permission_level>> requested,
                        ignore<transaction> trx )
{
   name _proposer;
   name _proposal_name;
   std::vector<permission_level> _requested;
   transaction_header _trx_header;

   _ds >> _proposer >> _proposal_name >> _requested;

   const char* trx_pos = _ds.pos();
   size_t size    = _ds.remaining();
   _ds >> _trx_header;

   require_auth( _proposer );
   check( _trx_header.expiration >= eosio::time_point_sec(current_time_point()), "transaction expired" );
   //check( trx_header.actions.size() > 0, "transaction must have at least one action" );

   proposals proptable( _self, _proposer.value );
   check( proptable.find( _proposal_name.value ) == proptable.end(), "proposal with the same name exists" );

   auto packed_requested = pack(_requested);
   auto res = ::check_transaction_authorization( trx_pos, size,
                                                 (const char*)0, 0,
                                                 packed_requested.data(), packed_requested.size()
                                               );
   check( res > 0, "transaction authorization failed" );

   std::vector<char> pkd_trans;
   pkd_trans.resize(size);
   memcpy((char*)pkd_trans.data(), trx_pos, size);
   proptable.emplace( _proposer, [&]( auto& prop ) {
      prop.proposal_name       = _proposal_name;
      prop.packed_transaction  = pkd_trans;
   });

   approvals apptable(  _self, _proposer.value );
   apptable.emplace( _proposer, [&]( auto& a ) {
      a.proposal_name       = _proposal_name;
      a.requested_approvals.reserve( _requested.size() );
      for ( auto& level : _requested ) {
         a.requested_approvals.push_back( approval{ level, time_point{ microseconds{0} } } );
      }
   });
}

void multisig::approve( name proposer, name proposal_name, permission_level level,
                        const eosio::binary_extension<eosio::checksum256>& proposal_hash )
{
   require_auth( level );

   if( proposal_hash ) {
      proposals proptable( _self, proposer.value );
      auto& prop = proptable.get( proposal_name.value, "proposal not found" );
      assert_sha256( prop.packed_transaction.data(), prop.packed_transaction.size(), *proposal_hash );
   }

   //check whether opposed or abstained to this proposal.
   opposes oppotable(_self, proposer.value );
   auto oppos_it = oppotable.find(proposal_name.value );
   if(oppos_it != oppotable.end() ){
	  auto check_itr = std::find( oppos_it->opposed_approvals.begin(), oppos_it->opposed_approvals.end(), level );
	  check( check_itr == oppos_it->opposed_approvals.end(), "you already opposed this proposal" );
	  check_itr = std::find( oppos_it->abstained_approvals.begin(), oppos_it->abstained_approvals.end(), level );
	  check( check_itr == oppos_it->abstained_approvals.end(), "you already abstained this proposal" );
   }

   approvals apptable(  _self, proposer.value );
   auto apps_it = apptable.find( proposal_name.value );
   if ( apps_it != apptable.end() ) {
      auto itr = std::find_if( apps_it->requested_approvals.begin(), apps_it->requested_approvals.end(), [&](const approval& a) { return a.level == level; } );
      check( itr != apps_it->requested_approvals.end(), "approval is not on the list of requested approvals" );

      apptable.modify( apps_it, proposer, [&]( auto& a ) {
            a.provided_approvals.push_back( approval{ level, current_time_point() } );
            a.requested_approvals.erase( itr );
         });
   } else {
      old_approvals old_apptable(  _self, proposer.value );
      auto& apps = old_apptable.get( proposal_name.value, "proposal not found" );

      auto itr = std::find( apps.requested_approvals.begin(), apps.requested_approvals.end(), level );
      check( itr != apps.requested_approvals.end(), "approval is not on the list of requested approvals" );

      old_apptable.modify( apps, proposer, [&]( auto& a ) {
            a.provided_approvals.push_back( level );
            a.requested_approvals.erase( itr );
         });
   }
}

void multisig::unapprove( name proposer, name proposal_name, permission_level level ) {
   require_auth( level );

   approvals apptable(  _self, proposer.value );
   auto apps_it = apptable.find( proposal_name.value );
   if ( apps_it != apptable.end() ) {
      auto itr = std::find_if( apps_it->provided_approvals.begin(), apps_it->provided_approvals.end(), [&](const approval& a) { return a.level == level; } );
      check( itr != apps_it->provided_approvals.end(), "no approval previously granted" );
      apptable.modify( apps_it, proposer, [&]( auto& a ) {
            a.requested_approvals.push_back( approval{ level, current_time_point() } );
            a.provided_approvals.erase( itr );
         });
   } else {
      old_approvals old_apptable(  _self, proposer.value );
      auto& apps = old_apptable.get( proposal_name.value, "proposal not found" );
      auto itr = std::find( apps.provided_approvals.begin(), apps.provided_approvals.end(), level );
      check( itr != apps.provided_approvals.end(), "no approval previously granted" );
      old_apptable.modify( apps, proposer, [&]( auto& a ) {
            a.requested_approvals.push_back( level );
            a.provided_approvals.erase( itr );
         });
   }
}

void multisig::cancel( name proposer, name proposal_name, name canceler ) {
   require_auth( canceler );

   proposals proptable( _self, proposer.value );
   auto& prop = proptable.get( proposal_name.value, "proposal not found" );

   if( canceler != proposer ) {
      check( unpack<transaction_header>( prop.packed_transaction ).expiration < eosio::time_point_sec(current_time_point()), "cannot cancel until expiration" );
   }
   proptable.erase(prop);

   //remove from new table
   approvals apptable(  _self, proposer.value );
   auto apps_it = apptable.find( proposal_name.value );
   if ( apps_it != apptable.end() ) {
      apptable.erase(apps_it);
   } else {
      old_approvals old_apptable(  _self, proposer.value );
      auto apps_it = old_apptable.find( proposal_name.value );
      check( apps_it != old_apptable.end(), "proposal not found" );
      old_apptable.erase(apps_it);
   }

   //remove from oppose table
   opposes oppotable(_self, proposer.value );
   auto oppo_it = oppotable.find( proposal_name.value );
   if( oppo_it != oppotable.end() ){
   	  oppotable.erase(oppo_it);
   	}
}

void multisig::oppose( name proposer, name proposal_name, permission_level level,
                        const eosio::binary_extension<eosio::checksum256>& proposal_hash ){
   require_auth( level );
   if( proposal_hash ) {
	  proposals proptable( _self, proposer.value );
	  auto& prop = proptable.get( proposal_name.value, "proposal not found" );
	  assert_sha256( prop.packed_transaction.data(), prop.packed_transaction.size(), *proposal_hash );
   }

   //check whether level is in requested approvals or already approved to it
   approvals apptable( _self, proposer.value );
   auto app_it = apptable.find(proposal_name.value );
   if( app_it != apptable.end() ){
      auto requested_it = std::find_if( app_it->requested_approvals.begin(), app_it->requested_approvals.end(), [&](const approval& a) { return a.level == level; } );
      bool requested = (requested_it != app_it->requested_approvals.end());
      auto approved_it = std::find_if( app_it->provided_approvals.begin(), app_it->provided_approvals.end(), [&](const approval &a){ return a.level == level;} );
      bool approved = (approved_it != app_it->provided_approvals.end());
	  check( requested && !approved , "provided permission not requested, or you have approved to this proposal" );
   }else{
      old_approvals old_apptable( _self, proposer.value );
      auto old_app_it = old_apptable.find( proposal_name.value );
      if( old_app_it != old_apptable.end() ){
         auto requested_it = std::find( old_app_it->requested_approvals.begin(), old_app_it->requested_approvals.end(), level );
         bool requested =  (requested_it != old_app_it->requested_approvals.end());
         auto approved_it = std::find( old_app_it->provided_approvals.begin(), old_app_it->provided_approvals.end(), level );
		 bool approved = (approved_it != old_app_it->provided_approvals.end());
         check( requested && !approved , "provided permission not requested, or you have approved to this proposal" );
      }
   }

   //check whether abstained/opposed to this proposal
   //update opposed
   opposes oppotable(	_self, proposer.value );
   auto oppo_it = oppotable.find( proposal_name.value );
   if ( oppo_it != oppotable.end() ) {
	  auto abs_check_it = std::find( oppo_it->abstained_approvals.begin(), oppo_it->abstained_approvals.end(), level );
	  check( abs_check_it == oppo_it->abstained_approvals.end(), "you have abstained to this proposal" );
      auto oppo_check_it = std::find( oppo_it->opposed_approvals.begin(), oppo_it->opposed_approvals.end(), level);
	  check( oppo_check_it == oppo_it->opposed_approvals.end(), "you have opposed to this proposal" );
	  oppotable.modify( oppo_it, proposer, [&]( auto& o ) {
			o.opposed_approvals.push_back( level );
	     });
   }else{
      oppotable.emplace( proposer, [&]( auto& o ) {
            o.proposal_name       = proposal_name;
            o.opposed_approvals.push_back( level );
         });
   }
}

void multisig::unoppose( name proposer, name proposal_name, permission_level level ){
   require_auth( level );

   opposes oppotable(  _self, proposer.value );
   auto& oppo_it = oppotable.get( proposal_name.value, "can't find this proposal in opposes table" );
   auto itr = std::find( oppo_it.opposed_approvals.begin(), oppo_it.opposed_approvals.end(), level );
   check( itr != oppo_it.opposed_approvals.end(), "no oppose found" );
   oppotable.modify( oppo_it, proposer, [&]( auto& o ) {
         o.opposed_approvals.erase( itr );
      });
}

void multisig::abstain( name proposer, name proposal_name, permission_level level,
                        const eosio::binary_extension<eosio::checksum256>& proposal_hash ){
   require_auth( level );

   if( proposal_hash ) {
	  proposals proptable( _self, proposer.value );
	  auto& prop = proptable.get( proposal_name.value, "proposal not found" );
	  assert_sha256( prop.packed_transaction.data(), prop.packed_transaction.size(), *proposal_hash );
   }

   //check whether level is in requested approvals or already approved to it
   approvals apptable( _self, proposer.value );
   auto app_it = apptable.find(proposal_name.value );
   if( app_it != apptable.end() ){
      auto requested_it = std::find_if( app_it->requested_approvals.begin(), app_it->requested_approvals.end(), [&](const approval& a) { return a.level == level; } );
      bool requested = (requested_it != app_it->requested_approvals.end());
      auto approved_it = std::find_if( app_it->provided_approvals.begin(), app_it->provided_approvals.end(), [&](const approval &a){ return a.level == level;} );
      bool approved = (approved_it != app_it->provided_approvals.end());
	  check( requested && !approved , "provided permission not requested, or you have approved to this proposal" );
   }else{
      old_approvals old_apptable( _self, proposer.value );
      auto old_app_it = old_apptable.find( proposal_name.value );
      if( old_app_it != old_apptable.end() ){
         auto requested_it = std::find( old_app_it->requested_approvals.begin(), old_app_it->requested_approvals.end(), level );
         bool requested = (requested_it != old_app_it->requested_approvals.end());
         auto approved_it = std::find( old_app_it->provided_approvals.begin(), old_app_it->provided_approvals.end(), level );
		 bool approved = (approved_it != old_app_it->provided_approvals.end());
         check( requested && !approved , "provided permission not requested, or you have approved to this proposal" );
      }
   }

   //check whether abstained/opposed to this proposal
   //update abstained
   opposes oppotable(	_self, proposer.value );
   auto abs_it = oppotable.find( proposal_name.value );
   if ( abs_it != oppotable.end() ) {
	  auto abs_check_it = std::find( abs_it->abstained_approvals.begin(), abs_it->abstained_approvals.end(), level );
	  check( abs_check_it == abs_it->abstained_approvals.end(), "you have abstained to this proposal" );
      auto oppo_check_it = std::find( abs_it->opposed_approvals.begin(), abs_it->opposed_approvals.end(), level);
	  check( oppo_check_it == abs_it->opposed_approvals.end(), "you have opposed to this proposal" );
	  oppotable.modify( abs_it, proposer, [&]( auto& a ) {
			a.abstained_approvals.push_back( level );
	     });
   }else{
      oppotable.emplace( proposer, [&]( auto& a ) {
            a.proposal_name       = proposal_name;
            a.abstained_approvals.push_back( level );
         });
   }
}

void multisig::unabstain( name proposer, name proposal_name, permission_level level ){
   require_auth( level );

   opposes oppotable(  _self, proposer.value );
   auto& abs_it = oppotable.get( proposal_name.value, "can't find this proposal in opposes table" );
   auto itr = std::find( abs_it.abstained_approvals.begin(), abs_it.abstained_approvals.end(), level );
   check( itr != abs_it.abstained_approvals.end(), "no abstain found" );
   oppotable.modify( abs_it, proposer, [&]( auto& a ) {
         a.abstained_approvals.erase( itr );
      });
}

void multisig::exec( name proposer, name proposal_name, name executer ) {
   require_auth( executer );

   proposals proptable( _self, proposer.value );
   auto& prop = proptable.get( proposal_name.value, "proposal not found" );
   transaction_header trx_header;
   datastream<const char*> ds( prop.packed_transaction.data(), prop.packed_transaction.size() );
   ds >> trx_header;
   check( trx_header.expiration >= eosio::time_point_sec(current_time_point()), "transaction expired" );

   approvals apptable(  _self, proposer.value );
   auto apps_it = apptable.find( proposal_name.value );
   std::vector<permission_level> approvals;
   invalidations inv_table( _self, _self.value );
   if ( apps_it != apptable.end() ) {
      approvals.reserve( apps_it->provided_approvals.size() );
      for ( auto& p : apps_it->provided_approvals ) {
         auto it = inv_table.find( p.level.actor.value );
         if ( it == inv_table.end() || it->last_invalidation_time < p.time ) {
            approvals.push_back(p.level);
         }
      }
      apptable.erase(apps_it);
   } else {
      old_approvals old_apptable(  _self, proposer.value );
      auto& apps = old_apptable.get( proposal_name.value, "proposal not found" );
      for ( auto& level : apps.provided_approvals ) {
         auto it = inv_table.find( level.actor.value );
         if ( it == inv_table.end() ) {
            approvals.push_back( level );
         }
      }
      old_apptable.erase(apps);
   }
   auto packed_provided_approvals = pack(approvals);
   auto res = ::check_transaction_authorization( prop.packed_transaction.data(), prop.packed_transaction.size(),
                                                 (const char*)0, 0,
                                                 packed_provided_approvals.data(), packed_provided_approvals.size()
                                                 );
   check( res > 0, "transaction authorization failed" );

   send_deferred( (uint128_t(proposer.value) << 64) | proposal_name.value, executer.value,
                  prop.packed_transaction.data(), prop.packed_transaction.size() );

   proptable.erase(prop);

   //clear opposes
   opposes oppotable(_self, proposer.value);
   auto oppos_it = oppotable.find( proposal_name.value );
   if( oppos_it != oppotable.end() ){
      oppotable.erase( oppos_it );
   }
}

void multisig::invalidate( name account ) {
   require_auth( account );
   invalidations inv_table( _self, _self.value );
   auto it = inv_table.find( account.value );
   if ( it == inv_table.end() ) {
      inv_table.emplace( account, [&](auto& i) {
            i.account = account;
            i.last_invalidation_time = current_time_point();
         });
   } else {
      inv_table.modify( it, account, [&](auto& i) {
            i.last_invalidation_time = current_time_point();
         });
   }
}

} /// namespace eosio

EOSIO_DISPATCH( eosio::multisig, (propose)(approve)(unapprove)(cancel)(oppose)(unoppose)(abstain)(unabstain)(exec)(invalidate) )
