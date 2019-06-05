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
    check( type == arbitrator_type::profession || type == arbitrator_type::amateur, "Arbitrator type can only be 1 or 2." );
    auto abr_table = arbitrators( get_self(), get_self().value );
    auto iter = abr_table.find( account.value );
    check( iter == abr_table.end(), "Arbitrator already registered" );
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
    check( arbi_method == arbi_method_type::crowd_arbitration || arbi_method_type::multiple_rounds, "`arbi_method` can only be 1 or 2." );

    data_services svctable(_self, _self.value);
    auto svc_iter = svctable.find(service_id);
    check(svc_iter != svctable.end(), "service does not exist");
    check(svc_iter->status == service_status::service_in, "service status shoule be service_in");
    transfer(applicant, arbitrat_account, amount, "complain deposit.");

    auto complainant_tb = complainants( get_self(), get_self().value );
    auto complainant_by_svc = complainant_tb.template get_index<"svc"_n>();
    auto iter_compt = complainant_by_svc.find( service_id );
    auto is_sponsor = false;
    if ( iter_compt == complainant_by_svc.end() ) {
        is_sponsor = true;
    } else {
        check( iter_compt->status == complainant_status::wait_for_accept, "This complainant is not available." );
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

    // add_freeze
    add_delay(service_id, applicant, time_point_sec(now()), 1, amount);

    // Arbitration case application
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbicaseapp_tb_by_svc = arbicaseapp_tb.template get_index<"svc"_n>();
    auto arbicaseapp_iter = arbicaseapp_tb_by_svc.find( service_id );
    if (arbicaseapp_iter != arbicaseapp_tb_by_svc.end()) {
        auto arbi_id = arbicaseapp_tb.available_primary_key();
        arbicaseapp_tb.emplace( get_self(), [&]( auto& p ) {
            p.arbitration_id = arbi_id;
            p.appeal_id = appeal_id;
            p.service_id = service_id;
            p.evidence_info = reason;
            p.arbi_step = 1;
            p.required_arbitrator = 5;
            p.deadline = time_point_sec(now() + 3600);
            p.add_applicant(applicant);
        } );
    } else {
        // Check the last aribiration process time.
        if (arbicaseapp_iter->last_process_update_time + time_point_sec(arbi_process_time_limit) > time_point_sec(now())) {
            // Someone complains about this arbitration.
            random_chose_arbitrator(arbicaseapp_iter->arbitration_id, arbicaseapp_iter->service_id);
        } else {
            arbicaseapp_tb.modify(arbicaseapp_iter, get_self(), [&]( auto& p ) {
                p.arbi_step = arbi_step_type::arbi_end;
            } );
            auto notify_amount = eosio::asset(1, _bos_symbol);
            auto memo = "arbitration_id: " + std::to_string(arbicaseapp_iter->arbitration_id)
                + ", service_id: " + std::to_string(arbicaseapp_iter->service_id)
                + ", arbitration finished.";

            auto arbitrator_tb = arbitrators( get_self(), get_self().value );
            for (auto iter = arbicaseapp_iter->arbitrators.begin(); iter != arbicaseapp_iter->arbitrators.end(); iter++) {
                transfer(get_self(), *iter, notify_amount, memo);
                auto arbitrator_iter = arbitrator_tb.find(*iter.value);
                transfer(get_self(), *iter, arbitrator_iter->stake_amount, memo);
            }

            for (auto iter = arbicaseapp_iter->applicants.begin(); iter != arbicaseapp_iter->applicants.end(); iter++) {
                transfer(get_self(), *iter, notify_amount, memo);
            }

            // TODO: Update arbitration correction.
        }
    }
    
    // Data provider
    auto svcprovider_tb = data_service_provisions( get_self(), get_self().value );
    auto svcprovider_tb_by_svc = svcprovider_tb.template get_index<"bysvcid"_n>();
    auto svcprovider_iter = svcprovider_tb_by_svc.find( service_id );
    check(svcprovider_iter->stop_service == false, "service stopped.");
    
    auto notify_amount = eosio::asset(1, _bos_symbol);
    // Transfer to provider
    auto memo = "arbitration_id: " + std::to_string(arbi_id) 
        + ", service_id: " + std::to_string(service_id) 
        + ", state_amount: " + amount.to_string();
    transfer(get_self(), svcprovider_iter->account, notify_amount, memo);

    start_arbitration(arbitrator_type::profession, arbi_id, service_id);
}

void bos_oracle::uploadeviden( name applicant, uint64_t process_id, std::string evidence ) {
    require_auth( applicant );
    auto arbiprocess_tb = arbitration_processs( get_self(), get_self().value );
    auto arbipro_iter = arbiprocess_tb.find( process_id );
    check( arbipro_iter != arbiprocess_tb.end(), "Can not find such process.");

    arbiprocess_tb.modify( arbipro_iter, get_self(), [&]( auto& p ) {
        p.evidence_info = evidence;
    } );
}

