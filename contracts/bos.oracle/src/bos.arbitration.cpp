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

    data_services svctable(get_self(), get_self().value);
    auto svc_iter = svctable.find(service_id);
    check(svc_iter != svctable.end(), "service does not exist");
    check(svc_iter->status == service_status::service_in, "service status shoule be service_in");
    transfer(applicant, arbitrat_account, amount, "complain deposit.");

    auto complainant_tb = complainants( get_self(), get_self().value );
    auto complainant_by_svc = complainant_tb.template get_index<"svc"_n>();
    auto iter_compt = complainant_by_svc.find( service_id );
    auto is_sponsor = false;
    // 空或申请结束两种情况又产生新的申诉
    if ( iter_compt == complainant_by_svc.end() ) {
        is_sponsor = true;
    } else {
        // 正在仲裁中不接受对该服务申诉
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
    const uint64_t duration = eosio::days(1).to_seconds();
    add_delay(service_id, applicant, time_point_sec(now()), duration, amount);

    // Arbitration case application
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbicaseapp_tb_by_svc = arbicaseapp_tb.template get_index<"svc"_n>();
    auto arbicaseapp_iter = arbicaseapp_tb_by_svc.find( service_id );
    auto arbi_id = arbicaseapp_tb.available_primary_key();
    /// 不为空插入
    if (arbicaseapp_iter == arbicaseapp_tb_by_svc.end() || 
        (arbicaseapp_iter != arbicaseapp_tb_by_svc.end() && arbicaseapp_iter->arbi_step == arbi_step_type::arbi_started)) {
        arbicaseapp_tb.emplace( get_self(), [&]( auto& p ) {
            p.arbitration_id = arbi_id;
            p.appeal_id = appeal_id;
            p.service_id = service_id;
            p.evidence_info = reason;
            p.arbi_step = arbi_step_type::arbi_init;
            p.required_arbitrator = 5;
            p.deadline = time_point_sec(now() + 3600);
            p.add_applicant(applicant);
        } );
    } else {
        auto arbi_iter = arbicaseapp_tb.find(arbicaseapp_iter->arbitration_id);
        check(arbi_iter != arbicaseapp_tb.end(), "Can not find such arbitration.");
        arbicaseapp_tb.modify(arbi_iter, get_self(), [&]( auto& p ) {
            p.add_applicant(applicant);
        } );
    }

    // Data provider
    auto svcprovider_tb = data_service_provisions( get_self(), get_self().value );
    auto svcprovider_tb_by_svc = svcprovider_tb.template get_index<"bysvcid"_n>();
    auto svcprovider_iter = svcprovider_tb_by_svc.find( service_id );
    check(svcprovider_iter != svcprovider_tb_by_svc.end(), "Such service has no providers.");

    // Service data providers
    bool hasProvider = false;
    for(auto iter = svcprovider_tb.begin(); iter != svcprovider_tb.end(); iter++)
    {
        if(!svcprovider_iter->stop_service) {
            hasProvider = true;
            auto notify_amount = eosio::asset(1, _bos_symbol);
            // Transfer to provider
            auto memo = "arbitration_id: " + std::to_string(arbi_id)
                + ", service_id: " + std::to_string(service_id) 
                + ", state_amount: " + amount.to_string();
            transfer(get_self(), svcprovider_iter->account, notify_amount, memo);
        }
    }
    check(hasProvider, "no provier");
    timeout_deferred(arbi_id, arbitration_timer_type::resp_appeal_timeout, eosio::hours(10).to_seconds());
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
    check(arbi_iter->deadline >= time_point_sec(now()), "update result deadline.");

    auto arbiprocess_tb = arbitration_processs( get_self(), get_self().value );
    auto arbipro_iter = arbiprocess_tb.find( process_id );
    check( arbipro_iter != arbiprocess_tb.end(), "Can not find such process.");
    arbiprocess_tb.modify( arbipro_iter, get_self(), [&]( auto& p ) {
        p.add_result(result);
    } );

    // Calculate results
    if (arbipro_iter->result_size() >= arbi_iter->required_arbitrator / 2) {
       uint128_t deferred_id = make_deferred_id(arbitration_id, arbitration_timer_type::upload_result_timeout);
       cancel_deferred(deferred_id);
       handle_upload_result(arbitrator, arbitration_id, process_id);
    }
}

