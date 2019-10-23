/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */

#include "bos.oracle/bos.oracle.hpp"
#include "bos.oracle/bos.util.hpp"
#include <algorithm>
#include <cmath>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/time.hpp>
#include <eosio/transaction.hpp>
#include <set>
#include <string>
// namespace eosio {

using eosio::asset;
using eosio::public_key;
using std::string;

/**
 * @brief  Sets status for arbitration case  process
 *
 * @param arbitration_id   arbitration case id
 * @param status   process status   such as    arbi_wait_for_resp_appeal=3,   arbi_wait_for_accept_arbitrate_invitation=4,   arbi_wait_for_upload_result=5,   arbi_wait_for_reappeal=6,
 */
void bos_oracle::setstatus(uint64_t arbitration_id, uint8_t status) {
   require_auth(_self);
   uint64_t service_id = arbitration_id;
   auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
   auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
   check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such arbitration.setstatus");
   arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto& p) { p.arbi_step = status; });
}

/**
 * @brief Imports wps auditors as fulltime arbitrators
 *
 * @param auditors  list of auditors
 */
void bos_oracle::importwps(vector<name> auditors) {
   require_auth(_self);

   auto abr_table = arbitrators(get_self(), get_self().value);

   for (auto& account : auditors) {
      auto itr = abr_table.find(account.value);
      check(itr == abr_table.end(), "Arbitrator already registered");

      // 注册仲裁员, 填写信息
      abr_table.emplace(get_self(), [&](auto& p) {
         p.account = account;
         p.type = arbitrator_type::wps;
         p.public_info = "";
         p.arbitration_correct_times = 0;            ///仲裁正确次数
         p.arbitration_times = 0;                    ///仲裁次数
         p.invitations = 0;                          ///邀请次数
         p.responses = 0;                            ///接收邀请次数
         p.uncommitted_arbitration_result_times = 0; ///未提交仲裁结果次数
      });
   }
}

/**
 * 注册仲裁员
 */
// void bos_oracle::regarbitrat( name account,  uint8_t type, asset amount, std::string public_info ) {
//     require_auth( account );
//     _regarbitrat(  account,    type,  amount,  public_info );
// }

void bos_oracle::_regarbitrat(name account, uint8_t type, asset amount, std::string public_info) {
   check_data(public_info, "public_info");

   check(type == arbitrator_type::fulltime || type == arbitrator_type::crowd, "Arbitrator type can only be 1 or 2.");
   auto abr_table = arbitrators(get_self(), get_self().value);
   auto itr = abr_table.find(account.value);
   check(itr == abr_table.end(), "Arbitrator already registered");

   check_stake(amount, "register arbitrator stake amount ", unpack<oracle_parameters>(_oracle_meta_parameters.parameters_data).min_reg_arbitrator_stake_limit);

   stake_arbitration(0, account, amount, 0, 0, "");

   // 注册仲裁员, 填写信息
   abr_table.emplace(get_self(), [&](auto& p) {
      p.account = account;
      p.type = type;
      p.public_info = public_info;
      p.arbitration_correct_times = 0;            ///仲裁正确次数
      p.arbitration_times = 0;                    ///仲裁次数
      p.invitations = 0;                          ///邀请次数
      p.responses = 0;                            ///接收邀请次数
      p.uncommitted_arbitration_result_times = 0; ///未提交仲裁结果次数
      p.status = 0xff;                            /// oracel version 1.0 set unstakearbi disable for arbitrator
   });
}

