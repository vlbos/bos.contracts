/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

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

enum arbitrator_status : uint8_t { stoped = 0, serving = 1 };

enum arbitrator_type : uint8_t { profession = 1, amateur = 2 };

enum complainant_status : uint8_t {
  wait_for_accept = 1,
  accepted = 2,
  failed = 3,
  success = 4
};

enum arbi_method_type : uint8_t { multiple_rounds = 1, public_arbitration = 2 };

enum arbi_step_type : uint64_t {
  arbi_init = 1,
  arbi_responded,
  arbi_choosing_arbitrator,
  arbi_started,
  arbi_end,
  arbi_timeout,
  arbi_reappeal,
  arbi_reappeal_timeout_end,
  arbi_public_init,
  arbi_public_responded,
  arbi_public_choosing_arbitrator,
  arbi_public_started,
  arbi_public_end,
  arbi_public_timeout
};

enum final_winner_type : uint64_t {
  consumer,
  provider

};

struct [[eosio::table, eosio::contract("bos.oracle")]] complainant {
  uint64_t appeal_id;
  uint64_t service_id;
  uint8_t status;
  bool is_sponsor;
  bool is_provider;        // 申诉者是否为数据提供者
  uint64_t arbitration_id; // 如果为再申诉, 需要记录此申诉ID
  name applicant;
  name complaint_object; ///投诉对象
  bool is_respondent;    //应诉者
  time_point_sec appeal_time;
  std::string reason;
  asset amount;
  uint64_t process_id;

  uint64_t primary_key() const { return appeal_id; }
  uint64_t by_svc() const { return service_id; }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] arbitrator {
  name account;
  public_key pubkey;
  uint8_t type;
  uint8_t status;
  double correct_rate;
  uint64_t invitations;
  uint64_t responses;
  std::string public_info;
  bool is_malicious;

  uint64_t primary_key() const { return account.value; }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] arbicaseapp {
  uint64_t arbitration_id;
  uint64_t appeal_id;
  uint64_t service_id;
  uint64_t update_number;
  uint64_t arbi_step;
  uint64_t final_result;
  uint64_t required_arbitrator;
  uint64_t last_process_id;
  uint64_t final_winner;
  uint8_t arbi_method;
  bool is_provider;
  time_point_sec deadline;
  time_point_sec last_process_update_time;
  std::string evidence_info;
  std::vector<name> applicants;
  std::vector<name> arbitrators;
  std::vector<name> respondents;

  uint64_t primary_key() const { return arbitration_id; }
  uint64_t by_svc() const { return service_id; }
  void add_applicant(name applicant) { applicants.push_back(applicant); }
  void add_arbitrator(name arbitrator) { arbitrators.push_back(arbitrator); }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] arbitration_process {
  uint64_t process_id;
  uint64_t appeal_id;
  uint64_t arbitration_id;
  uint64_t num_id;
  uint64_t required_arbitrator; // 每一轮需要的仲裁员的个数: 2^num_id+1
  std::vector<name> respondents;        // 数据提供者应诉者
  std::vector<name> arbitrators;        // 每一轮响应的仲裁员
  std::vector<name> random_arbitrators; // 每一轮随机选择的仲裁员
  time_point_sec arbiresp_deadline;     // 仲裁员应诉的截止时间

  std::vector<uint64_t> arbitrator_arbitration_results;
  std::string evidence_info;
  uint64_t arbitration_result;
  uint64_t arbi_method; // 本轮使用的仲裁方法

  uint64_t primary_key() const { return process_id; }
  uint64_t by_arbi() const { return arbitration_id; }
  void add_respondent(name respondent) { respondents.push_back(respondent); }
  void add_arbitrator(name arbitrator) { arbitrators.push_back(arbitrator); }
  void add_random_arbitrator(name arbitrator) {
    random_arbitrators.push_back(arbitrator);
  }
  void add_result(uint64_t result) {
    arbitrator_arbitration_results.push_back(result);
  }
  uint64_t result_size() const { return arbitrator_arbitration_results.size(); }
  uint64_t total_result() const {
    uint64_t total = 0;
    for (auto &n : arbitrator_arbitration_results)
      total += n;
    return total;
  }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] arbitration_result {
  uint64_t result_id;
  uint64_t arbitration_id;
  uint64_t result;
  uint64_t process_id;
  name arbitrator;

  uint64_t primary_key() const { return result_id; }
  uint64_t by_arbi() const { return arbitration_id; }
};

struct [[eosio::table, eosio::contract("bos.oracle")]] fair_award {
  uint64_t service_id;
  uint64_t arbitration_id;
  std::string arbitrator_evidence;

  uint64_t primary_key() const { return service_id; }
};

/**
 * @brief 申诉，应诉者仲裁抵押金额  scope arbitration id
 * 
 */
struct [[eosio::table, eosio::contract("bos.oracle")]] arbitration_stake_account {
  name account;
  asset balance; /// stake amount
  bool is_provider;
  uint64_t primary_key() const { return account.value; }
  uint64_t by_type() const { return (is_provider ? 0 : 1); }
};

/**
 * @brief 抵押 记录
 * 
 */
struct [[eosio::table, eosio::contract("bos.oracle")]] stake_record {
  uint64_t record_id;
  name account;
  time_point_sec stake_time;
  asset amount;

  uint64_t primary_key() const { return record_id; }
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

typedef eosio::multi_index<"complainants"_n, complainant, indexed_by<"svc"_n, const_mem_fun<complainant, uint64_t, &complainant::by_svc>>> complainants;
typedef eosio::multi_index<"arbitrators"_n, arbitrator> arbitrators;
typedef eosio::multi_index<"arbicaseapp"_n, arbicaseapp, indexed_by<"svc"_n, const_mem_fun<arbicaseapp, uint64_t, &arbicaseapp::by_svc>>> arbicaseapps;
typedef eosio::multi_index<"arbiprocess"_n, arbitration_process,indexed_by<"arbi"_n, const_mem_fun<arbitration_process, uint64_t,
                                       &arbitration_process::by_arbi>>> arbitration_processs;

typedef eosio::multi_index<"arbiresults"_n, arbitration_result,indexed_by<"arbi"_n, const_mem_fun<arbitration_result, uint64_t,&arbitration_result::by_arbi>>>
    arbitration_results;

typedef eosio::multi_index<"fairawards"_n, fair_award> fair_awards;

typedef eosio::multi_index<"arbistakeacc"_n, arbitration_stake_account, indexed_by<"type"_n, const_mem_fun<arbitration_stake_account, uint64_t,
                                       &arbitration_stake_account::by_type>>>
    arbitration_stake_accounts;

typedef eosio::multi_index<"arbistakes"_n, stake_record> stake_records;
typedef eosio::multi_index<"arbiincomes"_n, arbitration_income_account>
    arbitration_income_accounts;

// };

// } /// namespace eosio