void bos_oracle::handle_upload_result(name arbitrator, uint64_t arbitration_id, uint64_t process_id)
{
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbi_iter = arbicaseapp_tb.find( arbitration_id );
    check(arbi_iter != arbicaseapp_tb.end(), "Can not find such arbitration case application.");

    auto arbiprocess_tb = arbitration_processs( get_self(), get_self().value );
    auto arbipro_iter = arbiprocess_tb.find( process_id );
    check( arbipro_iter != arbiprocess_tb.end(), "Can not find such process.");

    uint64_t arbi_result = 0;
    if (arbipro_iter->total_result() >= arbi_iter->required_arbitrator / 2) {
        arbi_result = 1;
    } else {
        arbi_result = 0;
    }
    arbiprocess_tb.modify( arbipro_iter, get_self(), [&]( auto& p ) {
        p.arbitration_result = arbi_result;
    } );

    // Add result to arbitration_results
    add_arbitration_result(arbitrator, arbitration_id, arbi_result, arbipro_iter->process_id);  
    timeout_deferred(arbitration_id, arbitration_timer_type::appeal_timeout, eosio::hours(10).to_seconds());
}

void bos_oracle::resparbitrat( name arbitrator, asset amount, uint64_t arbitration_id ) {
    require_auth( arbitrator );
    transfer(arbitrator, arbitrat_account, amount, "resparbitrat deposit.");

    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbi_iter = arbicaseapp_tb.find( arbitration_id );
    check(arbi_iter != arbicaseapp_tb.end(), "Can not find such arbitration.");
    arbicaseapp_tb.modify( arbi_iter, get_self(), [&]( auto& p ) {
        p.arbi_step = arbi_step_type::arbi_responded;
        p.add_arbitrator(arbitrator);
    } );

    // Check arbitrator number requirements.
    if (arbi_iter->arbitrators.size() >= arbi_iter->required_arbitrator) {
        arbicaseapp_tb.modify( arbi_iter, get_self(), [&]( auto& p ) {
            p.arbi_step = arbi_step_type::arbi_started;
        } );

        upload_result_timeout_deferred(arbitrator, arbitration_id, 0, arbitration_timer_type::upload_result_timeout, eosio::hours(10).to_seconds());
    } else {
        auto arbi_to_chose = arbi_iter->required_arbitrator - arbi_iter->arbitrators.size();
        random_chose_arbitrator(arbitration_id, arbi_iter->service_id, arbi_to_chose);
    }
}

