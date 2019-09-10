/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */

#include "bos.oracle/bos.oracle.hpp"
#include "bos.oracle/bos.util.hpp"
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

void bos_oracle::setstatus(uint64_t arbitration_id, uint8_t status) {
   require_auth(_self);
   uint64_t service_id = arbitration_id;
   auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
   auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
   check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such arbitration.setstatus");
   arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto& p) { p.arbi_step = status; });
}

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
   check(type == arbitrator_type::fulltime || type == arbitrator_type::crowd, "Arbitrator type can only be 1 or 2.");
   auto abr_table = arbitrators(get_self(), get_self().value);
   auto itr = abr_table.find(account.value);
   check(itr == abr_table.end(), "Arbitrator already registered");
   // TODO
   check(amount.amount >= uint64_t(10000) * pow(10, core_symbol().precision()), "stake amount could not be less than 10000");
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
   });
}

/**
 * 申诉者申诉
 */
// void bos_oracle::appeal( name appeallant, uint64_t service_id, asset amount,std::string reason, uint8_t arbi_method , std::string evidence) {
//     require_auth( appeallant );
//     _appeal(  appeallant,  service_id,  amount,  reason,  arbi_method,evidence) ;
// }

void bos_oracle::_appeal(name appeallant, uint64_t service_id, asset amount, std::string reason, std::string evidence, uint8_t role_type) {
   check(consumer == role_type || provider == role_type, "role type only support consume(1) and provider(2)");

   print("appeal >>>>>>role_type", role_type);
   // check(arbi_method == arbi_method_type::public_arbitration ||
   //           arbi_method_type::multiple_rounds,
   //       "`arbi_method` can only be 1 or 2.");

   // 检查申诉的服务的服务状态
   data_services svctable(get_self(), get_self().value);
   auto svc_itr = svctable.find(service_id);
   check(svc_itr != svctable.end(), "service does not exist");
   check(svc_itr->status == service_status::service_in, "service status shoule be service_in");

   uint64_t arbitration_id = service_id;

   if (arbitration_role_type::consumer == role_type) {
      const uint8_t arbi_freeze_stake_duration = 1; // days
      // add_freeze
      const uint32_t duration = eosio::days(arbi_freeze_stake_duration).to_seconds();
      add_freeze(service_id, appeallant, bos_oracle::current_time_point_sec(), duration, amount, arbitration_id);
   }

   const uint8_t arbi_case_deadline = 1; // hours
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

   const uint8_t first_round = 1;
   if (first_round == current_round) {
      check(arbitration_role_type::consumer == role_type, "only support consume(1) role type  in the first round");
   } else {
      check(arbitration_case_itr->arbitration_result != role_type, "role type  could not be  the winner of the previous round");
   }

   auto arbiprocess_itr = arbiprocess_tb.find(current_round);

   uint8_t arbitrator_type = arbitrator_type::fulltime;
   if (arbiprocess_itr->arbi_method != arbi_method_type::multiple_rounds) {
      print("arbitrator_type::crowd;_appeal");
      arbitrator_type = arbitrator_type::crowd;
   }

   uint8_t fulltime_count = 0;
   // 遍历仲裁员表, 找出可以选择的仲裁员
   auto arb_table = arbitrators(get_self(), get_self().value);
   for (auto itr = arb_table.begin(); itr != arb_table.end() && 0 != (arbitrator_type & itr->type) && itr->check_correct_rate(); ++itr) {
      ++fulltime_count;
   }

   uint8_t arbi_method = arbi_method_type::multiple_rounds;
   uint8_t arbi_count = pow(2, current_round + 1) + current_round - 2;
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
         p.appeallants.push_back(appeallant);
         p.arbi_method = arbi_method;
         p.role_type = role_type;
      });
   } else {
      // 新增申诉者
      arbiprocess_tb.modify(arbiprocess_itr, get_self(), [&](auto& p) { p.appeallants.push_back(appeallant); });
   }

   // 申诉者表
   auto appeal_request_tb = appeal_requests(get_self(), (service_id << 2) | (0x03 & current_round));

   uint8_t sponsor_bit = 0;
   uint8_t provider_bit = 0;
   // 空或申请结束两种情况又产生新的申诉
   if (appeal_request_tb.begin() != appeal_request_tb.end()) {
      sponsor_bit = arbitration_role_type::sponsor;
   }
   if (arbitration_role_type::provider == role_type) {
      provider_bit = arbitration_role_type::provider;
   }

   auto appeal_request_itr = appeal_request_tb.find(appeallant.value);
   check(appeal_request_itr == appeal_request_tb.end(), "the account has appealed in the round and the  service");
   // 创建申诉者
   appeal_request_tb.emplace(get_self(), [&](auto& p) {
      p.role_type = (sponsor_bit | provider_bit);
      p.appeallant = appeallant;
      p.appeal_time = bos_oracle::current_time_point_sec();
      p.reason = reason;
   });

   uint64_t stake_amount_limit = pow(2, current_round) * uint64_t(100);
   uint64_t stake_amount_limit_asset_value = stake_amount_limit * pow(10, core_symbol().precision());
   std::string checkmsg = "appeal stake amount could not be less than " + std::to_string(stake_amount_limit);
   check(amount.amount >= stake_amount_limit_asset_value, checkmsg.c_str());
   print("appeal 2 >>>>>>role_type", role_type);
   stake_arbitration(arbitration_id, appeallant, amount, current_round, role_type, "");

   if (first_one) {
      if (arbitration_role_type::provider == role_type) {
         uint8_t previous_round = current_round - 1;
         check(previous_round > 0, "wrong round");
         auto arbiprocess_itr = arbiprocess_tb.find(previous_round);
         check(arbiprocess_itr != arbiprocess_tb.end(), "no round info");

         check(arbiprocess_itr->appeallants.size() > 0, "no appeallant in the round ");

         for (auto& a : arbiprocess_itr->appeallants) {
            auto notify_amount = eosio::asset(1, core_symbol());
            // Transfer to appeallant
            auto memo = "resp appeallant >arbitration_id: " + std::to_string(arbitration_id) + ", service_id: " + std::to_string(service_id) + ", stake_amount " + amount.to_string();
            transfer(get_self(), a, notify_amount, memo);
         }

      } else {
         // Data provider
         auto svcprovider_tb = data_service_provisions(get_self(), service_id);
         check(svcprovider_tb.begin() != svcprovider_tb.end(), "Such service has no providers.");

         // Service data providers
         bool hasProvider = false;
         // 对所有的数据提供者发送通知, 通知数据提供者应诉
         for (auto itr = svcprovider_tb.begin(); itr != svcprovider_tb.end(); ++itr) {
            if (itr->status == provision_status::provision_reg) {
               hasProvider = true;
               auto notify_amount = eosio::asset(1, core_symbol());
               // Transfer to provider
               auto memo = "resp>arbitration_id: " + std::to_string(arbitration_id) + ", service_id: " + std::to_string(service_id) + ", stake_amount " + amount.to_string();
               transfer(get_self(), itr->account, notify_amount, memo);
            }
         }

         check(hasProvider, "no provider");
      }

      arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto& p) {
         p.arbi_step = arbi_step_type::arbi_wait_for_resp_appeal;
         p.last_round = current_round;
      });
      const uint8_t arbi_resp_appeal_timeout_value = 1; // hours
      timeout_deferred(arbitration_id, current_round, arbitration_timer_type::resp_appeal_timeout, eosio::hours(arbi_resp_appeal_timeout_value).to_seconds());
   }

   if (!evidence.empty()) {
      uploadeviden(appeallant, arbitration_id, evidence);
   }
}

