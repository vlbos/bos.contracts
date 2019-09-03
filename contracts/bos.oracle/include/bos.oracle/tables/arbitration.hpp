/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include "../bos.constants.hpp"
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include <string>

// namespace eosio {
using namespace eosio;
using eosio::asset;
using eosio::name;
using eosio::public_key;
using eosio::time_point_sec;
using std::string;

struct [[eosio::table, eosio::contract("bos.oracle")]] appeal_request {
   name appeallant;
   std::string reason;
   asset amount;
   uint8_t role_type; // 1110 is_respondent is_sponsor is_provider
   time_point_sec appeal_time;

   uint64_t primary_key() const { return appeallant.value; }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] arbitrator {
   name account;
   uint8_t type;
   uint8_t status;
   uint16_t arbitration_correct_times;            ///仲裁正确次数
   uint16_t arbitration_times;                    ///仲裁次数
   uint16_t invitations;                          ///邀请次数
   uint16_t responses;                            ///接收邀请次数
   uint16_t uncommitted_arbitration_result_times; ///未提交仲裁结果次数
   std::string public_info;

   uint64_t primary_key() const { return account.value; }
   double correct_rate() const { return 0 == arbitration_times || 0 == arbitration_correct_times ? 1.0f : 1.0f * arbitration_correct_times / arbitration_times; }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] arbitration_case {
   uint64_t arbitration_id;
   uint8_t arbi_step;
   uint8_t arbitration_result;
   uint8_t last_round;
   uint8_t final_result;
   std::vector<name> arbitrators;
   std::vector<name> chosen_arbitrators;
   uint64_t primary_key() const { return arbitration_id; }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] arbitration_process {
   uint8_t round;
   uint8_t required_arbitrator;
   std::vector<name> appeallants;
   std::vector<name> respondents;
   std::vector<name> arbitrators;
   std::vector<name> invited_arbitrators;
   uint8_t role_type;
   std::vector<uint8_t> arbitrator_arbitration_results;
   uint8_t arbitration_result;
   uint8_t arbi_method; // 本轮使用的仲裁方法

   uint64_t primary_key() const { return round; }

   uint8_t total_result() const {
      uint8_t total = 0;
      for (auto& n : arbitrator_arbitration_results) {
         if (arbitration_role_type::consumer == n) {
            ++total;
         }
      }

      return total;
   }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] arbitration_evidence {
   uint64_t evidence_id;
   uint8_t round;
   name account;
   std::string evidence_info;

   uint64_t primary_key() const { return evidence_id; }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] arbitration_result {
   name arbitrator;
   uint8_t result;
   string comment;

   uint64_t primary_key() const { return arbitrator.value; }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] fair_award {
   uint64_t award_id;
   uint64_t service_id;
   uint64_t arbitration_id;
   std::string arbitrator_evidence;

   uint64_t primary_key() const { return award_id; }
};

/**
 * @brief 申诉，应诉者仲裁抵押金额  scope arbitration id
 *
 */
struct [[eosio::table, eosio::contract("bos.oracle")]] arbitration_stake_account {
   name account;
   asset balance; /// stake amount
   uint8_t role_type;
   uint64_t primary_key() const { return account.value; }
   uint64_t by_type() const { return static_cast<uint64_t>(role_type); }
};

/**
 * @brief 抵押 记录  scope arbitration_id << 3 | round
 *
 */
struct [[eosio::table, eosio::contract("bos.oracle")]] arbitration_stake_record {
   name account;
   time_point_sec stake_time;
   asset amount;

   uint64_t primary_key() const { return account.value; }
};

/**
 * @brief 仲裁收入表
 *
 */
struct [[eosio::table, eosio::contract("bos.oracle")]] arbitration_income_account {
   name account;
   asset income;
   asset claim;
   uint64_t primary_key() const { return income.symbol.code().raw(); }
};

typedef eosio::multi_index<"appealreq"_n, appeal_request> appeal_requests;
typedef eosio::multi_index<"arbitrators"_n, arbitrator> arbitrators;
typedef eosio::multi_index<"arbicase"_n, arbitration_case> arbitration_cases;
typedef eosio::multi_index<"arbiprocess"_n, arbitration_process> arbitration_processes;
typedef eosio::multi_index<"arbievidence"_n, arbitration_evidence> arbitration_evidences;
typedef eosio::multi_index<"arbiresults"_n, arbitration_result> arbitration_results;
typedef eosio::multi_index<"fairawards"_n, fair_award> fair_awards;
typedef eosio::multi_index<"arbistakeacc"_n, arbitration_stake_account, indexed_by<"type"_n, const_mem_fun<arbitration_stake_account, uint64_t, &arbitration_stake_account::by_type>>>
    arbitration_stake_accounts;

typedef eosio::multi_index<"arbistakes"_n, arbitration_stake_record> arbitration_stake_records;
typedef eosio::multi_index<"arbiincomes"_n, arbitration_income_account> arbitration_income_accounts;

// };

// } /// namespace eosio