void bos_oracle::reappeal( name applicant, uint64_t arbitration_id, uint64_t service_id, uint64_t result, uint64_t process_id, bool is_provider, asset amount) {
  require_auth( applicant );
  
  data_services svctable(get_self(), get_self().value);
  auto svc_iter = svctable.find(service_id);
  check(svc_iter != svctable.end(), "service does not exist");
  check(svc_iter->status == service_status::service_in, "service status shoule be service_in");
  transfer(applicant, arbitrat_account, amount, "reappeal.");

  
 auto complainant_tb = complainants( get_self(), get_self().value );
    auto complainant_by_svc = complainant_tb.template get_index<"svc"_n>();
    auto iter_compt = complainant_by_svc.find( service_id );
    auto is_sponsor = false;
    // // 空或申请结束两种情况又产生新的申诉
    // if ( iter_compt == complainant_by_svc.end() ) {
    //     is_sponsor = true;
    // } else {
    //     // 正在仲裁中不接受对该服务申诉
    //     check( iter_compt->status == complainant_status::wait_for_accept, "This complainant is not available." );
    // }
    
    auto appeal_id = 0;
    complainant_tb.emplace( get_self(), [&]( auto& p ) {
        p.appeal_id = complainant_tb.available_primary_key();
        p.service_id = service_id;
        p.status = complainant_status::wait_for_accept;
        p.arbi_method = arbi_method;
        p.is_sponsor = false;
        p.is_provider=is_provider;
        p.arbitration_id = arbitration_id;
        p.applicant = applicant;
        p.appeal_time = time_point_sec(now());
        p.reason = reason;
        appeal_id = p.appeal_id;
    } );

    // add_freeze
    const uint64_t duration = eosio::days(1).to_seconds();
    add_delay(service_id, applicant, time_point_sec(now()), duration, amount);

    // Arbitration case application
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbicaseapp_tb_by_svc = arbicaseapp_tb.template get_index<"svc"_n>();
    auto arbicaseapp_iter = arbicaseapp_tb_by_svc.find( service_id );
    auto arbi_id = arbicaseapp_tb.available_primary_key();
    /// 不为空插入
    check (arbicaseapp_iter != arbicaseapp_tb_by_svc.end() ,""); 
    //     (arbicaseapp_iter != arbicaseapp_tb_by_svc.end() && arbicaseapp_iter->arbi_step == arbi_step_type::arbi_started)) {
    //     arbicaseapp_tb.emplace( get_self(), [&]( auto& p ) {
    //         p.arbitration_id = arbi_id;
    //         p.appeal_id = appeal_id;
    //         p.service_id = service_id;
    //         p.evidence_info = reason;
    //         p.arbi_step = arbi_step_type::arbi_init;
    //         p.required_arbitrator = 5;
    //         p.deadline = time_point_sec(now() + 3600);
    //         p.add_applicant(applicant);
    //     } );
    // } else 
    {
        auto arbi_iter = arbicaseapp_tb.find(arbicaseapp_iter->arbitration_id);
        check(arbi_iter != arbicaseapp_tb.end(), "Can not find such arbitration.");
        arbicaseapp_tb.modify(arbi_iter, get_self(), [&]( auto& p ) {
            // p.add_applicant(applicant);
            p.is_provider = is_provider;
             p.arbi_step = arbi_step_type::arbi_reappeal;//arbi_step_crowd
        } );
    }
// if(is_provider) {
//     // TODO
//     // find complaint process_id
//     // notify
//   } else {
//     // TODO
//     // commplain(,bool isreappeal);
//   }
if(!is_provider)
{
    // Data provider
    auto svcprovider_tb = data_service_provisions( get_self(), get_self().value );
    auto svcprovider_tb_by_svc = svcprovider_tb.template get_index<"bysvcid"_n>();
    auto svcprovider_iter = svcprovider_tb_by_svc.find( service_id );
    check(svcprovider_iter != svcprovider_tb_by_svc.end(), "Such service has no providers.");

    // Service data providers
    bool hasProvider = false;
    for(auto iter = svcprovider_tb.begin(); iter != svcprovider_tb.end(); iter++)
    {
        if(!svcprovider_iter->stop_service) {
            hasProvider = true;
            auto notify_amount = eosio::asset(1, _bos_symbol);
            // Transfer to provider
            auto memo = "arbitration_id: " + std::to_string(arbi_id)
                + ", service_id: " + std::to_string(service_id) 
                + ", state_amount: " + amount.to_string();
            transfer(get_self(), svcprovider_iter->account, notify_amount, memo);
        }
    }
    check(hasProvider, "no provier");
    }
    else
    {
        ///notifi service complain
    }
    
  timeout_deferred(arbitration_id, arbitration_timer_type::resp_appeal_timeout, eosio::hours(10).to_seconds());
}

void bos_oracle::rerespcase( name provider, uint64_t arbitration_id, uint64_t result, uint64_t process_id, bool is_provider) {
    respcase(provider, arbitration_id, result, process_id, is_provider);
}