void bos_oracle::_appeal(name appellant, uint64_t service_id, asset amount, std::string reason, std::string evidence, uint8_t role_type) {
   check_data(evidence, "evidence");
   check(consumer == role_type || provider == role_type, "role type only support consume(1) and provider(2)");
   print("appeal >>>>>>role_type", role_type);
   // check(arbi_method == arbi_method_type::public_arbitration ||
   //           arbi_method_type::multiple_rounds,
   //       "`arbi_method` can only be 1 or 2.");

   // 检查申诉的服务的服务状态
   data_services svctable(get_self(), get_self().value);
   auto svc_itr = svctable.find(service_id);
   check(svc_itr != svctable.end(), "service does not exist");
   check(svc_itr->status == service_status::service_in, "service status should be service_in");

   uint64_t arbitration_id = service_id;
   if (arbitration_role_type::consumer == role_type) {
      // add_freeze
      uint32_t para_duration = unpack<oracle_parameters>(_oracle_meta_parameters.parameters_data).arbi_freeze_stake_duration;
      add_freeze(service_id, appellant, bos_oracle::current_time_point_sec(), para_duration, amount, arbitration_id);
   }

   uint8_t current_round = 1;

   // Arbitration case application
   auto arbitration_case_tb = arbitration_cases(get_self(), service_id);

   auto insert_arbi_case = [&]() {
      arbitration_case_tb.emplace(get_self(), [&](auto& p) {
         p.arbitration_id = arbitration_id;
         p.arbi_step = arbi_step_type::arbi_init; // 仲裁状态为初始化状态, 等待应诉
         p.last_round = current_round;
      });
   };

   auto arbiprocess_tb = arbitration_processes(get_self(), arbitration_id);

   auto clear_arbi_process = [&](uint64_t arbitration_id) {
      std::vector<uint8_t> rounds;
      for (auto& p : arbiprocess_tb) {
         rounds.push_back(p.round);
      }

      for (auto& r : rounds) {
         auto arbiprocess_itr = arbiprocess_tb.find(r);
         if (arbiprocess_itr != arbiprocess_tb.end()) {
            arbiprocess_tb.erase(arbiprocess_itr);
         }
      }
   };

   auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);

   // 仲裁案件不存在或者存在但是状态为开始仲裁, 那么创建一个仲裁案件
   if (arbitration_case_itr == arbitration_case_tb.end()) {
      insert_arbi_case();
   } else if (arbitration_case_itr->arbi_step == arbi_reappeal_timeout_end || arbitration_case_itr->arbi_step == arbi_step_type::arbi_resp_appeal_timeout_end ||
              arbitration_case_itr->arbi_step == arbi_step_type::arbi_public_end) {
      arbitration_case_tb.erase(arbitration_case_itr);
      clear_arbi_process(arbitration_id);
      insert_arbi_case();
   } else {
      current_round = arbitration_case_itr->last_round;
   }

   arbitration_case_itr = arbitration_case_tb.find(arbitration_id);

   print("arbi_step=", arbitration_case_itr->arbi_step);
   // std::set<uint8_t> steps = {arbi_step_type::arbi_init, arbi_step_type::arbi_wait_for_reappeal, arbi_step_type::arbi_public_end};
   // auto steps_itr = steps.find(arbitration_case_itr->arbi_step);
   check(arbi_step_type::arbi_wait_for_upload_result != arbitration_case_itr->arbi_step, "should not  appeal,arbitration is processing");

   if (arbitration_case_itr->arbi_step == arbi_step_type::arbi_wait_for_reappeal) {
      current_round++;
      // 仲裁案件存在, 为此案件新增一个再申诉者
      arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto& p) { p.last_round = current_round; });
   }

   if (first_round == current_round) {
      check(arbitration_role_type::consumer == role_type, "only support consume(1) role type  in the first round");
   } else {
      check(arbitration_case_itr->arbitration_result != role_type, "role type  could not be  the winner of the previous round");
   }

   auto arbiprocess_itr = arbiprocess_tb.find(current_round);

   uint8_t arbitrator_type = arbitrator_type::fulltime;
   if (arbiprocess_itr->arbi_method != arbi_method_type::multiple_rounds) {
      arbitrator_type = arbitrator_type::crowd;
   }

   uint8_t fulltime_count = get_arbitrators_count();

   uint8_t arbi_method = arbi_method_type::multiple_rounds;
   uint8_t arbi_count = pow(2, current_round) + 1;
   if (arbiprocess_itr->arbi_method == arbi_method_type::multiple_rounds && arbi_count > fulltime_count) {
      arbi_method = arbi_method_type::public_arbitration;
      arbi_count = 2 * fulltime_count;
   }

   bool first_one = false;
   if (arbiprocess_itr == arbiprocess_tb.end()) {
      first_one = true;
      // 第一次申诉, 创建第1个仲裁过程
      arbiprocess_tb.emplace(get_self(), [&](auto& p) {
         p.round = current_round;            // 仲裁过程为第一轮
         p.required_arbitrator = arbi_count; // 每一轮需要的仲裁员的个数
         p.increment_arbitrator = 0;
         p.appellants.push_back(appellant);
         p.arbi_method = arbi_method;
         p.role_type = role_type;
      });
   } else {
      // 新增申诉者
      arbiprocess_tb.modify(arbiprocess_itr, get_self(), [&](auto& p) { p.appellants.push_back(appellant); });
   }

   // 申诉者表
   auto appeal_request_tb = appeal_requests(get_self(), make_scope_value(service_id, current_round));

   uint8_t sponsor_bit = 0;
   uint8_t provider_bit = 0;
   // 空或申请结束两种情况又产生新的申诉
   if (appeal_request_tb.begin() == appeal_request_tb.end()) {
      sponsor_bit = arbitration_role_type::sponsor;
   }
   if (arbitration_role_type::provider == role_type) {
      provider_bit = arbitration_role_type::provider;
   }

   auto appeal_request_itr = appeal_request_tb.find(appellant.value);
   check(appeal_request_itr == appeal_request_tb.end(), "the account has appealed in the round and the  service");
   // 创建申诉者
   appeal_request_tb.emplace(get_self(), [&](auto& p) {
      p.role_type = (sponsor_bit | provider_bit);
      p.appellant = appellant;
      p.appeal_time = bos_oracle::current_time_point_sec();
      p.reason = reason;
   });

   check_stake(amount, "appeal stake amount", pow(2, current_round) * unpack<oracle_parameters>(_oracle_meta_parameters.parameters_data).min_appeal_stake_limit);

   stake_arbitration(arbitration_id, appellant, amount, current_round, role_type, "");

   if (!evidence.empty()) {
      uploadeviden(appellant, arbitration_id, evidence);
   }

   if (!first_one) {
      return;
   }

   auto get_providers_by_service_id = [&](const uint64_t service_id) -> std::vector<name> {
      // Data provider
      auto svcprovider_tb = data_service_provisions(get_self(), service_id);
      check(svcprovider_tb.begin() != svcprovider_tb.end(), "Such service has no providers.");
      std::vector<name> reg_providers;
      for (auto& p : svcprovider_tb) {
         if (p.status == provision_status::provision_reg) {
            reg_providers.push_back(p.account);
         }
      }

      return reg_providers;
   };

   auto memo = "resp>arbitration_id: " + std::to_string(arbitration_id) + ", service_id: " + std::to_string(service_id) + ", stake_amount " + amount.to_string();

   if (arbitration_role_type::provider == role_type) {
      uint8_t previous_round = current_round - 1;
      check(previous_round > 0, "wrong round");
      auto arbiprocess_itr = arbiprocess_tb.find(previous_round);
      check(arbiprocess_itr != arbiprocess_tb.end(), "no round info");

      send_notify(arbiprocess_itr->appellants, memo, "no appellant in the round ");

   } else {
      // recheck service status is 'in'
      data_services svctable(get_self(), get_self().value);
      auto svc_itr = svctable.find(service_id);
      check(svc_itr != svctable.end(), "service does not exist");
      print("\n no status=", svc_itr->status);
      check(svc_itr->status == service_status::service_in, "service status is not service_in when notify");
      std::vector<name> reg_providers = get_providers_by_service_id(service_id);
      check(reg_providers.size() >= svc_itr->provider_limit, "current available providers is insufficient");
      // Service data providers

      // 对所有的数据提供者发送通知, 通知数据提供者应诉
      send_notify(reg_providers, memo);
   }

   arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto& p) {
      p.arbi_step = arbi_step_type::arbi_wait_for_resp_appeal;
      p.last_round = current_round;
   });

   uint128_t deferred_id = make_deferred_id(arbitration_id, arbitration_timer_type::reappeal_timeout);
   cancel_deferred(deferred_id);

   timeout_deferred(arbitration_id, current_round, arbitration_timer_type::resp_appeal_timeout, unpack<oracle_parameters>(_oracle_meta_parameters.parameters_data).arbi_timeout_value);
}

void bos_oracle::send_notify(const vector<name>& accounts, const std::string& memo, std::string checkmsg) {
   if (!checkmsg.empty()) {
      check(!accounts.empty(), checkmsg.c_str());
   }

   for (auto& a : accounts) {
      auto notify_amount = eosio::asset(1, core_symbol());
      transfer(get_self(), a, notify_amount, memo);
   }
}

void bos_oracle::send_notify(const vector<name>& accounts, const vector<name>& resp_accounts, const std::string& memo) {
   check(!accounts.empty(), "no appellant in the round ");
   send_notify(accounts, memo);
   check(!resp_accounts.empty(), "no respondents in the round ");
   send_notify(resp_accounts, memo);
}

