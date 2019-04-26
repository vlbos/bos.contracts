#pragma once
#include <eosiolib/eosio.hpp>
#include "bos.types.hpp"


// using namespace eosio;
// using std::string;

 struct [[eosio::table, eosio::contract("bos.oracle")]] data_service {
            uint64_t          service_id;
            uint8_t           fee_type;
            uint8_t           data_type;
            uint8_t           status;
            uint8_t           injection_method;
            uint64_t          acceptance;
            time_point_sec    duration;
            uint64_t          provider_limit;
            uint64_t          update_cycle;
            time_point_sec    update_start_time;
            uint64_t          appeal_freeze_period;
            uint64_t          exceeded_risk_control_freeze_period;
            uint64_t          guarantee_id;
            asset             service_price;
            asset             stake_amount;
            asset             risk_control_amount;
            asset             pause_service_stake_amount;
            bool              freeze_flag;
            bool              emergency_flag;
            std::string       data_format;
            std::string       criteria;
            std::string       declaration;

            uint64_t primary_key() const { return service_id; }
         };
         
         struct [[eosio::table, eosio::contract("bos.oracle")]] data_service_fee {
            uint64_t          id;
            uint64_t          service_id;
            asset             service_price;
            uint8_t           fee_type;

            uint64_t primary_key() const { return id; }
         };


         struct [[eosio::table, eosio::contract("bos.oracle")]] data_provider {
            name                  account;
            public_key            pubkey;
            asset                 total_stake_amount;
            asset                 unconfirmed_amount;

            uint64_t primary_key() const { return account.value; }
         };

         struct [[eosio::table, eosio::contract("bos.oracle")]] data_service_provision {
            uint64_t              provision_id;
            uint64_t              service_id;
            name                  account;
            public_key            pubkey;
            asset                 stake_amount;
            asset                 service_income;
            std::string           public_information;
            bool                  stop_service;

            uint64_t primary_key() const { return provision_id; }
            uint64_t by_account()const { return account.value; }
         };

         struct [[eosio::table, eosio::contract("bos.oracle")]] svc_provision_cancel_apply {
            uint64_t              apply_id;
            uint64_t              service_id;
            name                  provider;
            uint64_t              status;
            time_point_sec        cancel_time;
            time_point_sec        finish_time;

            uint64_t primary_key() const { return apply_id; }
         };

          struct [[eosio::table, eosio::contract("bos.oracle")]] data_service_provision_log {
            uint64_t              log_id;
            uint64_t              service_id;
            uint64_t              update_number;
            uint64_t              status;
            std::string           data_json;
            signature             provider_sig;
            uint64_t               request_id;
            time_point_sec        update_time;

            uint64_t primary_key() const { return log_id; }
         };

         typedef eosio::multi_index< "dataservices"_n, data_service > data_services;

         typedef eosio::multi_index< "servicefees"_n, data_service_fee > data_service_fees;
         typedef eosio::multi_index< "dataprovides"_n, data_provider > data_providers;
         typedef eosio::multi_index< "datasvcprovs"_n, data_service_provision,
         indexed_by<"byaccount"_n, const_mem_fun<data_service_provision, uint64_t, &data_service_provision::by_account>> data_service_provisions;
 
         typedef eosio::multi_index< "cancelappys"_n, svc_provision_cancel_apply > svc_provision_cancel_applys;

         typedef eosio::multi_index< "svcprovilogs"_n, data_service_provision_log > data_service_provision_logs;





// //   ///bos.oraclize end
// };


// } // namespace bosoracle