void bos_oracle::respcase( name provider, uint64_t arbitration_id, uint64_t result, uint64_t process_id, bool is_provider) {
    require_auth( provider );
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbi_iter = arbicaseapp_tb.find( arbitration_id );
    check(arbi_iter != arbicaseapp_tb.end(), "Can not find such arbitration case application.");
    check(arbi_iter->arbi_step != arbi_step_type::arbi_started, "arbitration setp shoule not be arbi_started");

    arbicaseapp_tb.modify( arbi_iter, get_self(), [&]( auto& p ) {
        p.arbi_step = arbi_step_type::arbi_choosing_arbitrator;
    } );

    auto complainant_tb = complainants( get_self(), get_self().value );
    auto complainant_iter = complainant_tb.find( arbi_iter->appeal_id );

    auto arbiprocess_tb = arbitration_processs( get_self(), get_self().value );
    auto arbipro_iter = arbiprocess_tb.find( process_id );
    if ( arbipro_iter == arbiprocess_tb.end() ) {
        arbiprocess_tb.emplace( get_self(), [&]( auto& p ) {
            p.process_id = arbiprocess_tb.available_primary_key();
            p.arbitration_id = arbitration_id;
            p.add_responder(provider);
        } );
        random_chose_arbitrator(arbitration_id, arbi_iter->service_id, arbi_iter->required_arbitrator);
    } else {
        arbiprocess_tb.modify( arbipro_iter, get_self(), [&]( auto& p ) {
            p.arbitration_id = arbitration_id;
            p.add_responder(provider);
        } );
    }
}

void bos_oracle::random_chose_arbitrator(uint64_t arbitration_id, uint64_t service_id, uint64_t arbi_to_chose) const {
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbi_iter = arbicaseapp_tb.find( arbitration_id );

    vector<name> arbitrators = random_arbitrator(arbitration_id, arbi_to_chose);
    auto notify_amount = eosio::asset(1, _bos_symbol);
    // Transfer to the chosen arbitrators
    for (auto arbitrator : arbitrators) {
        auto memo = "arbitration_id: " + std::to_string(arbitration_id)
            + ", service_id: " + std::to_string(service_id);
        transfer(get_self(), arbitrator, notify_amount, memo);
        arbicaseapp_tb.modify( arbi_iter, get_self(), [&]( auto& p ) {
            p.arbi_step = arbi_step_type::arbi_responded;
            p.add_arbitrator(arbitrator);
        } );
    }

    timeout_deferred(arbitration_id, arbitration_timer_type::resp_arbitrate_timeout, eosio::hours(10).to_seconds());
}

vector<name> bos_oracle::random_arbitrator(uint64_t arbitration_id, uint64_t arbi_to_chose) const {
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto iter_arbicaseapp = arbicaseapp_tb.find( arbitration_id );
    check(iter_arbicaseapp != arbicaseapp_tb.end(), "Can not find such arbitration");
    auto chosen_arbitrators = iter_arbicaseapp->arbitrators;
    std::vector<name> chosen_from_arbitrators;
    std::set<name> arbitrators_set;

    auto arb_table = arbitrators( get_self(), get_self().value );
    for (auto iter = arb_table.begin(); iter != arb_table.end(); iter++)
    {
        auto chosen = std::find(chosen_arbitrators.begin(), chosen_arbitrators.end(), iter->account);
        if (chosen == chosen_arbitrators.end() && !iter->is_malicious) {
            chosen_from_arbitrators.push_back(iter->account);
        }
    }

 if(chosen_from_arbitrators.size() < arbi_to_chose)
 {
     if(crowd_size > arbi_to_chose*2)
     {  
         arib
         notif()
     }
     return ;
 }

    while (arbitrators_set.size() < arbi_to_chose) {
        auto total_arbi = chosen_from_arbitrators.size();
        auto tmp = tapos_block_prefix();
        auto arbi_id = random((void*)&tmp, sizeof(tmp));
        arbi_id %= total_arbi;
        auto arbitrator = chosen_from_arbitrators.at(arbi_id);
        if (arbitrators_set.find(arbitrator) != arbitrators_set.end()) {
            continue;
        } else {
            arbitrators_set.insert(arbitrator);
        }
    }

    std::vector<name> final_arbi(arbitrators_set.begin(), arbitrators_set.end());
    return final_arbi;
}