void bos_oracle::_respcase(name respondent, uint64_t arbitration_id, asset amount, std::string evidence) {
   check_data(evidence, "evidence");

   uint64_t service_id = arbitration_id;
   // 检查仲裁案件状态
   auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
   auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
   check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such arbitration case . _respcase");
   check(arbitration_case_itr->arbi_step < arbi_step_type::arbi_wait_for_upload_result, "arbitration step should not be arbi_started");

   uint8_t current_round = arbitration_case_itr->last_round;

   auto arbiprocess_tb = arbitration_processes(get_self(), arbitration_id);
   auto arbiprocess_itr = arbiprocess_tb.find(current_round);
   check(arbiprocess_itr != arbiprocess_tb.end(), "no round process");

   auto responded = std::find(arbiprocess_itr->respondents.begin(), arbiprocess_itr->respondents.end(), respondent);
   check(responded == arbiprocess_itr->respondents.end(), "the respondent has responded in the current process");

   check_stake(amount, "resp case stake amount", pow(2, current_round) * unpack<oracle_parameters>(_oracle_meta_parameters.parameters_data).min_appeal_stake_limit);

   uint8_t resp_role_type = arbitration_role_type::provider;
   if (arbiprocess_itr->role_type == arbitration_role_type::provider) {
      resp_role_type = arbitration_role_type::consumer;
   }

   stake_arbitration(arbitration_id, respondent, amount, current_round, resp_role_type, "");

   bool first_one = false;
   // 新增应诉者
   arbiprocess_tb.modify(arbiprocess_itr, get_self(), [&](auto& p) {
      p.respondents.push_back(respondent);
      if (first_respondent == p.respondents.size()) {
         first_one = true;
      }
   });

   if (arbitration_case_itr->arbi_step == arbi_wait_for_resp_appeal && first_one) {
      // 修改仲裁案件状态为: 正在选择仲裁员
      arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto& p) { p.arbi_step = arbi_step_type::arbi_choosing_arbitrator; });

      // 随机选择仲裁员
      random_chose_arbitrator(arbitration_id, current_round, service_id, arbiprocess_itr->required_arbitrator);
   }

   if (!evidence.empty()) {
      uploadeviden(respondent, arbitration_id, evidence);
   }

   uint128_t deferred_id = make_deferred_id(arbitration_id, arbitration_timer_type::resp_appeal_timeout);
   cancel_deferred(deferred_id);
}

/**
 * @brief   Uploads evidence by both parties from arbitration case
 *
 * @param account   account of uploading evidence
 * @param arbitration_id   arbitration case id
 * @param evidence   evidence  ipfs hash link
 */
void bos_oracle::uploadeviden(name account, uint64_t arbitration_id, std::string evidence) {
   check_data(evidence, "evidence");

   require_auth(account);
   uint64_t service_id = arbitration_id;

   auto arbievidencetable = arbitration_evidences(get_self(), arbitration_id);

   auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
   auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
   check(arbitration_case_itr != arbitration_case_tb.end(), "no arbitration id");

   // 申诉者上传仲裁证据
   arbievidencetable.emplace(get_self(), [&](auto& e) {
      e.evidence_id = arbievidencetable.available_primary_key();
      e.round = arbitration_case_itr->last_round;
      e.account = account;
      e.evidence_info = evidence;
   });
}

/**
 * @brief Uploads result by arbitrator
 *
 * @param arbitrator  arbitrator account
 * @param arbitration_id    arbitration case id
 * @param result    arbitration result
 * @param comment  comment
 */
void bos_oracle::uploadresult(name arbitrator, uint64_t arbitration_id, uint8_t result, std::string comment) {
   check_data(comment, "comment");

   require_auth(arbitrator);
   check(result == consumer || result == provider, "`result` can only be consumer(1) or provider(2).");
   uint64_t service_id = arbitration_id;
   auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
   auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
   check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such arbitration case .uploadresult");
   check(arbitration_case_itr->arbi_step == arbi_step_type::arbi_wait_for_upload_result, "current arbitration step is not upload result");

   uint8_t round = arbitration_case_itr->last_round;
   // 仲裁员上传本轮仲裁结果
   auto arbiprocess_tb = arbitration_processes(get_self(), arbitration_id);
   auto arbiprocess_itr = arbiprocess_tb.find(round);
   check(arbiprocess_itr != arbiprocess_tb.end(), "Can not find such process.");

   auto invited = std::find(arbiprocess_itr->answer_arbitrators.begin(), arbiprocess_itr->answer_arbitrators.end(), arbitrator);
   check(invited != arbiprocess_itr->answer_arbitrators.end(), "could not find such an arbitrator in current invited arbitrators.");

   arbiprocess_tb.modify(arbiprocess_itr, get_self(), [&](auto& p) { p.arbitrator_arbitration_results.push_back(result); });

   // 计算本轮结果, 条件为上传结果的人数 >= 本轮所需要的仲裁员人数 / 2 + 1
   // 满足条件取消计算结果定时器, 直接计算上传结果
   // Add result to arbitration_results
   add_arbitration_result(arbitrator, arbitration_id, result, round, comment);

   if (arbiprocess_itr->arbitrator_arbitration_results.size() >= arbiprocess_itr->required_arbitrator) {
      uint128_t deferred_id = make_deferred_id(arbitration_id, arbitration_timer_type::upload_result_timeout);
      cancel_deferred(deferred_id);
      handle_upload_result(arbitration_id, round);
   }
}

/**
 * 处理上传结果
 */
void bos_oracle::handle_upload_result(uint64_t arbitration_id, uint8_t round) {
   // 仲裁案件检查
   uint64_t service_id = arbitration_id;
   auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
   auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
   check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such arbitration case .handle_upload_result");

   // 仲裁过程检查
   auto arbiprocess_tb = arbitration_processes(get_self(), arbitration_id);
   auto arbiprocess_itr = arbiprocess_tb.find(round);
   check(arbiprocess_itr != arbiprocess_tb.end(), "Can not find such process.");

   // 此处1表示申诉者赢, 应诉者输
   uint64_t arbi_result = arbitration_role_type::provider;
   if (arbiprocess_itr->total_result() > arbiprocess_itr->required_arbitrator / 2) {
      arbi_result = arbitration_role_type::consumer;
   }

   // 修改本轮的结果
   arbiprocess_tb.modify(arbiprocess_itr, get_self(), [&](auto& p) { p.arbitration_result = arbi_result; });

   arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto& p) { p.arbitration_result = arbi_result; });

   // Calculate arbitration correction.
   update_arbitration_correction(arbitration_id);

   // 通知仲裁结果
   // Transfer to appellant
   auto memo = "result>arbitration_id: " + std::to_string(arbitration_id) + ", arbitration_result: " + std::to_string(arbi_result);
   send_notify(arbiprocess_itr->appellants, arbiprocess_itr->respondents, memo);

   // 看是否有人再次申诉, 大众仲裁不允许再申诉    version 1.0 limits <=3
   if (arbiprocess_itr->arbi_method == arbi_method_type::multiple_rounds && round < 3) {
      arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto& p) { p.arbi_step = arbi_step_type::arbi_wait_for_reappeal; });
      timeout_deferred(arbitration_id, round, arbitration_timer_type::reappeal_timeout, unpack<oracle_parameters>(_oracle_meta_parameters.parameters_data).arbi_timeout_value);
   } else {
      arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto& p) {
         p.arbi_step = arbi_step_type::arbi_public_end;
         p.final_result = p.arbitration_result;
      });

      handle_arbitration_result(arbitration_id);

      auto memo = "final result>arbitration_id:" + std::to_string(arbitration_id) + ",final_arbitration_result:" + std::to_string(arbi_result);
      send_notify(arbiprocess_itr->appellants, arbiprocess_itr->respondents, memo);
   }
}