/**
 * (数据提供者/数据使用者)应诉
 */
// void bos_oracle::respcase( name respondent, uint64_t arbitration_id, asset
// amount,uint8_t round, std::string evidence) {
//     require_auth( respondent );
//     _respcase(  respondent,  arbitration_id,  amount, round,evidence);
// }

void bos_oracle::_respcase(name respondent, uint64_t arbitration_id, asset amount, std::string evidence) {
   uint64_t service_id = arbitration_id;
   // 检查仲裁案件状态
   auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
   auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
   check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such arbitration case . _respcase");
   check(arbitration_case_itr->arbi_step != arbi_step_type::arbi_wait_for_upload_result, "arbitration setp shoule not be arbi_started");

   uint8_t current_round = arbitration_case_itr->last_round;

   auto arbiprocess_tb = arbitration_processes(get_self(), arbitration_id);
   auto arbiprocess_itr = arbiprocess_tb.find(current_round);
   check(arbiprocess_itr != arbiprocess_tb.end(), "no round process");

   auto responded = std::find(arbiprocess_itr->respondents.begin(), arbiprocess_itr->respondents.end(), respondent);
   check(responded == arbiprocess_itr->respondents.end(), "the respondent has responded in the current process");

   uint64_t stake_amount_limit = pow(2, current_round) * uint64_t(100) * pow(10, core_symbol().precision());
   std::string checkmsg = "resp case stake amount could not be less than " + std::to_string(stake_amount_limit);
   check(amount.amount >= stake_amount_limit, checkmsg.c_str());

   uint8_t resp_role_type = arbitration_role_type::provider;
   if (arbiprocess_itr->role_type == arbitration_role_type::provider) {
      resp_role_type = arbitration_role_type::consumer;
   }

   stake_arbitration(arbitration_id, respondent, amount, current_round, resp_role_type, "");

   bool first_one = false;
   const uint8_t first_respondent = 1;
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
}

