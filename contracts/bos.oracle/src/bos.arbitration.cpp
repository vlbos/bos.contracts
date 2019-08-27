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

void bos_oracle::setstatus(uint64_t arbitration_id) {
}

void bos_oracle::importwps(vector<name> arbitrators) {
}

/**
 * 注册仲裁员
 */
// void bos_oracle::regarbitrat( name account,  uint8_t type, asset amount, std::string public_info ) {
//     require_auth( account );
//     _regarbitrat(  account,    type,  amount,  public_info );
// }

void bos_oracle::_regarbitrat(name account, uint8_t type, asset amount, std::string public_info) {
  check(type == arbitrator_type::fulltime || type == arbitrator_type::crowd,
        "Arbitrator type can only be 1 or 2.");
  auto abr_table = arbitrators(get_self(), get_self().value);
  auto itr = abr_table.find(account.value);
  check(itr == abr_table.end(), "Arbitrator already registered");
  // TODO
  check(amount.amount >= uint64_t(10000) * pow(10, core_symbol().precision()), "stake amount could not be less than 10000");
  stake_arbitration(account.value, account, amount, 0, 0, "");

  // 注册仲裁员, 填写信息
  abr_table.emplace(get_self(), [&](auto &p) {
    p.account = account;
    p.type = type;
    p.public_info = public_info;
    p.correct_rate = 1.0f;
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

void bos_oracle::_appeal(name appeallant, uint64_t service_id, asset amount, std::string reason, std::string evidence, bool is_provider) {

  // check(arbi_method == arbi_method_type::public_arbitration ||
  //           arbi_method_type::multiple_rounds,
  //       "`arbi_method` can only be 1 or 2.");

  // 检查申诉的服务的服务状态
  data_services svctable(get_self(), get_self().value);
  auto svc_itr = svctable.find(service_id);
  check(svc_itr != svctable.end(), "service does not exist");
  check(svc_itr->status == service_status::service_in,
        "service status shoule be service_in");

  uint64_t arbitration_id = service_id;
  const uint8_t arbi_freeze_stake_duration = 1; //days
  // add_freeze
  const uint32_t duration = eosio::days(arbi_freeze_stake_duration).to_seconds();
  add_delay(service_id, appeallant, bos_oracle::current_time_point_sec(),
            duration, amount);

  const uint8_t arbi_case_deadline = 1; //hours
  uint8_t current_round = 1;

  // Arbitration case application
  auto arbitration_case_tb = arbitration_cases(get_self(), service_id);

  auto insert_arbi_case = [&]() {
    arbitration_case_tb.emplace(get_self(), [&](auto &p) {
      p.arbitration_id = arbitration_id;
      p.arbi_step = arbi_step_type::arbi_init; // 仲裁状态为初始化状态, 等待应诉
      p.last_round = current_round;
    });
  };

  auto arbiprocess_tb = arbitration_processes(get_self(), arbitration_id);

  auto clear_arbi_process = [&](uint64_t arbitration_id) {
    std::vector<uint8_t> rounds;
    for (auto &p : arbiprocess_tb) {
      rounds.push_back(p.round);
    }

    for (auto &r : rounds) {
      auto arbiprocess_itr = arbiprocess_tb.find(r);
      if (arbiprocess_itr != arbiprocess_tb.end()) {
        arbiprocess_tb.erase(arbiprocess_itr);
      }
    }
  };

  // auto arbitration_case_tb_by_svc = arbitration_case_tb.template get_index<"svc"_n>();
  auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);

  // 仲裁案件不存在或者存在但是状态为开始仲裁, 那么创建一个仲裁案件
  if (arbitration_case_itr == arbitration_case_tb.end()) {
    insert_arbi_case();
  } else if (arbitration_case_itr->arbi_step == arbi_step_type::arbi_resp_appeal_timeout_end || arbitration_case_itr->arbi_step == arbi_step_type::arbi_public_end) {
    arbitration_case_tb.erase(arbitration_case_itr);
    clear_arbi_process(arbitration_id);
    insert_arbi_case();
  } else {
    current_round = arbitration_case_itr->last_round;
  }

  arbitration_case_itr = arbitration_case_tb.find(arbitration_id);

  print("arbi_step=", arbitration_case_itr->arbi_step);
  std::set<uint8_t> steps = {arbi_step_type::arbi_init, arbi_step_type::arbi_wait_for_resp_appeal, arbi_step_type::arbi_reappeal, arbi_step_type::arbi_public_end};
  auto steps_itr = steps.find(arbitration_case_itr->arbi_step);
  check(steps_itr != steps.end(), "should not  appeal,arbitration is processing");
  // check(arbi_step_type::arbi_init == arbitration_case_itr->arbi_step || arbi_step_type::arbi_wait_for_resp_appeal == arbitration_case_itr->arbi_step
  // || arbi_step_type::arbi_reappeal == arbitration_case_itr->arbi_step || arbi_step_type::arbi_public_end == arbitration_case_itr->arbi_step,
  //       "should not  appeal,arbitration is processing");

  if (arbitration_case_itr->arbi_step == arbi_step_type::arbi_wait_for_resp_appeal) {
    current_round++;
    // 仲裁案件存在, 为此案件新增一个再申诉者
    arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto &p) {
      p.arbi_step = arbi_step_type::arbi_reappeal;
      p.last_round = current_round;
    });
  }

  auto arbiprocess_itr = arbiprocess_tb.find(current_round);

  uint8_t arbitrator_type = arbitrator_type::fulltime;
  if (arbiprocess_itr->arbi_method != arbi_method_type::multiple_rounds) {
    arbitrator_type = arbitrator_type::crowd;
  }

  uint8_t fulltime_count = 0;
  // 遍历仲裁员表, 找出可以选择的仲裁员
  auto arb_table = arbitrators(get_self(), get_self().value);
  for (auto itr = arb_table.begin(); itr != arb_table.end() && 0 != (arbitrator_type & itr->type) && itr->correct_rate > bos_oracle::default_arbitration_correct_rate; ++itr) {
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
    arbiprocess_tb.emplace(get_self(), [&](auto &p) {
      p.arbitration_id = arbitration_id;
      p.round = current_round;            // 仲裁过程为第一轮
      p.required_arbitrator = arbi_count; // 每一轮需要的仲裁员的个数
      p.appeallants.push_back(appeallant);
      p.arbi_method = arbi_method;
      p.is_provider = is_provider;
    });
  } else {
    // 新增申诉者
    arbiprocess_tb.modify(arbiprocess_itr, get_self(), [&](auto &p) {
      p.appeallants.push_back(appeallant);
    });
  }

  // 申诉者表
  auto appeal_request_tb = appeal_requests(get_self(), service_id);

  uint8_t sponsor_bit = 0;
  uint8_t provider_bit = 0;
  // 空或申请结束两种情况又产生新的申诉
  if (appeal_request_tb.begin() != appeal_request_tb.end()) {
    sponsor_bit = 0x01;
  }
  if (is_provider) {
    provider_bit = 0x02;
  }

  // 创建申诉者
  appeal_request_tb.emplace(get_self(), [&](auto &p) {
    p.appeal_id = appeal_request_tb.available_primary_key();
    p.role_type = (sponsor_bit | provider_bit);
    p.appeallant = appeallant;
    p.appeal_time = bos_oracle::current_time_point_sec();
    p.reason = reason;
    p.round = current_round;
  });

  uint64_t stake_amount_limit = pow(2, current_round) * uint64_t(100);
  uint64_t stake_amount_limit_asset_value = stake_amount_limit * pow(10, core_symbol().precision());
  std::string checkmsg = "appeal stake amount could not be less than " + std::to_string(stake_amount_limit);
  check(amount.amount >= stake_amount_limit_asset_value, checkmsg.c_str());
  stake_arbitration(arbitration_id, appeallant, amount, current_round, is_provider, "");

  if (first_one) {
    if (is_provider) {
      uint8_t previous_round = current_round - 1;
      check(previous_round > 0, "wrong round");
      auto arbiprocess_itr = arbiprocess_tb.find(previous_round);
      check(arbiprocess_itr != arbiprocess_tb.end(), "no round info");

      check(arbiprocess_itr->appeallants.size() > 0, "no appeallant in the round ");

      for (auto &a : arbiprocess_itr->appeallants) {
        auto notify_amount = eosio::asset(1, core_symbol());
        // Transfer to appeallant
        auto memo = "resp appeallant >arbitration_id: " + std::to_string(arbitration_id) +
                    ", service_id: " + std::to_string(service_id) +
                    ", state_amount: " + amount.to_string();
        transfer(get_self(), a, notify_amount, memo);
      }

    } else {
      // Data provider
      auto svcprovider_tb = data_service_provisions(get_self(), service_id);
      check(svcprovider_tb.begin() != svcprovider_tb.end(), "Such service has no providers.");

      // Service data providers
      bool hasProvider = false;
      // 对所有的数据提供者发送通知, 通知数据提供者应诉
      for (auto itr = svcprovider_tb.begin(); itr != svcprovider_tb.end();
           ++itr) {
        if (itr->status == service_status::service_in) {
          hasProvider = true;
          auto notify_amount = eosio::asset(1, core_symbol());
          // Transfer to provider
          auto memo = "resp>arbitration_id: " + std::to_string(arbitration_id) +
                      ", service_id: " + std::to_string(service_id) +
                      ", state_amount: " + amount.to_string();
          transfer(get_self(), itr->account, notify_amount, memo);
        }
      }

      check(hasProvider, "no provider");
    }

    arbitration_case_tb.modify(arbitration_case_itr, get_self(),
                               [&](auto &p) {
                                 p.arbi_step = arbi_step_type::arbi_wait_for_resp_appeal;
                                 p.last_round = current_round;
                               });
    const uint8_t arbi_resp_appeal_timeout_value = 1; //hours
    timeout_deferred(arbitration_id, current_round, arbitration_timer_type::resp_appeal_timeout,
                     eosio::hours(arbi_resp_appeal_timeout_value).to_seconds());
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

  uint64_t stake_amount_limit = pow(2, current_round) * uint64_t(100) * pow(10, core_symbol().precision());
  std::string checkmsg = "resp case stake amount could not be less than " + std::to_string(stake_amount_limit);
  check(amount.amount >= stake_amount_limit, checkmsg.c_str());
  stake_arbitration(arbitration_id, respondent, amount, current_round, false, "");

  auto arbiprocess_tb = arbitration_processes(get_self(), arbitration_id);
  auto arbiprocess_itr = arbiprocess_tb.find(current_round);
  check(arbiprocess_itr != arbiprocess_tb.end(), "no round process");

  if (arbitration_case_itr->arbi_step == arbi_wait_for_resp_appeal) {
    // 修改仲裁案件状态为: 正在选择仲裁员
    arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto &p) {
      p.arbi_step = arbi_step_type::arbi_choosing_arbitrator;
    });

    // 随机选择仲裁员
    random_chose_arbitrator(arbitration_id, current_round, service_id, arbiprocess_itr->required_arbitrator);
  }

  // 新增应诉者
  arbiprocess_tb.modify(arbiprocess_itr, get_self(), [&](auto &p) {
    p.respondents.push_back(respondent);
  });

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
  arbievidencetable.emplace(get_self(), [&](auto &e) {
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
  check(result == 0 || result == 1, "`result` can only be 0 or 1.");
  uint64_t service_id = arbitration_id;
  auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
  auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
  check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such arbitration case .uploadresult");
  check(arbitration_case_itr->arbi_step == arbi_step_type::arbi_wait_for_upload_result, "arbitration setp shoule  be upload result");

  uint8_t round = arbitration_case_itr->last_round;
  // 仲裁员上传本轮仲裁结果
  auto arbiprocess_tb = arbitration_processes(get_self(), arbitration_id);
  auto arbiprocess_itr = arbiprocess_tb.find(round);
  check(arbiprocess_itr != arbiprocess_tb.end(), "Can not find such process.");

  arbiprocess_tb.modify(arbiprocess_itr, get_self(), [&](auto &p) { p.arbitrator_arbitration_results.push_back(result); });

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
  uint64_t arbi_result = 0;
  if (arbiprocess_itr->total_result() > arbiprocess_itr->required_arbitrator / 2) {
    arbi_result = 1;
  }

  // 修改本轮的结果
  arbiprocess_tb.modify(arbiprocess_itr, get_self(),
                        [&](auto &p) { p.arbitration_result = arbi_result; });

  arbitration_case_tb.modify(arbitration_case_itr, get_self(),
                             [&](auto &p) {
                               p.arbitration_result = arbi_result;
                             });

  // Calculate arbitration correction.
  update_arbitration_correcction(arbitration_id);

  // 通知仲裁结果
  auto notify_amount = eosio::asset(1, core_symbol());
  // Transfer to appeallant
  auto memo = "result>arbitration_id: " + std::to_string(arbitration_id) +
              ", arbitration_result: " + std::to_string(arbi_result);
  check(arbiprocess_itr->appeallants.size() > 0, "no appeallant in the round ");

  for (auto &a : arbiprocess_itr->appeallants) {
    transfer(get_self(), a, notify_amount, memo);
  }

  check(arbiprocess_itr->respondents.size() > 0, "no respondents in the round ");

  for (auto &r : arbiprocess_itr->respondents) {
    transfer(get_self(), r, notify_amount, memo);
  }

  // 看是否有人再次申诉, 大众仲裁不允许再申诉
  if (arbiprocess_itr->arbi_method == arbi_method_type::multiple_rounds) {
    arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto &p) {
      p.arbi_step = arbi_step_type::arbi_wait_for_reappeal;
    });
    const uint8_t arbi_reappeal_timeout_value = 1; ///hours
    timeout_deferred(arbitration_id, round, arbitration_timer_type::reappeal_timeout, eosio::hours(arbi_reappeal_timeout_value).to_seconds());
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

  bool public_arbi = arbiprocess_itr->arbi_method == arbi_method_type::public_arbitration; // 是否为大众仲裁

  arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto &p) {
    p.arbitrators.push_back(arbitrator);
  });

  arbiprocess_tb.modify(arbiprocess_itr, get_self(), [&](auto &p) { p.arbitrators.push_back(arbitrator); });

  // 如果仲裁员人数满足需求, 那么开始仲裁
  if (arbiprocess_itr->arbitrators.size() >= arbiprocess_itr->required_arbitrator) {
    arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto &p) {
      p.arbi_step = arbi_step_type::arbi_wait_for_upload_result;
    });

    // 通知仲裁结果
    auto notify_amount = eosio::asset(1, core_symbol());
    // Transfer to appeallant
    auto memo = "wait upload result>arbitration_id: " + std::to_string(arbitration_id);
    check(arbiprocess_itr->arbitrators.size() > 0, "no arbitrators in the round ");

    for (auto &a : arbiprocess_itr->arbitrators) {
      transfer(get_self(), a, notify_amount, memo);
    }

    const uint8_t arbi_upload_result_timeout_value = 1; //hours
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
    arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto &p) {
      p.chosen_arbitrators.insert(p.chosen_arbitrators.end(), chosen_arbitrators.begin(), chosen_arbitrators.end());
      p.arbi_step = arbi_step_type::arbi_wait_for_accept_arbitrate_invitation;
    });

    // 保存被选择的仲裁员
    arbiprocess_tb.modify(arbiprocess_itr, get_self(), [&](auto &p) {
      // 刚好选择完毕仲裁员, 那么设置这些仲裁员需要在指定时间内应诉的时间
      p.arbiresp_deadline = bos_oracle::current_time_point_sec() + eosio::days(arbiresp_deadline_days);
    });

    auto notify_amount = eosio::asset(1, core_symbol());

    // 通知选择的仲裁员
    for (auto &arbitrator : chosen_arbitrators) {
      auto memo = "invite>arbitration_id: " + std::to_string(arbitration_id) +
                  ", service_id: " + std::to_string(service_id);
      transfer(get_self(), arbitrator, notify_amount, memo);
    }

    const uint8_t arbi_resp_arbitrate_timeout_value = 1; //hours
    uint8_t timer_type = arbitration_timer_type::accept_arbitrate_invitation_timeout;
    uint32_t time_length = eosio::hours(arbi_resp_arbitrate_timeout_value).to_seconds();
    // 等待仲裁员响应
    timeout_deferred(arbitration_id, round, timer_type, time_length);

    return;
  }

  ///public arbitration
  uint8_t arbitrator_type = arbitrator_type::fulltime;
  if (arbiprocess_itr->arbi_method != arbi_method_type::multiple_rounds) {
    arbitrator_type = arbitrator_type::crowd;
  }

  uint8_t fulltime_count = 0;
  // 遍历仲裁员表, 找出可以选择的仲裁员
  auto arb_table = arbitrators(get_self(), get_self().value);
  for (auto itr = arb_table.begin(); itr != arb_table.end() && 0 != (arbitrator_type & itr->type) && itr->correct_rate > bos_oracle::default_arbitration_correct_rate; ++itr) {
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
    arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto &p) {
      p.arbi_step = arbi_step_type::arbi_choosing_arbitrator; //大众仲裁阶段开始
    });
    // 修改本轮仲裁过程为大众仲裁
    arbiprocess_tb.modify(arbiprocess_itr, get_self(), [&](auto &p) {
      p.arbi_method = arbi_method_type::public_arbitration; //仲裁案件变为大众仲裁
      p.required_arbitrator = 2 * fulltime_count;           // 该轮需要的仲裁员的个数
      p.arbi_method = arbi_method_type::public_arbitration; //本轮变为大众仲裁
    });

    // 挑选专业仲裁员过程中人数不够，进入大众仲裁，开始随机挑选大众仲裁
    random_chose_arbitrator(arbitration_id, round, arbitration_id, fulltime_count * 2);

    return;
  }

  // 大众仲裁人数不够, TODO

  print("public arbitration processing ");
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
    arbitrator_type = arbitrator_type::crowd;
  }

  // 遍历仲裁员表, 找出可以选择的仲裁员
  auto arb_table = arbitrators(get_self(), get_self().value);
  for (auto itr = arb_table.begin(); itr != arb_table.end() && 0 != (arbitrator_type & itr->type) && itr->correct_rate > bos_oracle::default_arbitration_correct_rate; ++itr) {
    auto chosen = std::find(chosen_arbitrators.begin(), chosen_arbitrators.end(), itr->account);
    if (chosen == chosen_arbitrators.end()) {
      chosen_from_arbitrators.push_back(itr->account);
    }
  }

  if (chosen_from_arbitrators.size() < required_arbitrator_count) {
    return std::vector<name>{};
  }

  // 挑选 `required_arbitrator_count` 个仲裁员
  while (arbitrators_set.size() < required_arbitrator_count) {
    auto total_arbi = chosen_from_arbitrators.size();
    auto tmp = tapos_block_prefix();
    auto arbi_id = random((void *)&tmp, sizeof(tmp));
    arbi_id %= total_arbi;
    auto arbitrator = chosen_from_arbitrators.at(arbi_id);
    if (arbitrators_set.find(arbitrator) != arbitrators_set.end()) {
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
  auto arbi_result_tb = arbitration_results(get_self(), arbitration_id);
  arbi_result_tb.emplace(get_self(), [&](auto &p) {
    p.result_id = arbi_result_tb.available_primary_key();
    p.result = result;
    p.round = round;
    p.arbitrator = arbitrator;
    p.comment = comment;
  });
}

/**
 * 更新仲裁正确率
 */
void bos_oracle::update_arbitration_correcction(uint64_t arbitration_id) {
  uint64_t service_id = arbitration_id;

  auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
  auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
  check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such arbitration.update_arbitration_correcction");
  auto arbiresults_tb = arbitration_results(get_self(), arbitration_id);
  auto arbitrator_tb = arbitrators(get_self(), get_self().value);

  auto arbitrators = arbitration_case_itr->arbitrators;
  for (auto arbitrator : arbitrators) {
    uint64_t correct = 0;
    uint64_t total = 0;
    for (auto itr = arbiresults_tb.begin(); itr != arbiresults_tb.end(); ++itr) {
      if (itr->arbitrator == arbitrator) {
        ++total;
        if (itr->result == arbitration_case_itr->arbitration_result) {
          ++correct;
        }
      }
    }

    double rate = correct > 0 && total > 0 ? 1.0 * correct / total : 1.0f;
    auto arbitrator_itr = arbitrator_tb.find(arbitrator.value);

    arbitrator_tb.modify(arbitrator_itr, get_self(), [&](auto &p) {
      p.correct_rate = rate;
    });
  }
}

uint128_t bos_oracle::make_deferred_id(uint64_t arbitration_id, uint8_t timer_type) {
  return (uint128_t(arbitration_id) << 64) | timer_type;
}

void bos_oracle::timeout_deferred(uint64_t arbitration_id, uint8_t round, uint8_t timer_type, uint32_t time_length) {
  transaction t;
  t.actions.emplace_back(permission_level{_self, active_permission}, _self, "timertimeout"_n, std::make_tuple(arbitration_id, round, timer_type));
  t.delay_sec = time_length;
  uint128_t deferred_id = make_deferred_id(arbitration_id, timer_type);
  cancel_deferred(deferred_id);
  print(">>>===timertimeout=", deferred_id, "=id=", arbitration_id, "=round=", round, "=timer_type=", timer_type, "=time_length=", time_length);
  t.send(deferred_id, get_self());
}

void bos_oracle::timertimeout(uint64_t arbitration_id, uint8_t round, uint8_t timer_type) {
  uint64_t service_id = arbitration_id;
  auto arbitration_case_tb = arbitration_cases(get_self(), service_id);
  auto arbitration_case_itr = arbitration_case_tb.find(arbitration_id);
  check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such arbitration.timertimeout");

  auto arbiprocess_tb = arbitration_processes(get_self(), arbitration_id);
  auto arbiprocess_itr = arbiprocess_tb.find(round);
  check(arbitration_case_itr != arbitration_case_tb.end(), "Can not find such process.");

  uint8_t half_count = arbiprocess_itr->required_arbitrator / 2;
  uint8_t consumer_results = arbiprocess_itr->total_result();
  uint8_t results_count = arbiprocess_itr->arbitrator_arbitration_results.size();

  print(">>>===timer_type=", timer_type);
  switch (timer_type) {
  case arbitration_timer_type::reappeal_timeout: { // 再次申诉
    if (arbitration_case_itr->arbi_step == arbi_step_type::arbi_wait_for_reappeal) {
      print(">>>===timer_type=arbitration_timer_type::reappeal_timeout");
      // 没人再次申诉, 记录最后一次仲裁过程
      arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto &p) {
        p.last_round = round;
        p.arbi_step = arbi_step_type::arbi_reappeal_timeout_end;
        p.final_result = p.arbitration_result;
      });
      handle_arbitration_result(arbitration_id);
    }
    break;
  }
  case arbitration_timer_type::resp_appeal_timeout: {
    // 如果仲裁案件状态仍然为初始化状态, 说明没有数据提供者应诉,
    // 直接处理仲裁结果
    if (arbitration_case_itr->arbi_step == arbi_step_type::arbi_wait_for_resp_appeal) {
      print(">>>===timer_type=arbitration_timer_type::resp_appeal_timeout");

      arbitration_case_tb.modify(arbitration_case_itr, get_self(), [&](auto &p) {
        p.arbi_step = arbi_step_type::arbi_resp_appeal_timeout_end;
        if (arbiprocess_itr->is_provider) {
          p.final_result = final_result_type::provider;
        } else {
          p.final_result = final_result_type::consumer;
        }
      });

      handle_arbitration_result(arbitration_id);
    }
    break;
  }
  case arbitration_timer_type::accept_arbitrate_invitation_timeout: {
    // 如果状态为还在选择仲裁员, 那么继续选择仲裁员
    uint8_t inc_arbitrator_count = arbiprocess_itr->required_arbitrator - arbiprocess_itr->arbitrators.size();
    if (arbitration_case_itr->arbi_step == arbi_step_type::arbi_choosing_arbitrator && inc_arbitrator_count > 0) {
      print(">>>===timer_type=arbitration_timer_type::accept_arbitrate_invitation_timeout");
      random_chose_arbitrator(arbitration_id, round, arbitration_id, inc_arbitrator_count);
    }
    break;
  }
  case arbitration_timer_type::upload_result_timeout: {
    bool is_provider_results = (-consumer_results) > half_count;

    if (consumer_results > half_count || is_provider_results) {
      print(">>>===timer_type=arbitration_timer_type::resp_appeal_timeout");

      uint128_t deferred_id = make_deferred_id(arbitration_id, arbitration_timer_type::upload_result_timeout);
      cancel_deferred(deferred_id);
      handle_upload_result(arbitration_id, round);
    } else {
      print(">>>===timer_type=arbitration_timer_type::upload_result_timeout else");

      uint8_t remainder = arbiprocess_itr->required_arbitrator - results_count;
      if (remainder > 0) {
        random_chose_arbitrator(arbitration_id, round, arbitration_id, remainder);
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

  const uint64_t arbitration_result_provider = 0;
  bool is_provider = arbitration_case_itr->final_result != arbitration_result_provider;
  std::tuple<std::vector<name>, asset> slash_stake_accounts = get_balances(arbitration_id, is_provider);
  int64_t slash_amount = std::get<1>(slash_stake_accounts).amount;

  // if final winner is not provider then slash  all service providers' stakes
  if (is_provider) {
    uint64_t service_id = arbitration_id;

    std::tuple<std::vector<name>, asset> service_stakes = get_provider_service_stakes(service_id);
    const std::vector<name> &accounts = std::get<0>(service_stakes);
    const asset &amount = std::get<1>(service_stakes);
    slash_amount += amount.amount; //  add slash service stake from all service providers
    slash_service_stake(service_id, accounts, amount);
  }

  double dividend_percent = 0.8;
  double slash_amount_dividend_part = slash_amount * dividend_percent;
  double slash_amount_fee_part = slash_amount * (1 - dividend_percent);
  check(slash_amount_dividend_part > 0 && slash_amount_fee_part > 0, "");

  // award stake accounts
  std::tuple<std::vector<name>, asset> award_stake_accounts = get_balances(arbitration_id, !is_provider);
  // slash all losers' arbitration stake
  slash_arbitration_stake(arbitration_id, std::get<0>(slash_stake_accounts));

  // pay all winners' award
  pay_arbitration_award(arbitration_id, std::get<0>(award_stake_accounts), slash_amount_dividend_part);

  // pay all arbitrators' arbitration fee
  pay_arbitration_fee(arbitration_id, arbitration_case_itr->arbitrators, slash_amount_fee_part);
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

  stake_acnts.modify(acc, same_payer, [&](auto &a) { a.balance -= value; });
}

/**
 * @brief
 *
 * @param owner
 * @param value
 * @param arbitration_id
 * @param is_provider
 */
void bos_oracle::add_balance(name owner, asset value, uint64_t arbitration_id,
                             bool is_provider) {
  arbitration_stake_accounts stake_acnts(_self, arbitration_id);
  auto acc = stake_acnts.find(owner.value);
  if (acc == stake_acnts.end()) {
    stake_acnts.emplace(_self, [&](auto &a) {
      a.account = owner;
      a.balance = value;
      a.is_provider = is_provider;
    });
  } else {
    stake_acnts.modify(acc, same_payer, [&](auto &a) { a.balance += value; });
  }
}

/**
 * @brief
 *
 * @param arbitration_id
 * @param is_provider
 * @return std::tuple<std::vector<name>,asset>
 */
std::tuple<std::vector<name>, asset> bos_oracle::get_balances(uint64_t arbitration_id, bool is_provider) {
  uint64_t stake_type = static_cast<uint64_t>(is_provider);
  arbitration_stake_accounts stake_acnts(_self, arbitration_id);
  auto type_index = stake_acnts.get_index<"type"_n>();
  auto type_itr = type_index.lower_bound(stake_type);
  auto upper = type_index.upper_bound(stake_type);
  std::vector<name> accounts;
  asset stakes = asset(0, core_symbol());
  for (; type_itr != upper; ++type_itr) {
    if (type_itr->is_provider == is_provider) {
      accounts.push_back(type_itr->account);
      stakes += type_itr->balance;
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

  for (const auto &p : provisionstable) {
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
void bos_oracle::slash_service_stake(uint64_t service_id, const std::vector<name> &slash_accounts, const asset &amount) {

  // oracle internal account provider acount transfer to arbitrat account
  if (amount.amount > 0) {
    transfer(provider_account, arbitrat_account, amount, "");
  }

  for (auto &account : slash_accounts) {
    data_providers providertable(_self, _self.value);
    auto provider_itr = providertable.find(account.value);
    check(provider_itr != providertable.end(), "");

    data_service_provisions provisionstable(_self, service_id);

    auto provision_itr = provisionstable.find(account.value);
    check(provision_itr != provisionstable.end(), "account does not subscribe services");
    check(provider_itr->total_stake_amount >= provision_itr->amount, "account does not subscribe services");

    providertable.modify(provider_itr, same_payer, [&](auto &p) {
      p.total_stake_amount -= provision_itr->amount;
    });

    provisionstable.modify(provision_itr, same_payer, [&](auto &p) {
      p.amount = asset(0, core_symbol());
    });
  }
  data_service_stakes svcstaketable(_self, _self.value);
  auto svcstake_itr = svcstaketable.find(service_id);
  check(svcstake_itr != svcstaketable.end(), "");
  check(svcstake_itr->amount >= amount, "");

  svcstaketable.modify(svcstake_itr, same_payer,
                       [&](auto &ss) { ss.amount -= amount; });
}

/**
 * @brief
 *
 * @param arbitration_id
 * @param slash_accounts
 */
void bos_oracle::slash_arbitration_stake(uint64_t arbitration_id, std::vector<name> &slash_accounts) {
  arbitration_stake_accounts stake_acnts(_self, arbitration_id);
  for (auto &a : slash_accounts) {
    auto acc = stake_acnts.find(a.value);
    check(acc != stake_acnts.end(), "");

    stake_acnts.modify(acc, same_payer, [&](auto &a) { a.balance = asset(0, core_symbol()); });
  }
}

/**
 * @brief
 *
 * @param arbitration_id
 * @param award_accounts
 * @param dividend_amount
 */
void bos_oracle::pay_arbitration_award(uint64_t arbitration_id, std::vector<name> &award_accounts, double dividend_amount) {
  uint64_t award_size = award_accounts.size();
  check(award_size > 0, "");
  int64_t average_award_amount = static_cast<int64_t>(dividend_amount / award_size);
  if (average_award_amount > 0) {
    for (auto &a : award_accounts) {
      add_income(a, asset(average_award_amount, core_symbol()));
    }
  }
}

void bos_oracle::add_income(name account, asset quantity) {
  arbitration_income_accounts incometable(_self, account.value);
  auto acc = incometable.find(quantity.symbol.code().raw());
  if (acc == incometable.end()) {
    incometable.emplace(_self, [&](auto &a) {
      a.income = quantity;
      a.claim = asset(0, quantity.symbol);
    });
  } else {
    incometable.modify(acc, same_payer, [&](auto &a) { a.income += quantity; });
  }
}

/**
 * @brief
 *
 * @param arbitration_id
 * @param fee_accounts
 * @param fee_amount
 */
void bos_oracle::pay_arbitration_fee(uint64_t arbitration_id, const std::vector<name> &fee_accounts, double fee_amount) {
  auto abr_table = arbitrators(get_self(), get_self().value);

  for (auto &a : fee_accounts) {
    add_income(a, asset(static_cast<int64_t>(fee_amount), core_symbol()));
  }
}

void bos_oracle::stake_arbitration(uint64_t id, name account, asset amount, uint8_t round, bool is_provider, string memo) {
  uint64_t id_round = (id << 3) | round;
  arbitration_stake_records staketable(_self, id_round);

  auto stake_itr = staketable.find(account.value);
  if (stake_itr == staketable.end()) {
    staketable.emplace(_self, [&](auto &a) {
      a.account = account;
      a.amount = amount;
      a.stake_time = bos_oracle::current_time_point_sec();
    });
  } else {
    print("repeat id stake arbiration:", id, account, amount.amount, round, memo);
    staketable.modify(stake_itr, same_payer, [&](auto &a) { a.amount += amount; });
  }

  add_balance(account, amount, id, is_provider);
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

  stake_acnts.modify(acc, same_payer, [&](auto &a) { a.balance -= amount; });

  transfer(account, provider_account, amount, "");
}

void bos_oracle::claimarbi(name account, name receive_account) {
  require_auth(account);
  arbitration_income_accounts incometable(_self, account.value);

  auto income_itr = incometable.find(core_symbol().code().raw());
  check(income_itr != incometable.end(), "no income by account");

  asset new_income = income_itr->income - income_itr->claim;
  check(new_income.amount > 0, "no income ");

  incometable.modify(income_itr, same_payer, [&](auto &p) { p.claim += new_income; });

  transfer(consumer_account, receive_account, new_income, "claim arbi");
}