/**
 * @brief Accepts invitation  for arbitrating case by a arbitrator
 *
 * @param arbitrator   arbitrator account
 * @param arbitration_id    arbitration case id
 */
void bos_oracle::acceptarbi(name arbitrator, uint64_t arbitration_id) {
   require_auth(arbitrator);

   uint64_t service_id = arbitration_id;
   // 修改仲裁案件状态
   auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
   auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
   check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such arbitration.acceptarbi");

   check(arbitration_case_itr->arbi_step == arbi_step_type::arbi_wait_for_accept_arbitrate_invitation, "current arbitration step is not  arbi_wait_for_accept_arbitrate_invitation ");

   uint8_t round = arbitration_case_itr->last_round;

   // 将仲裁员添加此轮应诉的仲裁者中
   auto arbiprocess_tb = arbitration_processes(get_self(), arbitration_id);
   auto arbiprocess_itr = arbiprocess_tb.find(round);
   check(arbiprocess_itr != arbiprocess_tb.end(), "Can not find such arbitration process.");

   print(",id=", arbitration_id, "==arbitration_case_itr->chosen_arbitrators=====================", arbitration_case_itr->chosen_arbitrators.size());
   print(",round=", round, "==arbiprocess_itr->invited_arbitrators=====================", arbiprocess_itr->invited_arbitrators.size());
   auto chosen = std::find(arbitration_case_itr->chosen_arbitrators.begin(), arbitration_case_itr->chosen_arbitrators.end(), arbitrator);
   check(chosen != arbitration_case_itr->chosen_arbitrators.end(), "could not find such an arbitrator in current chosen arbitration.");

   auto accepted = std::find(arbitration_case_itr->answer_arbitrators.begin(), arbitration_case_itr->answer_arbitrators.end(), arbitrator);
   check(accepted == arbitration_case_itr->answer_arbitrators.end(), "the arbitrator has accepted invitation");

   bool public_arbi = arbiprocess_itr->arbi_method == arbi_method_type::public_arbitration; // 是否为大众仲裁

   arbiprocess_tb.modify(arbiprocess_itr, get_self(), [&](auto& p) { p.answer_arbitrators.push_back(arbitrator); });

   arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto& p) { p.answer_arbitrators.push_back(arbitrator); });

   // 如果仲裁员人数满足需求, 那么开始仲裁
   if (arbiprocess_itr->answer_arbitrators.size() >= arbiprocess_itr->required_arbitrator + arbiprocess_itr->increment_arbitrator) {

      uint128_t deferred_id = make_deferred_id(arbitration_id, arbitration_timer_type::accept_arbitrate_invitation_timeout);
      cancel_deferred(deferred_id);
      arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto& p) { p.arbi_step = arbi_step_type::arbi_wait_for_upload_result; });

      // 通知仲裁结果
      // Transfer to appellant
      auto memo = "wait upload result>arbitration_id: " + std::to_string(arbitration_id);
      send_notify(arbiprocess_itr->answer_arbitrators, memo, "no arbitrators in the round ");

      uint32_t timeout_sec = unpack<oracle_parameters>(_oracle_meta_parameters.parameters_data).arbi_timeout_value;
      // 等待仲裁员上传结果
      timeout_deferred(arbitration_id, round, arbitration_timer_type::upload_result_timeout, timeout_sec);
   }
}

/**
 * 随机选择仲裁员
 */
void bos_oracle::random_chose_arbitrator(uint64_t arbitration_id, uint8_t round, uint64_t service_id, uint64_t required_arbitrator_count) {
   vector<name> chosen_arbitrators = random_arbitrator(arbitration_id, round, required_arbitrator_count);

   auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
   auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
   check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such arbitration.random_chose_arbitrator");

   auto arbiprocess_tb = arbitration_processes(get_self(), arbitration_id);
   auto arbiprocess_itr = arbiprocess_tb.find(round);
   check(arbiprocess_itr != arbiprocess_tb.end(), "Can not find such arbitration process");

   if (chosen_arbitrators.size() == required_arbitrator_count) {
      // 保存被选择的仲裁员
      arbiprocess_tb.modify(arbiprocess_itr, get_self(), [&](auto& p) {
         // 刚好选择完毕仲裁员, 那么设置这些仲裁员需要在指定时间内应诉的时间
         p.invited_arbitrators.insert(p.invited_arbitrators.end(), chosen_arbitrators.begin(), chosen_arbitrators.end());
      });

      arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto& p) {
         p.chosen_arbitrators.insert(p.chosen_arbitrators.end(), chosen_arbitrators.begin(), chosen_arbitrators.end());
         p.arbi_step = arbi_step_type::arbi_wait_for_accept_arbitrate_invitation;
      });

      // 通知选择的仲裁员
      auto memo = "invite>arbitration_id: " + std::to_string(arbitration_id) + ", service_id: " + std::to_string(service_id);
      send_notify(chosen_arbitrators, memo);

      uint8_t timer_type = arbitration_timer_type::accept_arbitrate_invitation_timeout;
      uint32_t time_length = unpack<oracle_parameters>(_oracle_meta_parameters.parameters_data).arbi_timeout_value;

      // 等待仲裁员响应
      timeout_deferred(arbitration_id, round, timer_type, time_length);

      return;
   } else {
      print("chosen_arbitrators.size()", chosen_arbitrators.size());
   }

   /// public arbitration
   uint8_t arbitrator_type = arbitrator_type::fulltime;
   if (arbiprocess_itr->arbi_method != arbi_method_type::multiple_rounds) {
      arbitrator_type = arbitrator_type::crowd;
   }

   uint8_t fulltime_count = get_arbitrators_count();

   //不重复选择的公式是 2^(n+1)+n-2,可重复选择的公式是2^n+1
   // 专业仲裁第一轮, 人数指数倍增加, 2^1+1, 2^2+1, 2^3+1, 2^num+1,
   // num为仲裁的轮次 专业仲裁人数不够, 走大众仲裁
   // 人数不够情况有两种: 1.最开始不够; 2.随机选择过程中不够;
   // 大众仲裁人数为专业仲裁2倍, 启动过程一样, 阶段重新设置,
   // 大众仲裁结束不能再申诉, 进入大众仲裁需要重新抵押, 抵押变为2倍 增加抵押金,
   // 记录方法为大众仲裁, 仲裁个数为2倍, 增加抵押金
   if (arbiprocess_itr->arbi_method == arbi_method_type::multiple_rounds) {
      // 专业仲裁人数不够, 走大众仲裁
      arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto& p) {
         p.arbi_step = arbi_step_type::arbi_choosing_arbitrator; //大众仲裁阶段开始
      });
      // 修改本轮仲裁过程为大众仲裁
      arbiprocess_tb.modify(arbiprocess_itr, get_self(), [&](auto& p) {
         p.arbi_method = arbi_method_type::public_arbitration; //仲裁案件变为大众仲裁
         p.required_arbitrator = 2 * fulltime_count;           // 该轮需要的仲裁员的个数
      });

      // 挑选专业仲裁员过程中人数不够，进入大众仲裁，开始随机挑选大众仲裁
      random_chose_arbitrator(arbitration_id, round, arbitration_id, fulltime_count * 2);

      return;
   }

   // 大众仲裁人数不够, TODO
   print("public arbitration processing ,to continue");
}