void bos_oracle::uploadeviden(name account, uint64_t arbitration_id, std::string evidence) {
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
 * 仲裁员上传仲裁结果
 */
void bos_oracle::uploadresult(name arbitrator, uint64_t arbitration_id, uint8_t result, std::string comment) {
   require_auth(arbitrator);
   check(result == consumer || result == provider, "`result` can only be consumer(1) or provider(2).");
   uint64_t service_id = arbitration_id;
   auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
   auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
   check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such arbitration case .uploadresult");
   check(arbitration_case_itr->arbi_step == arbi_step_type::arbi_wait_for_upload_result, "arbitration step shoule  be upload result");

   uint8_t round = arbitration_case_itr->last_round;
   // 仲裁员上传本轮仲裁结果
   auto arbiprocess_tb = arbitration_processes(get_self(), arbitration_id);
   auto arbiprocess_itr = arbiprocess_tb.find(round);
   check(arbiprocess_itr != arbiprocess_tb.end(), "Can not find such process.");

   auto invited = std::find(arbiprocess_itr->arbitrators.begin(), arbiprocess_itr->arbitrators.end(), arbitrator);
   check(invited != arbiprocess_itr->arbitrators.end(), "could not find such an arbitrator in current invited arbitrators.");

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
   auto notify_amount = eosio::asset(1, core_symbol());
   // Transfer to appeallant
   auto memo = "result>arbitration_id: " + std::to_string(arbitration_id) + ", arbitration_result: " + std::to_string(arbi_result);
   check(arbiprocess_itr->appeallants.size() > 0, "no appeallant in the round ");

   for (auto& a : arbiprocess_itr->appeallants) {
      transfer(get_self(), a, notify_amount, memo);
   }

   check(arbiprocess_itr->respondents.size() > 0, "no respondents in the round ");

   for (auto& r : arbiprocess_itr->respondents) {
      transfer(get_self(), r, notify_amount, memo);
   }

   // 看是否有人再次申诉, 大众仲裁不允许再申诉    version 1.0 limits <=3
   if (arbiprocess_itr->arbi_method == arbi_method_type::multiple_rounds && round < 3) {
      arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto& p) { p.arbi_step = arbi_step_type::arbi_wait_for_reappeal; });
      const uint8_t arbi_reappeal_timeout_value = 1; /// hours
      timeout_deferred(arbitration_id, round, arbitration_timer_type::reappeal_timeout, eosio::hours(arbi_reappeal_timeout_value).to_seconds());
   } else {
      arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto& p) {
         p.arbi_step = arbi_step_type::arbi_public_end;
         p.final_result = p.arbitration_result;
         print("\n448", p.final_result);
      });

      handle_arbitration_result(arbitration_id);

      // 通知仲裁结果
      auto notify_amount = eosio::asset(1, core_symbol());
      // Transfer to appeallant
      auto memo = "final result>arbitration_id:" + std::to_string(arbitration_id) + ",final_arbitration_result:" + std::to_string(arbi_result);
      check(arbiprocess_itr->appeallants.size() > 0, "no appeallant in the round ");

      for (auto& a : arbiprocess_itr->appeallants) {
         transfer(get_self(), a, notify_amount, memo);
      }

      check(arbiprocess_itr->respondents.size() > 0, "no respondents in the round ");

      for (auto& r : arbiprocess_itr->respondents) {
         transfer(get_self(), r, notify_amount, memo);
      }
   }
}