void bos_oracle::uploadresult( name arbitrator, uint64_t arbitration_id, uint64_t result, uint64_t process_id ) {
    require_auth( arbitrator );
    check(result == 0 || result == 1, "`result` can only be 0 or 1.");

    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbi_iter = arbicaseapp_tb.find( arbitration_id );
    check(arbi_iter != arbicaseapp_tb.end(), "Can not find such arbitration case application.");

    // Check if all the arbitrators submit the result on time.
    if (arbi_iter->deadline <= time_point_sec(now())) {
        random_chose_arbitrator(arbitration_id, arbi_iter->service_id);
        return;
    }

    auto arbiprocess_tb = arbitration_processs( get_self(), get_self().value );
    auto arbipro_iter = arbiprocess_tb.find( process_id );
    check( arbipro_iter != arbiprocess_tb.end(), "Can not find such process.");

    arbiprocess_tb.modify( arbipro_iter, get_self(), [&]( auto& p ) {
        p.num_id += 1;
        p.add_result(result);
        p.update_time = time_point_sec(now());
    } );

    // Calculate results
    if (arbipro_iter->result_size() >= arbi_iter->required_arbitrator) {
        uint64_t arbi_result = 0;
        if (arbipro_iter->total_result() >= arbi_iter->required_arbitrator / 2) {
            arbi_result = 1;
        } else {
            arbi_result = 0;
        }
        arbicaseapp_tb.modify( arbi_iter, get_self(), [&]( auto& p) {
            p.last_process_update_time = time_point_sec(now());
        } );
        arbiprocess_tb.modify( arbipro_iter, get_self(), [&]( auto& p ) {
            p.arbitration_result = arbi_result;
        } );
        arbicaseapp_tb.modify( arbipro_iter, get_self(), [&]( auto& p ) {
            p.final_result = arbi_result;
        } );
        auto notify_amount = eosio::asset(1, _bos_symbol);
        auto memo = "arbitration_id: " + std::to_string(arbitration_id)
            + ", service_id: " + std::to_string(service_id)
            + ", result: success.";

        for (auto iter = arbi_iter->arbitrators.begin(); iter != arbi_iter->arbitrators.end(); iter++) {
            transfer(get_self(), *iter, notify_amount, memo);
        }
        for (auto iter = arbi_iter->applicants.begin(); iter != arbi_iter->applicants.end(); iter++) {
            transfer(get_self(), *iter, notify_amount, memo);
        }
    }
}

void bos_oracle::resparbitrat( name arbitrator, asset amount, uint64_t arbitration_id ) {
    require_auth( arbitrator );
    transfer(arbitrator, arbitrat_account, amount, "resparbitrat deposit.");

    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbi_iter = arbicaseapp_tb.find( arbitration_id );

    arbicaseapp_tb.modify( arbi_iter, get_self(), [&]( auto& p ) {
        p.arbi_step = arbi_step_type::arbi_responded;
        p.add_arbitrator(arbitrator);
    } );

    // Check arbitrator number requirements.
    if (arbi_iter->arbitrators.size() >= arbi_iter->required_arbitrator) {
        arbicaseapp_tb.modify( arbi_iter, get_self(), [&]( auto& p ) {
            p.arbi_step = arbi_step_type::arbi_started;
        } );
    } else {
        random_chose_arbitrator(arbitration_id, arbi_iter->service_id);
    }
}

void bos_oracle::respcase( name arbitrator, uint64_t arbitration_id, uint64_t result, uint64_t process_id ) {
    require_auth( arbitrator );
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbi_iter = arbicaseapp_tb.find( arbitration_id );
    check(arbi_iter != arbicaseapp_tb.end(), "Can not find such arbitration case application.");

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
            p.add_responder(arbitrator);
            p.num_id = 1;
        } );
    } else {
        arbiprocess_tb.modify( arbipro_iter, get_self(), [&]( auto& p ) {
            p.arbitration_id = arbitration_id;
            p.add_responder(arbitrator);
            p.num_id += 1;
        } );
    }
}

void bos_oracle::random_chose_arbitrator(uint64_t arbitration_id, uint64_t service_id) {
    auto arbitrator = random_arbitrator(arbitration_id);
    auto notify_amount = eosio::asset(1, _bos_symbol);
    // Transfer to arbitrator
    auto memo = "arbitration_id: " + std::to_string(arbitration_id)
        + ", service_id: " + std::to_string(service_id);
    transfer(get_self(), arbitrator, notify_amount, memo);
}

void bos_oracle::start_arbitration(arbitrator_type arbitype, uint64_t arbitration_id, uint64_t service_id) {
    random_chose_arbitrator(arbitration_id, service_id);
}

name bos_oracle::random_arbitrator(uint64_t arbitration_id) {
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto iter_arbicaseapp = arbicaseapp_tb.find( arbitration_id );
    auto chosen_arbitrators = iter_arbicaseapp->arbitrators;
    std::vector<name> chosen_from_arbitrators;

    auto arb_table = arbitrators( get_self(), get_self().value );
    for (auto iter = arb_table.begin(); iter != arb_table.end(); iter++)
    {
        auto chosen = std::find(chosen_arbitrators.begin(), chosen_arbitrators.end(), iter->account);
        if (chosen == chosen_arbitrators.end() && !iter->is_malicious) {
            chosen_from_arbitrators.push_back(iter->account);
        }
    }

    auto total_arbi = chosen_from_arbitrators.size();
    auto tmp = tapos_block_prefix();
    auto arbi_id = random((void*)&tmp, sizeof(tmp));
    arbi_id %= total_arbi;

    return chosen_from_arbitrators.at(arbi_id);
}