std::vector<name> bos_oracle::get_arbitrators(uint8_t arbitrator_type) {
   std::vector<name> fulltime_arbitrators;

   uint16_t para_correct_rate = unpack<oracle_parameters>(_oracle_meta_parameters.parameters_data).arbitration_correct_rate;
   // 遍历仲裁员表, 找出可以选择的仲裁员
   auto arb_table = arbitrators(get_self(), get_self().value);
   for (auto& a : arb_table) {
      if (0 == (arbitrator_type & a.type) || !a.check_correct_rate(para_correct_rate)) {
         continue;
      }
      fulltime_arbitrators.push_back(a.account);
   }

   return fulltime_arbitrators;
}

int bos_oracle::get_arbitrators_count() { return get_arbitrators().size(); }

/**
 * 为某一个仲裁的某一轮随机选择 `required_arbitrator_count` 个仲裁员
 */
vector<name> bos_oracle::random_arbitrator(uint64_t arbitration_id, uint8_t round, uint64_t required_arbitrator_count) {
   int64_t service_id = arbitration_id;
   auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
   auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
   check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such arbitration.random_arbitrator");

   auto arbiprocess_tb = arbitration_processes(get_self(), arbitration_id);
   auto arbiprocess_itr = arbiprocess_tb.find(round);
   check(arbiprocess_itr != arbiprocess_tb.end(), "Can not find such arbitration process");

   uint8_t arbitrator_type = arbitrator_type::fulltime;
   if (arbiprocess_itr->arbi_method != arbi_method_type::multiple_rounds) {
      arbitrator_type = arbitrator_type::crowd;
   }

   std::vector<name> chosen_from_arbitrators = get_arbitrators(arbitrator_type); // 需要从哪里选择出来仲裁员的地方

   if (chosen_from_arbitrators.size() < required_arbitrator_count) {
      return std::vector<name>{};
   }

   std::vector<name> chosen_arbitrators(arbitration_case_itr->chosen_arbitrators); // 本案已经选择的仲裁员

   std::vector<name> filter_arbitrators;
   if (!chosen_arbitrators.empty()) {
      std::sort(chosen_arbitrators.begin(), chosen_arbitrators.end());
      std::sort(chosen_from_arbitrators.begin(), chosen_from_arbitrators.end());
      filter_arbitrators.resize(chosen_from_arbitrators.size(), name());
      auto itr = std::set_difference(chosen_from_arbitrators.begin(), chosen_from_arbitrators.end(), chosen_arbitrators.begin(), chosen_arbitrators.end(), filter_arbitrators.begin());
      filter_arbitrators.resize(itr - filter_arbitrators.begin());
   } else {
      filter_arbitrators = std::move(chosen_from_arbitrators);
   }

   if (filter_arbitrators.size() < required_arbitrator_count) {
      print(" if (filter_arbitrators.size() < required_arbitrator_count) ");
      return std::vector<name>{};
   }

   auto rand_generator = [&](int count) -> int {
      auto tmp = tapos_block_prefix();
      auto arbi_index = random((void*)&tmp, sizeof(tmp));
      return arbi_index %= count;
   };

   auto swap = [&](vector<name>& select_arbitrators, int i, int j) {
      name temp = select_arbitrators[i];
      select_arbitrators[i] = select_arbitrators[j];
      select_arbitrators[j] = temp;
   };

   auto rand_select = [&](vector<name>& select_arbitrators, int n) {
      for (int i = 0; i < n; i++) {
         swap(select_arbitrators, i, rand_generator(select_arbitrators.size() - i) + i);
      }
   };

   int rand_count = required_arbitrator_count;
   int begin_index = 0;
   std::vector<name> result_arbitrators;
   if (required_arbitrator_count > filter_arbitrators.size() / 2) {
      rand_count = filter_arbitrators.size() - required_arbitrator_count;
      begin_index = rand_count;
   }

   rand_select(filter_arbitrators, rand_count);
   auto begin_itr = filter_arbitrators.begin() + begin_index;
   result_arbitrators.insert(result_arbitrators.begin(), begin_itr, begin_itr + required_arbitrator_count);

   return result_arbitrators;

   // // 遍历仲裁员表, 找出可以选择的仲裁员
   // auto arb_table = arbitrators(get_self(), get_self().value);
   // int arbi_count = 0;
   // int arbi_good_count = 0;
   // for (auto itr = arb_table.begin(); itr != arb_table.end(); ++itr) {
   //    ++arbi_count;
   //    if (0 == (arbitrator_type & itr->type)) {
   //       continue;
   //    }
   //    if (!itr->check_correct_rate()) {
   //       continue;
   //    }

   //    auto chosen = std::find(chosen_arbitrators.begin(), chosen_arbitrators.end(), itr->account);
   //    if (chosen == chosen_arbitrators.end()) {
   //       chosen_from_arbitrators.push_back(itr->account);
   //    }

   //    arbi_good_count++;
   // }

   // print("arbi_count", arbi_count, "arbi_good_count", arbi_good_count);

   // 挑选 `required_arbitrator_count` 个仲裁员
   // while (arbitrators_set.size() < required_arbitrator_count) {
   //    auto total_arbi = chosen_from_arbitrators.size();
   //    auto tmp = tapos_block_prefix();
   //    auto arbi_id = random((void*)&tmp, sizeof(tmp));
   //    arbi_id %= total_arbi;
   //    auto arbitrator = chosen_from_arbitrators.at(arbi_id);
   //    if (arbitrators_set.find(arbitrator) != arbitrators_set.end()) {
   //       print("if (arbitrators_set.find(arbitrator) != arbitrators_set.end()) ");
   //       continue;
   //    }

   //    arbitrators_set.insert(arbitrator);
   //    auto chosen = std::find(chosen_from_arbitrators.begin(), chosen_from_arbitrators.end(), arbitrator);
   //    chosen_from_arbitrators.erase(chosen);
   // }

   // std::vector<name> final_arbi(arbitrators_set.begin(), arbitrators_set.end());
   // return final_arbi;
}

/**
 * 新增仲裁结果表
 */
void bos_oracle::add_arbitration_result(name arbitrator, uint64_t arbitration_id, uint8_t result, uint8_t round, std::string comment) {
   check_data(comment, "comment");
   auto arbi_result_tb = arbitration_results(get_self(), make_scope_value(arbitration_id, round));
   auto arbi_result_itr = arbi_result_tb.find(arbitrator.value);
   check(arbi_result_itr == arbi_result_tb.end(), "the arbitrator has uploaded result in the round and the arbitration case");
   arbi_result_tb.emplace(get_self(), [&](auto& p) {
      p.result = result;
      p.arbitrator = arbitrator;
      p.comment = comment;
   });
}