/**
 * 仲裁员应邀
 */
void bos_oracle::acceptarbi(name arbitrator, uint64_t arbitration_id) {
   require_auth(arbitrator);

   uint64_t service_id = arbitration_id;
   // 修改仲裁案件状态
   auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
   auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
   check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such arbitration.acceptarbi");

   check(arbitration_case_itr->arbi_step == arbi_step_type::arbi_wait_for_accept_arbitrate_invitation, "arbitration step shoule  be  arbi_wait_for_accept_arbitrate_invitation ");

   uint8_t round = arbitration_case_itr->last_round;

   // 将仲裁员添加此轮应诉的仲裁者中
   auto arbiprocess_tb = arbitration_processes(get_self(), arbitration_id);
   auto arbiprocess_itr = arbiprocess_tb.find(round);
   check(arbiprocess_itr != arbiprocess_tb.end(), "Can not find such arbitration process.");

   print(",id=", arbitration_id, "==arbitration_case_itr->chosen_arbitrators=====================", arbitration_case_itr->chosen_arbitrators.size());
   print(",round=", round, "==arbiprocess_itr->invited_arbitrators=====================", arbiprocess_itr->invited_arbitrators.size());
   auto chosen = std::find(arbitration_case_itr->chosen_arbitrators.begin(), arbitration_case_itr->chosen_arbitrators.end(), arbitrator);
   check(chosen != arbitration_case_itr->chosen_arbitrators.end(), "could not find such an arbitrator in current chosen arbitration.");

   auto accepted = std::find(arbitration_case_itr->arbitrators.begin(), arbitration_case_itr->arbitrators.end(), arbitrator);
   check(accepted == arbitration_case_itr->arbitrators.end(), "the arbitrator has accepted invitation");

   bool public_arbi = arbiprocess_itr->arbi_method == arbi_method_type::public_arbitration; // 是否为大众仲裁

   arbiprocess_tb.modify(arbiprocess_itr, get_self(), [&](auto& p) { p.arbitrators.push_back(arbitrator); });

   arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto& p) { p.arbitrators.push_back(arbitrator); });

   // 如果仲裁员人数满足需求, 那么开始仲裁
   if (arbiprocess_itr->arbitrators.size() >= arbiprocess_itr->required_arbitrator + arbiprocess_itr->increment_arbitrator) {
      arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto& p) { p.arbi_step = arbi_step_type::arbi_wait_for_upload_result; });

      // 通知仲裁结果
      auto notify_amount = eosio::asset(1, core_symbol());
      // Transfer to appeallant
      auto memo = "wait upload result>arbitration_id: " + std::to_string(arbitration_id);
      check(arbiprocess_itr->arbitrators.size() > 0, "no arbitrators in the round ");

      for (auto& a : arbiprocess_itr->arbitrators) {
         transfer(get_self(), a, notify_amount, memo);
      }

      const uint8_t arbi_upload_result_timeout_value = 1; // hours
      uint32_t timeout_sec = eosio::hours(arbi_upload_result_timeout_value).to_seconds();
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
         for (auto& a : p.invited_arbitrators) {
            print("\nround==", round, "=ch arbi=", a.to_string());
         }
      });

      arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto& p) {
         p.chosen_arbitrators.insert(p.chosen_arbitrators.end(), chosen_arbitrators.begin(), chosen_arbitrators.end());
         p.arbi_step = arbi_step_type::arbi_wait_for_accept_arbitrate_invitation;
      });

      print(arbitration_id, "==arbitration_case_itr->chosen_arbitrators=====================", arbitration_case_itr->chosen_arbitrators.size());

      auto notify_amount = eosio::asset(1, core_symbol());

      // 通知选择的仲裁员
      for (auto& arbitrator : chosen_arbitrators) {
         auto memo = "invite>arbitration_id: " + std::to_string(arbitration_id) + ", service_id: " + std::to_string(service_id);
         transfer(get_self(), arbitrator, notify_amount, memo);
      }

      const uint8_t arbi_resp_arbitrate_timeout_value = 1; // hours
      uint8_t timer_type = arbitration_timer_type::accept_arbitrate_invitation_timeout;
      uint32_t time_length = eosio::hours(arbi_resp_arbitrate_timeout_value).to_seconds();
      // 等待仲裁员响应
      timeout_deferred(arbitration_id, round, timer_type, time_length);

      return;
   } else {
      print("chosen_arbitrators.size()", chosen_arbitrators.size());
   }

   /// public arbitration
   uint8_t arbitrator_type = arbitrator_type::fulltime;
   if (arbiprocess_itr->arbi_method != arbi_method_type::multiple_rounds) {
      print("arbitrator_type::crowd;random_chose_arbitrator");
      arbitrator_type = arbitrator_type::crowd;
   }

   uint8_t fulltime_count = 0;
   // 遍历仲裁员表, 找出可以选择的仲裁员
   auto arb_table = arbitrators(get_self(), get_self().value);
   for (auto itr = arb_table.begin(); itr != arb_table.end() && 0 != (arbitrator_type & itr->type) && itr->check_correct_rate(); ++itr) {
      ++fulltime_count;
   }

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

   auto chosen_arbitrators = arbitration_case_itr->chosen_arbitrators; // 本案已经选择的仲裁员
   std::vector<name> chosen_from_arbitrators;                          // 需要从哪里选择出来仲裁员的地方
   std::set<name> arbitrators_set;

   uint8_t arbitrator_type = arbitrator_type::fulltime;
   if (arbiprocess_itr->arbi_method != arbi_method_type::multiple_rounds) {
      print("arbitrator_type::crowd;random_arbitrator");
      arbitrator_type = arbitrator_type::crowd;
   }

   // 遍历仲裁员表, 找出可以选择的仲裁员
   auto arb_table = arbitrators(get_self(), get_self().value);
   int arbi_count = 0;
   int arbi_good_count = 0;
   for (auto itr = arb_table.begin(); itr != arb_table.end(); ++itr) {
      ++arbi_count;
      if (0 == (arbitrator_type & itr->type)) {
         print("\nif(0 == (arbitrator_type=", arbitrator_type, "&itr->type))=", itr->type);
         continue;
      }
      if (!itr->check_correct_rate()) {
         print("\nif(!itr->check_correct_rate()),", "return (arbitration_correct_times + 1) * arbitration_correct_rate_base >= (arbitration_times + 1) * arbitration_correct_rate_base * ",
               "default_arbitration_correct_rate / percent_100;,arbitration_correct_times = ", itr->arbitration_correct_times, " arbitration_times ", itr->arbitration_times,
               " = (itr->arbitration_correct_times + 1)* arbitration_correct_rate_base =", (itr->arbitration_correct_times + 1) * arbitration_correct_rate_base,
               "=(itr->arbitration_times + 1) * arbitration_correct_rate_base * default_arbitration_correct_rate / percent_100=",
               (itr->arbitration_times + 1) * arbitration_correct_rate_base * default_arbitration_correct_rate / percent_100);
         continue;
      }

      auto chosen = std::find(chosen_arbitrators.begin(), chosen_arbitrators.end(), itr->account);
      if (chosen == chosen_arbitrators.end()) {
         chosen_from_arbitrators.push_back(itr->account);
      }

      arbi_good_count++;
   }

   print("arbi_count", arbi_count, "arbi_good_count", arbi_good_count);

   if (chosen_from_arbitrators.size() < required_arbitrator_count) {
      print(" if (chosen_from_arbitrators.size() < required_arbitrator_count) ");
      return std::vector<name>{};
   } else {
      print("\n=chosen_from_arbitrators.size()=", chosen_from_arbitrators.size());
   }

   // 挑选 `required_arbitrator_count` 个仲裁员
   while (arbitrators_set.size() < required_arbitrator_count) {
      auto total_arbi = chosen_from_arbitrators.size();
      auto tmp = tapos_block_prefix();
      auto arbi_id = random((void*)&tmp, sizeof(tmp));
      arbi_id %= total_arbi;
      auto arbitrator = chosen_from_arbitrators.at(arbi_id);
      if (arbitrators_set.find(arbitrator) != arbitrators_set.end()) {
         print("if (arbitrators_set.find(arbitrator) != arbitrators_set.end()) ");
         continue;
      }

      arbitrators_set.insert(arbitrator);
      auto chosen = std::find(chosen_from_arbitrators.begin(), chosen_from_arbitrators.end(), arbitrator);
      chosen_from_arbitrators.erase(chosen);
   }

   std::vector<name> final_arbi(arbitrators_set.begin(), arbitrators_set.end());
   return final_arbi;
}

