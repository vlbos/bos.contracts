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
using namespace eosio;
using eosio::asset;
using eosio::name;
using eosio::time_point_sec;
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
   crowd_arbitration = 1,
   multiple_rounds = 2
};

enum arbi_step_type : uint64_t
{
   arbi_init = 1,
   arbi_responded = 2,
   arbi_started = 3,
   arbi_end = 4
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
   double correct_rate;
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
   uint64_t final_result;
   uint64_t required_arbitrator;
   uint64_t last_process_id;
   time_point_sec deadline;
   time_point_sec last_process_update_time;
   std::string evidence_info;
   std::vector<name> applicants;
   std::vector<name> arbitrators;

   uint64_t primary_key() const { return arbitration_id; }
   uint64_t by_svc() const { return service_id; }
   void add_applicant ( name applicant ) { applicants.push_back( applicant ); }
   void add_arbitrator ( name arbitrator ) { arbitrators.push_back( arbitrator ); }
};

struct [[ eosio::table, eosio::contract("bos.oracle") ]] arbitration_process
{
   uint64_t process_id;
   uint64_t arbitration_id;
   uint64_t num_id;
   std::vector<name> responders;
   asset stake_amount;
   std::vector<uint64_t> arbitrator_arbitration_results;
   std::string evidence_info;
   uint64_t arbitration_result;
   uint64_t arbitration_method;

   uint64_t primary_key() const { return process_id; }
   uint64_t by_arbi() const { return arbitration_id; }
   void add_responder ( name responder ) { responders.push_back( responder ); }
   void add_result ( uint64_t result ) { arbitrator_arbitration_results.push_back( result ); }
   uint64_t result_size () const { return arbitrator_arbitration_results.size(); }
   uint64_t total_result () const {
      uint64_t total = 0;
      for (auto& n : arbitrator_arbitration_results)
         total += n;
      return total;
   }
};

struct [[ eosio::table, eosio::contract("bos.oracle") ]] arbitration_result
{
   uint64_t result_id;
   uint64_t arbitration_id;
   uint64_t result;
   uint64_t process_id;
   name arbitrator;

   uint64_t primary_key() const { return result_id; }
   uint64_t by_arbi() const { return arbitration_id; }
};

struct [[ eosio::table, eosio::contract("bos.oracle") ]] fair_award
{
   uint64_t service_id;
   uint64_t arbitration_id;
   std::string arbitrator_evidence;

   uint64_t primary_key() const { return service_id; }
};


typedef eosio::multi_index<"complainants"_n, complainant,
   indexed_by<"svc"_n, const_mem_fun<complainant, uint64_t, &complainant::by_svc>>> complainants;
typedef eosio::multi_index<"arbitrators"_n, arbitrator> arbitrators;
typedef eosio::multi_index<"arbicaseapp"_n, arbicaseapp,
   indexed_by<"svc"_n, const_mem_fun<arbicaseapp, uint64_t, &arbicaseapp::by_svc>>> arbicaseapps;
typedef eosio::multi_index<"arbiprocess"_n, arbitration_process,
   indexed_by<"arbi"_n, const_mem_fun<arbitration_process, uint64_t, &arbitration_process::by_arbi>>> arbitration_processs;
typedef eosio::multi_index<"arbiresults"_n, arbitration_result,
   indexed_by<"arbi"_n, const_mem_fun<arbitration_result, uint64_t, &arbitration_result::by_arbi>>> arbitration_results;
typedef eosio::multi_index<"fairawards"_n, fair_award> fair_awards;

// };

// } /// namespace eosio
