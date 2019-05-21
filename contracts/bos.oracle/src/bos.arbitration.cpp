/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */

#include <eosiolib/asset.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/time.hpp>
#include <string>
#include "bos.oracle/bos.oracle.hpp"
// namespace eosio {

using eosio::asset;
using eosio::public_key;
using std::string;

void bos_oracle::regarbitrat( name account, public_key pubkey, uint8_t type, asset stake_amount, std::string public_info ) {
    require_auth( account );
    eosio_assert( type == arbitrator_type::profession || type == arbitrator_type::amateur, "Arbitrator type can only be 1 or 2." );
    auto abr_table = arbitrators( get_self(), get_self().value );
    auto iter = abr_table.find( account.value );
    eosio_assert( iter == abr_table.end(), "Arbitrator already registered" );
    transfer(account, arbitrat_account, stake_amount, "regarbitrat deposit.");

    abr_table.emplace( get_self(), [&]( auto& p ) {
        p.account = account;
        p.pubkey = pubkey;
        p.type = type;
        p.stake_amount = stake_amount;
        p.public_info = public_info;
    } );
}

void bos_oracle::complain( name applicant, uint64_t service_id, asset amount, std::string reason, uint8_t arbi_method ) {
    require_auth( applicant );
    eosio_assert( arbi_method == arbi_method_type::crowd_arbitration || arbi_method_type::multiple_rounds, "`arbi_method` can only be 1 or 2." );

    data_services svctable(_self, _self.value);
    auto svc_iter = svctable.find(service_id);
    eosio_assert(svc_iter != svctable.end(), "service does not exist");
    eosio_assert(svc_iter->status == data_service_status::service_in, "service status shoule be service_in");
    transfer(applicant, arbitrat_account, amount, "complain deposit.");

    auto complainant_tb = complainants( get_self(), get_self().value );
    auto complainant_by_svc = complainant_tb.template get_index<"svc"_n>();
    auto iter_compt = complainant_by_svc.find( service_id );
    auto is_sponsor = false;
    if ( iter_compt == complainant_by_svc.end() ) {
        is_sponsor = true;
    } else {
        eosio_assert( iter_compt->status == complainant_status::wait_for_accept, "This complainant is not available." );
    }
    
    auto appeal_id = 0;
    complainant_tb.emplace( get_self(), [&]( auto& p ) {
        p.appeal_id = complainant_tb.available_primary_key();
        p.service_id = service_id;
        p.status = complainant_status::wait_for_accept;
        p.arbi_method = arbi_method;
        p.is_sponsor = is_sponsor;
        p.applicant = applicant;
        p.appeal_time = time_point_sec(now());
        p.reason = reason;
        appeal_id = p.appeal_id;
    } );

    // Arbitration case application
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbi_id = arbicaseapp_tb.available_primary_key();
    arbicaseapp_tb.emplace( get_self(), [&]( auto& p ) {
        p.arbitration_id = arbi_id;
        p.appeal_id = appeal_id;
        p.service_id = service_id;
        p.evidence_info = reason;
        // p.update_number = 
        p.arbi_step = 1;
    } );

    // Data provider
    auto svcprovider_tb = data_service_provisions( get_self(), get_self().value );
    auto svcprovider_tb_by_svc = svcprovider_tb.template get_index<"bysvcid"_n>();
    auto svcprovider_iter = svcprovider_tb_by_svc.find( service_id );
    eosio_assert(svcprovider_iter->stop_service == false, "service stopped.");
    
    auto notify_amount = eosio::asset(1, _bos_symbol);
    // Transfer to provider
    auto memo = "arbitration_id: " + std::to_string(arbi_id) 
        + ", service_id: " + std::to_string(service_id) 
        + ", state_amount: " + amount.to_string();
    transfer(get_self(), svcprovider_iter->account, notify_amount, memo);
}

void bos_oracle::uploadeviden( name applicant, uint64_t process_id, std::string evidence ) {
    require_auth( applicant );
    auto arbiprocess_tb = arbitration_processs( get_self(), get_self().value );
    auto arbipro_iter = arbiprocess_tb.find( process_id );
    eosio_assert( arbipro_iter != arbiprocess_tb.end(), "Can not find such process.");

    arbiprocess_tb.modify( arbipro_iter, get_self(), [&]( auto& p ) {
        p.evidence_info = evidence;
    } );
}

void bos_oracle::uploadresult( name arbitrator, uint64_t arbitration_id, uint64_t result, uint64_t process_id ) {
    require_auth( arbitrator );

    auto arbiprocess_tb = arbitration_processs( get_self(), get_self().value );
    auto arbipro_iter = arbiprocess_tb.find( process_id );
    eosio_assert( arbipro_iter != arbiprocess_tb.end(), "Can not find such process.");

    arbiprocess_tb.modify( arbipro_iter, get_self(), [&]( auto& p ) {
        p.num_id += 1;
        p.arbitration_result = result;
    } );
}

void bos_oracle::resparbitrat( name arbitrator, asset amount, uint64_t arbitration_id, signature sig ) {
    require_auth( arbitrator );
    transfer(arbitrator, arbitrat_account, amount, "resparbitrat deposit.");

    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbi_iter = arbicaseapp_tb.find( arbitration_id );

    arbicaseapp_tb.modify( arbi_iter, get_self(), [&]( auto& p ) {
        p.arbi_step = arbi_step_type::arbi_end;
    } );
}

void bos_oracle::respcase( name arbitrator, uint64_t arbitration_id, uint64_t result, uint64_t process_id ) {
    require_auth( arbitrator );
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbi_iter = arbicaseapp_tb.find( arbitration_id );

    arbicaseapp_tb.modify( arbi_iter, get_self(), [&]( auto& p ) {
        p.arbi_step = arbi_iter->arbi_step + 1;
    } );

    auto complainant_tb = complainants( get_self(), get_self().value );
    auto complainant_iter = complainant_tb.find( arbi_iter->appeal_id );

    auto arbiprocess_tb = arbitration_processs( get_self(), get_self().value );
    auto arbipro_iter = arbiprocess_tb.find( process_id );
    if ( arbipro_iter == arbiprocess_tb.end() ) {
        arbiprocess_tb.emplace( get_self(), [&]( auto& p ) {
            p.process_id = arbiprocess_tb.available_primary_key();
            p.arbitration_id = arbitration_id;
            p.num_id = 1;
            p.add_applicant(complainant_iter->applicant);
        } );
    } else {
        arbiprocess_tb.modify( arbipro_iter, get_self(), [&]( auto& p ) {
            p.arbitration_id = arbitration_id;
            p.num_id += 1;
            p.add_applicant(complainant_iter->applicant);
        } );
    }
}