/**
 * 新增仲裁结果表
 */
void bos_oracle::add_arbitration_result(name arbitrator, uint64_t arbitration_id, uint8_t result, uint8_t round, std::string comment) {
   auto arbi_result_tb = arbitration_results(get_self(), ((arbitration_id << 2) | (0x03 & round)));
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

   std::vector<name> arbitrators;
   for (uint8_t round = 1; round <= arbitration_case_itr->last_round; ++round) {
      auto arbiresults_tb = arbitration_results(get_self(), ((arbitration_id << 2) | (0x03 & round)));
      auto arbiresults_itr = arbiresults_tb.begin();
      while (arbiresults_itr != arbiresults_tb.end()) {
         arbitrators.push_back(arbiresults_itr->arbitrator);
         arbiresults_tb.erase(arbiresults_itr);
         arbiresults_itr = arbiresults_tb.begin();
      }
   }

   return arbitrators;
}

/**
 * 更新仲裁正确率
 */
void bos_oracle::update_arbitration_correction(uint64_t arbitration_id) {
   uint64_t service_id = arbitration_id;

   auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
   auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
   check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such arbitration in update_arbitration_correction");

   auto update_correction = [&](name arbitrator, uint8_t correct_times) {
      auto arbitrator_tb = arbitrators(get_self(), get_self().value);
      auto arbitrator_itr = arbitrator_tb.find(arbitrator.value);
      check(arbitrator_itr != arbitrator_tb.end(), "Could not find such arbitrator in update_arbitration_correction");

      arbitrator_tb.modify(arbitrator_itr, get_self(), [&](auto& p) {
         p.arbitration_times += 1;
         p.arbitration_correct_times += correct_times;
      });
   };

   for (uint8_t round = 1; round <= arbitration_case_itr->last_round; ++round) {
      auto arbiprocess_tb = arbitration_processes(get_self(), arbitration_id);
      auto arbiprocess_itr = arbiprocess_tb.find(round);
      check(arbiprocess_itr != arbiprocess_tb.end(), "Can not find such process calculate correction");

      auto arbiresults_tb = arbitration_results(get_self(), ((arbitration_id << 2) | (0x03 & round)));
      for (auto arbiresults_itr = arbiresults_tb.begin(); arbiresults_itr != arbiresults_tb.end(); ++arbiresults_itr) {
         update_correction(arbiresults_itr->arbitrator, (arbiresults_itr->result == arbiprocess_itr->arbitration_result ? 1 : 0));
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

         // 通知仲裁结果
         auto notify_amount = eosio::asset(1, core_symbol());
         // Transfer to appeallant
         auto memo = "final result>arbitration_id:" + std::to_string(arbitration_id) + ",final_arbitration_result:" + std::to_string(arbiprocess_itr->role_type);
         check(arbiprocess_itr->appeallants.size() > 0, "no appeallant in the round ");

         for (auto& a : arbiprocess_itr->appeallants) {
            transfer(get_self(), a, notify_amount, memo);
         }

         check(arbiprocess_itr->respondents.size() > 0, "no respondents in the round ");

         for (auto& r : arbiprocess_itr->respondents) {
            transfer(get_self(), r, notify_amount, memo);
         }
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
         // 通知仲裁结果
         auto notify_amount = eosio::asset(1, core_symbol());
         // Transfer to appeallant
         auto memo = "final result>arbitration_id:" + std::to_string(arbitration_id) + ",final_arbitration_result:" + std::to_string(arbiprocess_itr->role_type);
         check(arbiprocess_itr->appeallants.size() > 0, "no appeallant in the round ");

         for (auto& a : arbiprocess_itr->appeallants) {
            transfer(get_self(), a, notify_amount, memo);
         }
      }
      break;
   }
   case arbitration_timer_type::accept_arbitrate_invitation_timeout: {
      // 如果状态为还在选择仲裁员, 那么继续选择仲裁员
      uint8_t inc_arbitrator_count = arbiprocess_itr->required_arbitrator - arbiprocess_itr->arbitrators.size();
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
   std::tuple<std::vector<name>, asset> slash_stake_accounts = get_balances(arbitration_id, loser_role_type);
   int64_t slash_amount = std::get<1>(slash_stake_accounts).amount;

   // if final winner is not provider then slash  all service providers' stakes
   if (arbitration_role_type::provider == loser_role_type) {
      std::tuple<std::vector<name>, asset> service_stakes = get_provider_service_stakes(service_id);
      slash_amount += std::get<1>(service_stakes).amount; //  add slash service stake from all service providers
      slash_service_stake(service_id, std::get<0>(service_stakes), std::get<1>(service_stakes));
   }

   double dividend_percent = 0.8;
   double slash_amount_dividend_part = slash_amount * dividend_percent;
   double slash_amount_fee_part = slash_amount * (1 - dividend_percent);
   bool check_value = (slash_amount_dividend_part > 0 && slash_amount_fee_part > 0);
   // check(check_value, "slash_amount_dividend_part > 0 && slash_amount_fee_part > 0");

   // award stake accounts
   std::tuple<std::vector<name>, asset> award_stake_accounts = get_balances(arbitration_id, arbitration_case_itr->final_result);
   // slash all losers' arbitration stake
   slash_arbitration_stake(arbitration_id, std::get<0>(slash_stake_accounts));

   std::vector<name> asa = std::get<0>(award_stake_accounts);
   if (slash_amount_dividend_part > 0 && !asa.empty()) {
      // pay all winners' award
      pay_arbitration_award(arbitration_id, std::get<0>(award_stake_accounts), slash_amount_dividend_part);
   } else {
      print(asa.size(), "=slash_amount_dividend_part=", slash_amount_dividend_part);
   }

   std::vector<name> arbitrators = get_arbitrators_of_uploading_arbitration_result(arbitration_id);
   if (slash_amount_fee_part > 0 && !arbitrators.empty()) {
      // pay all arbitrators' arbitration fee
      pay_arbitration_fee(arbitration_id, arbitrators, slash_amount_fee_part);
   } else {
      print(arbitrators.size(), "=slash_amount_fee_part=", slash_amount_fee_part);
   }

   if (arbitration_case_itr->final_result == arbitration_role_type::provider) {
      unfreeze_asset(service_id, arbitration_id);
   }

   // clear 申诉者表
   for (int i = 1; i <= arbitration_case_itr->last_round; ++i) {
      auto appeal_request_tb = appeal_requests(get_self(), ((service_id << 2) | (0x03 & i)));
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
std::tuple<std::vector<name>, asset> bos_oracle::get_balances(uint64_t arbitration_id, uint8_t role_type) {
   uint64_t stake_type = static_cast<uint64_t>(role_type);
   arbitration_stake_accounts stake_acnts(_self, arbitration_id);
   auto type_index = stake_acnts.get_index<"type"_n>();
   auto type_itr = type_index.lower_bound(stake_type);
   auto upper = type_index.upper_bound(stake_type);
   std::vector<name> accounts;
   asset stakes = asset(0, core_symbol());
   for (; type_itr != upper; ++type_itr) {
      if (type_itr->role_type == role_type) {
         accounts.push_back(type_itr->account);
         stakes += type_itr->balance;
      } else {
         break;
      }
   }

   return std::make_tuple(accounts, stakes);
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
void bos_oracle::slash_arbitration_stake(uint64_t arbitration_id, std::vector<name>& slash_accounts) {
   arbitration_stake_accounts stake_acnts(_self, arbitration_id);
   for (auto& a : slash_accounts) {
      auto acc = stake_acnts.find(a.value);
      check(acc != stake_acnts.end(), "no account in arbitration_stake_accounts");

      stake_acnts.modify(acc, same_payer, [&](auto& a) { a.balance = asset(0, core_symbol()); });
   }
}

/**
 * @brief
 *
 * @param arbitration_id
 * @param award_accounts
 * @param dividend_amount
 */
void bos_oracle::pay_arbitration_award(uint64_t arbitration_id, std::vector<name>& award_accounts, double dividend_amount) {
   print("=====pay_arbitration_award=====");
   check(!award_accounts.empty(), "no accounts in award_accounts");
   int64_t average_award_amount = static_cast<int64_t>(dividend_amount / award_accounts.size());
   if (average_award_amount > 0) {
      for (auto& a : award_accounts) {
         add_income(a, asset(average_award_amount, core_symbol()));
         print("=====pay_arbitration_award===in==");
      }
   }
   print("=====pay_arbitration_award==end===");
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

/**
 * @brief
 *
 * @param arbitration_id
 * @param fee_accounts
 * @param fee_amount
 */
void bos_oracle::pay_arbitration_fee(uint64_t arbitration_id, const std::vector<name>& fee_accounts, double fee_amount) {
   print("=====pay_arbitration_fee=====");

   auto abr_table = arbitrators(get_self(), get_self().value);

   for (auto& a : fee_accounts) {
      add_income(a, asset(static_cast<int64_t>(fee_amount), core_symbol()));
      print("=====pay_arbitration_fee==in===");
   }

   print("=====pay_arbitration_fee===end==");
}

void bos_oracle::stake_arbitration(uint64_t id, name account, asset amount, uint8_t round, uint8_t role_type, string memo) {
   uint64_t id_round = (id << 3) | round;
   arbitration_stake_records staketable(_self, id_round);

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
   uint64_t id_round = (id << 3) | round;
   arbitration_stake_records staketable(_self, id_round);

   auto stake_itr = staketable.find(account.value);
   check(stake_itr != staketable.end(), "no stake");
}

void bos_oracle::unstakearbi(uint64_t arbitration_id, name account, asset amount, std::string memo) {
   require_auth(account);

   check(amount.amount > 0, "stake amount could not be  equal to zero");

   arbitration_stake_accounts stake_acnts(_self, arbitration_id);

   auto acc = stake_acnts.find(account.value);
   check(acc != stake_acnts.end(), "no account found");
   check(acc->balance.amount >= amount.amount, "overdrawn stake balance");

   stake_acnts.modify(acc, same_payer, [&](auto& a) { a.balance -= amount; });

   transfer(_self, account, amount, "");
}

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