std::vector<name> bos_oracle::get_arbitrators_of_uploading_arbitration_result(uint64_t arbitration_id) {
   uint64_t service_id = arbitration_id;

   auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
   auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
   check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such arbitration in update_arbitration_correction");

   (void)make_scope_value(arbitration_id, arbitration_case_itr->last_round);

   std::vector<name> upload_result_arbitrators;
   for (uint8_t round = 1; round <= arbitration_case_itr->last_round; ++round) {
      auto arbiresults_tb = arbitration_results(get_self(), make_scope_value(arbitration_id, round, false));
      auto arbiresults_itr = arbiresults_tb.begin();
      while (arbiresults_itr != arbiresults_tb.end()) {
         upload_result_arbitrators.push_back(arbiresults_itr->arbitrator);
         arbiresults_tb.erase(arbiresults_itr);
         arbiresults_itr = arbiresults_tb.begin();
      }
   }

   return upload_result_arbitrators;
}

/**
 * 更新仲裁正确率
 */
void bos_oracle::update_arbitration_correction(uint64_t arbitration_id) {
   uint64_t service_id = arbitration_id;

   auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
   auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
   check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such arbitration in update_arbitration_correction");

   auto update_correction = [&](name arbitrator, bool is_correct) {
      auto arbitrator_tb = arbitrators(get_self(), get_self().value);
      auto arbitrator_itr = arbitrator_tb.find(arbitrator.value);
      check(arbitrator_itr != arbitrator_tb.end(), "Could not find such arbitrator in update_arbitration_correction");

      uint8_t correct_times = 0;
      if (is_correct) {
         correct_times++;
      }

      arbitrator_tb.modify(arbitrator_itr, get_self(), [&](auto& p) {
         p.arbitration_times++;
         p.arbitration_correct_times += correct_times;
      });
   };

   (void)make_scope_value(arbitration_id, arbitration_case_itr->last_round);

   for (uint8_t round = 1; round <= arbitration_case_itr->last_round; ++round) {
      auto arbiprocess_tb = arbitration_processes(get_self(), arbitration_id);
      auto arbiprocess_itr = arbiprocess_tb.find(round);
      check(arbiprocess_itr != arbiprocess_tb.end(), "Can not find such process calculate correction");

      auto arbiresults_tb = arbitration_results(get_self(), make_scope_value(arbitration_id, round, false));
      for (auto arbiresults_itr = arbiresults_tb.begin(); arbiresults_itr != arbiresults_tb.end(); ++arbiresults_itr) {
         update_correction(arbiresults_itr->arbitrator, arbiresults_itr->result == arbiprocess_itr->arbitration_result);
      }
   }
}

uint128_t bos_oracle::make_deferred_id(uint64_t arbitration_id, uint8_t timer_type) { return (uint128_t(arbitration_id) << 64) | timer_type; }

void bos_oracle::timeout_deferred(uint64_t arbitration_id, uint8_t round, uint8_t timer_type, uint32_t time_length) {
   transaction t;
   t.actions.emplace_back(permission_level{_self, active_permission}, _self, "timertimeout"_n, std::make_tuple(arbitration_id, round, timer_type));
   t.delay_sec = time_length;
   uint128_t deferred_id = make_deferred_id(arbitration_id, timer_type);
   cancel_deferred(deferred_id);
   print("\n>>>===timertimeout=", deferred_id, "=id=", arbitration_id, "=round=", round, "=timer_type=", timer_type, "=time_length=", time_length);
   t.send(deferred_id, get_self(), true);
}

/**
 * @brief   Starts timer for handling  timeout wher arbitration operation's waiting time is expired
 *
 * @param arbitration_id  arbitration case id
 * @param round    arbitration case  arbitrat times
 * @param timer_type  timer type such as   reappeal_timeout = 1, resp_appeal_timeout=2, accept_arbitrate_invitation_timeout=3, upload_result_timeout =4
 */
void bos_oracle::timertimeout(uint64_t arbitration_id, uint8_t round, uint8_t timer_type) {
   require_auth(_self);
   uint64_t service_id = arbitration_id;
   auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
   auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
   check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such arbitration.timertimeout");

   auto arbiprocess_tb = arbitration_processes(get_self(), arbitration_id);
   auto arbiprocess_itr = arbiprocess_tb.find(round);
   check(arbiprocess_itr != arbiprocess_tb.end(), "Can not find such process.");

   uint8_t half_count = arbiprocess_itr->required_arbitrator / 2;
   uint8_t consumer_results = arbiprocess_itr->total_result();
   uint8_t results_count = arbiprocess_itr->arbitrator_arbitration_results.size();

   print(">>>===timer_type=", timer_type);
   switch (timer_type) {
   case arbitration_timer_type::reappeal_timeout: { // 再次申诉
      if (arbitration_case_itr->arbi_step == arbi_step_type::arbi_wait_for_reappeal) {
         print(">>>===timer_type=arbitration_timer_type::reappeal_timeout");
         // 没人再次申诉, 记录最后一次仲裁过程
         arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto& p) {
            p.last_round = round;
            p.arbi_step = arbi_step_type::arbi_reappeal_timeout_end;
            p.final_result = p.arbitration_result;
            print("\narbitration_id==", arbitration_id, " \n  p.final_result = p.arbitration_result;final_result=", p.final_result);
         });
         handle_arbitration_result(arbitration_id);

         // Transfer to appellant
         auto memo = "final result>arbitration_id:" + std::to_string(arbitration_id) + ",final_arbitration_result:" + std::to_string(arbitration_case_itr->arbitration_result);
         send_notify(arbiprocess_itr->appellants, arbiprocess_itr->respondents, memo);
      }
      break;
   }
   case arbitration_timer_type::resp_appeal_timeout: {
      // 如果仲裁案件状态仍然为初始化状态, 说明没有数据提供者应诉,
      // 直接处理仲裁结果
      if (arbitration_case_itr->arbi_step == arbi_step_type::arbi_wait_for_resp_appeal) {
         print(">>>===timer_type=arbitration_timer_type::resp_appeal_timeout");

         arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto& p) {
            p.arbi_step = arbi_step_type::arbi_resp_appeal_timeout_end;
            p.final_result = arbiprocess_itr->role_type;
            print("\final_result=", p.final_result);
         });

         handle_arbitration_result(arbitration_id);

         // Transfer to appellant
         auto memo = "final result>arbitration_id:" + std::to_string(arbitration_id) + ",final_arbitration_result:" + std::to_string(arbiprocess_itr->role_type);
         send_notify(arbiprocess_itr->appellants, memo, "no appellant in the round ");
      }
      break;
   }
   case arbitration_timer_type::accept_arbitrate_invitation_timeout: {
      // 如果状态为还在选择仲裁员, 那么继续选择仲裁员
      uint8_t inc_arbitrator_count = arbiprocess_itr->required_arbitrator - arbiprocess_itr->answer_arbitrators.size();
      if (arbitration_case_itr->arbi_step == arbi_step_type::arbi_wait_for_accept_arbitrate_invitation && inc_arbitrator_count > 0) {
         print(">>>===timer_type=arbitration_timer_type::accept_arbitrate_invitation_timeout");
         random_chose_arbitrator(arbitration_id, round, arbitration_id, inc_arbitrator_count);
      }
      break;
   }
   case arbitration_timer_type::upload_result_timeout: {
      if (arbitration_case_itr->arbi_step == arbi_step_type::arbi_wait_for_upload_result) {
         bool is_provider_results = (results_count - consumer_results) > half_count;

         if (consumer_results > half_count || is_provider_results) {
            print(">>>===timer_type=arbitration_timer_type::upload_result_timeout");

            uint128_t deferred_id = make_deferred_id(arbitration_id, arbitration_timer_type::upload_result_timeout);
            cancel_deferred(deferred_id);
            handle_upload_result(arbitration_id, round);
         } else {
            print(">>>===timer_type=arbitration_timer_type::upload_result_timeout else");

            uint8_t remainder = arbiprocess_itr->required_arbitrator - results_count;
            if (remainder > 0) {
               arbiprocess_tb.modify(arbiprocess_itr, get_self(), [&](auto& p) { p.increment_arbitrator += remainder; });
               random_chose_arbitrator(arbitration_id, round, arbitration_id, remainder);
            }
         }
      }
      break;
   }
   }
}

