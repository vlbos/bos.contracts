/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/time.hpp>
#include <string>

// namespace eosio {

using eosio::asset;
using eosio::public_key;
using std::string;

enum arbitrator_status : uint8_t
{
   stoped = 0,
   serving = 1
};

enum arbitrator_type : uint8_t
{
   profession = 1,
   amateur = 2
};

enum complainant_status : uint8_t
{
   wait_for_accept = 1,
   accepted = 2,
   failed = 3,
   success = 4
};

enum arbi_method_type : uint8_t
{
   public = 1,
   multiple_rounds = 2
};

enum arbi_step_type : uint64_t
{
   arbi_init = 1,
   arbi_responded = 2,
   arbi_end = 3
};

struct [[ eosio::table, eosio::contract("bos.oracle") ]] complainant
{
   uint64_t appeal_id;
   uint64_t service_id;
   uint8_t status;
   uint8_t arbi_method;
   bool is_sponsor;
   name applicant;
   time_point_sec appeal_time;
   std::string reason;

   uint64_t primary_key() const { return appeal_id; }
   uint64_t by_svc() const { return service_id; }
};

struct [[ eosio::table, eosio::contract("bos.oracle") ]] arbitrator
{
   name account;
   public_key pubkey;
   uint8_t type;
   uint8_t status;
   uint64_t correct_rate;
   uint64_t invitations;
   uint64_t responses;
   asset stake_amount;
   std::string public_info;
   bool is_malicious;

   uint64_t primary_key() const { return account.value; }
};

struct [[ eosio::table, eosio::contract("bos.oracle") ]] arbicaseapp
{
   uint64_t arbitration_id;
   uint64_t appeal_id;
   uint64_t service_id;
   uint64_t update_number;
   uint64_t arbi_step;
   uint64_t final_results;
   std::string evidence_info;

   uint64_t primary_key() const { return arbitration_id; }
};

struct [[ eosio::table, eosio::contract("bos.oracle") ]] arbitration_process
{
   uint64_t process_id;
   uint64_t arbitration_id;
   uint64_t num_id;
   std::vector<name> applicants;
   std::vector<name> responders;
   std::vector<name> arbitrators;
   asset stake_amount;
   std::string arbitrator_arbitration_results;
   uint64_t arbitration_results;
   uint64_t arbitration_method;

   uint64_t primary_key() const { return process_id; }
};

struct [[ eosio::table, eosio::contract("bos.oracle") ]] arbitration_result
{
   uint64_t result_id;
   uint64_t arbitrator_id;
   uint64_t arbitration_id;
   uint64_t arbitrator_arbitration_result;
   uint64_t process_id;

   uint64_t primary_key() const { return result_id; }
};

struct [[ eosio::table, eosio::contract("bos.oracle") ]] fair_award
{
   uint64_t service_id;
   uint64_t arbitration_id;
   std::string arbitrator_evidence;

   uint64_t primary_key() const { return service_id; }
};

struct [[ eosio::table, eosio::contract("bos.oracle") ]] transfer_freeze
{
   uint64_t freeze_id;
   uint64_t service_id;
   time_point_sec start_time;
   time_point_sec freezing_duration;
   asset freeze_amount;
   uint64_t status;

   uint64_t primary_key() const { return freeze_id; }
};

struct [[ eosio::table, eosio::contract("bos.oracle") ]] transfer_delay
{
   uint64_t delay_id;
   uint64_t service_id;
   time_point_sec start_time;
   name account;
   time_point_sec duration;
   asset amount;
   uint64_t status;

   uint64_t primary_key() const { return delay_id; }
};

struct [[eosio::table]] risk_guarantee
{
   uint64_t risk_id;
   name account;
   asset amount;
   time_point_sec duration;
   signature sig;

   uint64_t primary_key() const { return risk_id; }
};

typedef eosio::multi_index<"complainants"_n, complainant,
                           indexed_by<"svc"_n, const_mem_fun<complainant, uint64_t, &complainant::by_svc>>>
    complainants;
typedef eosio::multi_index<"arbitrators"_n, arbitrator> arbitrators;
typedef eosio::multi_index<"arbicaseapp"_n, arbicaseapp> arbicaseapps;
typedef eosio::multi_index<"arbiprocess"_n, arbitration_process> arbitration_processs;
typedef eosio::multi_index<"arbiresults"_n, arbitration_result> arbitration_results;
typedef eosio::multi_index<"fairawards"_n, fair_award> fair_awards;

// };

// } /// namespace eosio