void bos_oracle::add_arbitration_result(name arbitrator, uint64_t arbitration_id, uint64_t result, uint64_t process_id) {
    auto arbi_result_tb = arbitration_results( get_self(), get_self().value);
    arbi_result_tb.emplace( get_self(), [&]( auto& p ) {
        p.result_id = arbi_result_tb.available_primary_key();
        p.arbitration_id = arbitration_id;
        p.result = result;
        p.process_id = process_id;
        p.arbitrator = arbitrator;
    } );
}

void bos_oracle::update_arbitration_correcction(uint64_t arbitration_id) {
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbicaseapp_iter = arbicaseapp_tb.find(arbitration_id);
    check(arbicaseapp_iter != arbicaseapp_tb.end(), "Can not find such arbitration.");
    auto arbiresults_tb = arbitration_results( get_self(), get_self().value );
    auto arbitrator_tb = arbitrators( get_self(), get_self().value );

    auto arbitrators = arbicaseapp_iter->arbitrators;
    for (auto arbitrator : arbitrators) {
        uint64_t correct = 0;
        uint64_t total = 0;
        for (auto iter = arbiresults_tb.begin(); iter != arbiresults_tb.end(); iter++) {
            if (iter->arbitrator == arbitrator && iter->arbitration_id == arbitration_id) {
                total += 1;
                if (iter->result == arbicaseapp_iter->final_result) {
                    correct += 1;
                }
            }
        }

        double rate = correct > 0 && total > 0 ? 1.0 * correct / total : 0.0f;
        auto arbitrator_iter = arbitrator_tb.find(arbitrator.value);
        bool malicious = rate < bos_oracle::default_arbitration_correct_rate;

        arbitrator_tb.modify(arbitrator_iter, get_self(), [&]( auto& p ) {
            p.correct_rate = rate;
            p.is_malicious = malicious;
        } );
    }
}

uint128_t bos_oracle::make_deferred_id(uint64_t arbitration_id, arbitration_timer_type timer_type) const
{
    return (uint128_t(arbitration_id) << 64) | timer_type;
}

void bos_oracle::timeout_deferred(uint64_t arbitration_id, arbitration_timer_type timer_type, uint64_t time_length) const
{
    transaction t;
    t.actions.emplace_back(permission_level{_self, active_permission}, _self,
                           "timertimeout"_n,
                           std::make_tuple(arbitration_id, timer_type));
    t.delay_sec = time_length;
    uint128_t deferred_id = make_deferred_id(arbitration_id, timer_type);
    cancel_deferred(deferred_id);
    t.send(deferred_id, get_self());
}

void bos_oracle::upload_result_timeout_deferred(name arbitrator, uint64_t arbitration_id, uint64_t process_id,
    arbitration_timer_type timer_type, uint64_t time_length) const
{
    transaction t;
    t.actions.emplace_back(permission_level{_self, active_permission}, _self,
                           "uploaddefer"_n,
                           std::make_tuple(arbitrator, arbitration_id, process_id, timer_type));
    t.delay_sec = time_length;
    uint128_t deferred_id = make_deferred_id(arbitration_id, timer_type);
    cancel_deferred(deferred_id);
    t.send(deferred_id, get_self());
}

void bos_oracle::uploaddefer(name arbitrator, uint64_t arbitration_id, uint64_t process_id, arbitration_timer_type timer_type)
{
    if (timer_type == arbitration_timer_type::upload_result_timeout) {
        handle_upload_result(arbitrator, arbitration_id, process_id);
    }
}