/**
 * 处理仲裁案件最终结果
 */
void bos_oracle::handle_arbitration_result(uint64_t arbitration_id) {
   uint64_t service_id = arbitration_id;
   auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
   auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
   check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such arbitration. handle_arbitration_result");
   check(arbitration_role_type::consumer == arbitration_case_itr->final_result || arbitration_role_type::provider == arbitration_case_itr->final_result, "wrong final result");

   uint8_t loser_role_type = arbitration_role_type::provider;
   if (arbitration_case_itr->final_result == arbitration_role_type::provider) {
      loser_role_type = arbitration_role_type::consumer;
   }

   std::tuple<std::vector<name>, asset, std::vector<name>> stake_accounts = get_stake_accounts_and_asset(arbitration_id, loser_role_type);

   // slash all losers' arbitration stake
   slash_arbitration_stake(arbitration_id, loser_role_type);
   asset slash_arbitration_stake_amount = std::get<1>(stake_accounts);
   asset slash_service_stake_amount = asset(0, core_symbol());

   // if final winner is not provider then slash  all service providers' stakes
   if (arbitration_role_type::provider == loser_role_type) {
      std::tuple<std::vector<name>, asset> service_stakes = get_provider_service_stakes(service_id);
      slash_service_stake(service_id, std::get<0>(service_stakes), std::get<1>(service_stakes));
      slash_service_stake_amount = std::get<1>(service_stakes);
   }

   auto pay_arbitration = [&](std::vector<name>& award_accounts, asset dividend_amount) {
      if (award_accounts.empty()) {
         print("\n=no accounts in award_accounts =");
         return;
      }
      asset average_award_amount = dividend_amount / award_accounts.size();
      if (average_award_amount <= asset(0, core_symbol())) {
         print("\n=if (average_award_amount <= 0) =");
         return;
      }

      for (auto& a : award_accounts) {
         add_income(a, average_award_amount);
      }
   };

   // award stake accounts
   pay_arbitration(std::get<2>(stake_accounts), slash_service_stake_amount);

   std::vector<name> upload_result_arbitrators = get_arbitrators_of_uploading_arbitration_result(arbitration_id);
   // pay all arbitrators' arbitration fee
   pay_arbitration(upload_result_arbitrators, slash_arbitration_stake_amount);

   if (arbitration_case_itr->final_result == arbitration_role_type::provider) {
      unfreeze_asset(service_id, arbitration_id);
   }

   (void)make_scope_value(service_id, arbitration_case_itr->last_round);

   // clear 申诉者表
   for (int i = 1; i <= arbitration_case_itr->last_round; ++i) {
      auto appeal_request_tb = appeal_requests(get_self(), make_scope_value(service_id, i, false));
      auto appeal_request_itr = appeal_request_tb.begin();
      while (appeal_request_itr != appeal_request_tb.end()) {
         appeal_request_tb.erase(appeal_request_itr);
         appeal_request_itr = appeal_request_tb.begin();
      }
   }
}

/**
 * @brief
 *
 * @param owner
 * @param value
 */
void bos_oracle::sub_balance(name owner, asset value, uint64_t arbitration_id) {

   arbitration_stake_accounts stake_acnts(_self, arbitration_id);
   auto acc = stake_acnts.find(owner.value);
   check(acc != stake_acnts.end(), "no balance object found");
   check(acc->balance.amount >= value.amount, "overdrawn balance");

   stake_acnts.modify(acc, same_payer, [&](auto& a) { a.balance -= value; });
}

/**
 * @brief
 *
 * @param owner
 * @param value
 * @param arbitration_id
 * @param role_type
 */
void bos_oracle::add_balance(name owner, asset value, uint64_t arbitration_id, uint8_t role_type) {
   arbitration_stake_accounts stake_acnts(_self, arbitration_id);
   auto acc = stake_acnts.find(owner.value);
   if (acc == stake_acnts.end()) {
      stake_acnts.emplace(_self, [&](auto& a) {
         a.account = owner;
         a.balance = value;
         a.role_type = role_type;
      });
   } else {
      check(acc->role_type == role_type, "the same account could not be  both consume and provider role type in the arbitration");
      stake_acnts.modify(acc, same_payer, [&](auto& a) { a.balance += value; });
   }
}

/**
 * @brief
 *
 * @param arbitration_id
 * @param role_type
 * @return std::tuple<std::vector<name>,asset>
 */
std::tuple<std::vector<name>, asset, std::vector<name>> bos_oracle::get_stake_accounts_and_asset(uint64_t arbitration_id, uint8_t role_type) {
   arbitration_stake_accounts stake_acnts(_self, arbitration_id);
   std::vector<name> accounts;
   std::vector<name> opposant_accounts;
   asset stakes = asset(0, core_symbol());
   for (auto& a : stake_acnts) {
      if (a.role_type == role_type) {
         accounts.push_back(a.account);
         stakes += a.balance;
      } else {
         opposant_accounts.push_back(a.account);
      }
   }

   return std::make_tuple(accounts, stakes, opposant_accounts);
}

/**
 * @brief
 *
 * @param service_id
 * @return std::tuple<std::vector<name>,asset>
 */
std::tuple<std::vector<name>, asset> bos_oracle::get_provider_service_stakes(uint64_t service_id) {
   data_service_provisions provisionstable(_self, service_id);
   std::vector<name> providers;
   asset stakes = asset(0, core_symbol());

   for (const auto& p : provisionstable) {
      if (p.status == provision_status::provision_reg) {
         providers.push_back(p.account);
         stakes += p.amount;
      }
   }

   return std::make_tuple(providers, stakes);
}

/**
 * @brief
 *
 * @param service_id
 * @param slash_accounts
 * @param amount
 */
void bos_oracle::slash_service_stake(uint64_t service_id, const std::vector<name>& slash_accounts, const asset& amount) {

   for (auto& account : slash_accounts) {
      data_providers providertable(_self, _self.value);
      auto provider_itr = providertable.find(account.value);
      check(provider_itr != providertable.end(), "no privider in slash_service_stake");

      data_service_provisions provisionstable(_self, service_id);

      auto provision_itr = provisionstable.find(account.value);
      check(provision_itr != provisionstable.end(), "account does not subscribe services");
      check(provider_itr->total_stake_amount >= provision_itr->amount, "account does not subscribe services");
      check(provider_itr->total_freeze_amount >= provision_itr->freeze_amount, "account does not subscribe services");

      providertable.modify(provider_itr, same_payer, [&](auto& p) {
         p.total_stake_amount -= provision_itr->amount;
         p.total_freeze_amount -= provision_itr->freeze_amount;
      });

      provisionstable.modify(provision_itr, same_payer, [&](auto& p) {
         p.amount = asset(0, core_symbol());
         p.freeze_amount = asset(0, core_symbol());
      });
      update_service_provider_status(service_id, account);
   }
   data_service_stakes svcstaketable(_self, _self.value);
   auto svcstake_itr = svcstaketable.find(service_id);
   check(svcstake_itr != svcstaketable.end(), "no service in data_service_stakes of slash_service_stake ");
   check(svcstake_itr->amount >= amount, "insufficient service stake in data_service_stakes of slash_service_stake");

   svcstaketable.modify(svcstake_itr, same_payer, [&](auto& ss) { ss.amount -= amount; });
}

/**
 * @brief
 *
 * @param arbitration_id
 * @param slash_accounts
 */
void bos_oracle::slash_arbitration_stake(uint64_t arbitration_id, uint8_t role_type) {
   arbitration_stake_accounts stake_acnts(_self, arbitration_id);

   auto move_accounts = [&](name owner, asset value, uint8_t role_type) {
      arbitration_stake_accounts unstake_acnts(_self, (uint64_t(0x1) << 63) | arbitration_id);
      auto acc = unstake_acnts.find(owner.value);
      if (acc == unstake_acnts.end()) {
         unstake_acnts.emplace(_self, [&](auto& a) {
            a.account = owner;
            a.balance = value;
            a.role_type = role_type;
         });
      } else {
         unstake_acnts.modify(acc, same_payer, [&](auto& a) { a.balance += value; });
      }
   };

   auto itr = stake_acnts.begin();
   while (itr != stake_acnts.end()) {
      if (role_type != itr->role_type) {
         move_accounts(itr->account, itr->balance, itr->role_type);
      }

      stake_acnts.erase(itr);
      itr = stake_acnts.begin();
   }
}

void bos_oracle::add_income(name account, asset quantity) {
   arbitration_income_accounts incometable(_self, account.value);
   auto acc = incometable.find(quantity.symbol.code().raw());
   if (acc == incometable.end()) {
      incometable.emplace(_self, [&](auto& a) {
         a.account = account;
         a.income = quantity;
         a.claim = asset(0, quantity.symbol);
      });
   } else {
      incometable.modify(acc, same_payer, [&](auto& a) { a.income += quantity; });
   }
}

uint64_t bos_oracle::make_scope_value(uint64_t id, uint8_t round, bool is_check) {
   uint8_t para_round_limit = unpack<oracle_parameters>(_oracle_meta_parameters.parameters_data).round_limit;
   if (is_check) {
      uint8_t round_limit_value = pow(2, para_round_limit) - 1;
      std::string checkmsg = "round could not be greater than " + std::to_string(round_limit_value);
      check(round <= round_limit_value, checkmsg.c_str());
   }

   return (id << para_round_limit) | round;
}

void bos_oracle::stake_arbitration(uint64_t id, name account, asset amount, uint8_t round, uint8_t role_type, string memo) {
   check_data(memo, "memo");
   arbitration_stake_records staketable(_self, make_scope_value(id, round));

   auto stake_itr = staketable.find(account.value);
   if (stake_itr == staketable.end()) {
      staketable.emplace(_self, [&](auto& a) {
         a.account = account;
         a.amount = amount;
         a.stake_time = bos_oracle::current_time_point_sec();
      });
   } else {
      print("repeat id stake arbiration:", id, account, amount.amount, round, memo);
      staketable.modify(stake_itr, same_payer, [&](auto& a) { a.amount += amount; });
   }

   add_balance(account, amount, id, role_type);
}

void bos_oracle::check_stake_arbitration(uint64_t id, name account, uint8_t round) {
   arbitration_stake_records staketable(_self, make_scope_value(id, round));

   auto stake_itr = staketable.find(account.value);
   check(stake_itr != staketable.end(), "no stake");
}

/**
 * @brief Unstake core tokens from arbitration stake fund
 *
 * @param arbitration_id   arbitration case id
 * @param account  unstake account
 * @param amount  amount of tokens to be unstaked
 * @param memo  comment
 */
void bos_oracle::unstakearbi(uint64_t arbitration_id, name account, asset amount, std::string memo) {
   check_data(memo, "memo");

   require_auth(account);

   if (0 != arbitration_id) {
      uint64_t service_id = arbitration_id;
      auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
      auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
      check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such arbitration. unstakearbi");
      check(arbitration_case_itr->final_result != 0, "arbitration case is processing");
   } else {
      auto abr_table = arbitrators(get_self(), get_self().value);
      auto itr = abr_table.find(account.value);
      if (itr != abr_table.end()) {
         check(itr->type != arbitrator_type::wps, "wps auditor could not unstake");
         check(itr->status == 0, "arbitrator status could not be non zero");
      }
   }

   check(amount.amount > 0, "stake amount could not be  equal to zero");

   arbitration_stake_accounts stake_acnts(_self, (uint64_t(0x1) << 63) | arbitration_id);

   auto acc = stake_acnts.find(account.value);
   check(acc != stake_acnts.end(), "no account found");
   check(acc->balance.amount >= amount.amount, "overdrawn stake balance");

   stake_acnts.modify(acc, same_payer, [&](auto& a) { a.balance -= amount; });

   transfer(_self, account, amount, "");
}

/**
 * @brief  Claims income from arbitration stake fund
 *
 * @param account  claim account  name
 * @param receive_account   account name of receiving tokens
 */
void bos_oracle::claimarbi(name account, name receive_account) {
   require_auth(account);
   arbitration_income_accounts incometable(_self, account.value);

   auto income_itr = incometable.find(core_symbol().code().raw());
   check(income_itr != incometable.end(), "no income by account");

   asset new_income = income_itr->income - income_itr->claim;
   check(new_income.amount > 0, "no income ");

   incometable.modify(income_itr, same_payer, [&](auto& p) { p.claim += new_income; });

   transfer(_self, receive_account, new_income, "claim arbi");
}