void bos_oracle::timertimeout(uint64_t arbitration_id, arbitration_timer_type timer_type)
{
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbicaseapp_iter = arbicaseapp_tb.find(arbitration_id);
    check(arbicaseapp_iter != arbicaseapp_tb.end(), "Can not find such arbitration.");

    switch(timer_type)
    {
        case arbitration_timer_type::appeal_timeout: {
            if(arbicaseapp_iter->arbi_step == arbi_step_type::arbi_init) {
                handle_arbitration(arbitration_id);
                handle_arbitration_result(arbitration_id);
            }
            break;
        }
        case arbitration_timer_type::resp_appeal_timeout: {
            if(arbicaseapp_iter->arbi_step == arbi_step_type::arbi_init) {
                handle_arbitration_result(arbitration_id);
            }
            break;
        }
        case arbitration_timer_type::resp_reappeal_timeout: {
            if(arbicaseapp_iter->arbi_step == arbi_step_type::arbi_init) {
                handle_arbitration_result(arbitration_id);
            }
            break;
        }

        case arbitration_timer_type::resp_arbitrate_timeout: {
            random_chose_arbitrator(arbitration_id, arbicaseapp_iter->service_id, arbicaseapp_iter->required_arbitrator);
            break;
        }
    }
}

void bos_oracle::handle_arbitration(uint64_t arbitration_id) {
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbicaseapp_iter = arbicaseapp_tb.find(arbitration_id);
    check(arbicaseapp_iter != arbicaseapp_tb.end(), "Can not find such arbitration.");

    // Find last process
    auto arbiprocess_tb = arbitration_processs(get_self(), get_self().value);
    auto arbiprocess_iter = arbiprocess_tb.find(arbicaseapp_iter->last_process_id);
    check(arbiprocess_iter != arbiprocess_tb.end(), "Can not find such arbitration process.");

    arbicaseapp_tb.modify(arbicaseapp_iter, get_self(), [&]( auto& p ) {
        p.arbi_step = arbi_step_type::arbi_end;
        p.final_result = arbiprocess_iter->arbitration_result;
        if(p.is_provider && p.final_result)
        {
        p.final_winer = provider;
        }
        else
        {
            p.final_winer = consumer;
        }
    } );

    // Calculate arbitration correction.
    update_arbitration_correcction(arbitration_id);
}


void bos_oracle::handle_arbitration_result(uint64_t arbitration_id) {
    // TODO
    // appeal over  give  data prvoider fine amount from his stake amount, give  complaints bonus,   
    // all  data provider stake amount minus
    
}

void bos_oracle::handle_rearbitration_result(uint64_t arbitration_id) {
    // TODO
    // appeal over  give  data prvoider fine amount from his stake amount, give  complaints bonus,   
    // all  data provider stake amount minus
    
     auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbicaseapp_iter = arbicaseapp_tb.find(arbitration_id);
    check(arbicaseapp_iter != arbicaseapp_tb.end(), "Can not find such arbitration.");

    if(arbicaseapp_iter->is_provider)
    arbicaseapp_tb.modify(arbicaseapp_iter, get_self(), [&]( auto& p ) {
        p.arbi_step = arbi_step_type::arbi_reappeal_timeout_end;
        // p.final_result = arbiprocess_iter->arbitration_result;
        p.final_winer = provider;///arbiprocess_iter->arbitration_result;
    } );


    分钱
    handle_arbitration_result
}

/**
 * @brief
 *
 * @param owner
 * @param value
 */
void bos_oracle::sub_balance(name owner, asset value,uint64_t arbitration_id,bool is_provider) {
  
  riskcontrol_accounts dapp_acnts(_self, arbitration_id);

  const auto &dapp =
      dapp_acnts.get(value.symbol.code().raw(), "no balance object found");
  check(dapp.balance.amount >= value.amount, "overdrawn balance");

  dapp_acnts.modify(dapp, same_payer, [&](auto &a) { a.balance -= value; });
}

void bos_oracle::add_balance(name owner, asset value,uint64_t arbitration_id,bool is_provider) {

  riskcontrol_accounts dapp_acnts(_self, arbitration_id);
  auto dapp = dapp_acnts.find(value.symbol.code().raw());
  if (dapp == dapp_acnts.end()) {
    dapp_acnts.emplace(_self, [&](auto &a) {  a.balance = value; });
  } else {
    dapp_acnts.modify(dapp, same_payer, [&](auto &a) { a.balance += value; });
  }
}
