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
#include "bos.oracle/bos.util.hpp"
// namespace eosio {

using eosio::asset;
using eosio::public_key;
using std::string;

/**
 * 注册仲裁员
 */
void bos_oracle::regarbitrat( name account, public_key pubkey, uint8_t type, asset amount, std::string public_info ) {
    require_auth( account );
    _regarbitrat(  account,  pubkey,  type,  amount,  public_info );
}

void bos_oracle::_regarbitrat( name account, public_key pubkey, uint8_t type, asset amount, std::string public_info ) {
    check( type == arbitrator_type::profession || type == arbitrator_type::amateur, "Arbitrator type can only be 1 or 2." );
    auto abr_table = arbitrators( get_self(), get_self().value );
    auto iter = abr_table.find( account.value );
    check( iter == abr_table.end(), "Arbitrator already registered" );
    // TODO
    // transfer(account, arbitrat_account, amount, "regarbitrat deposit.");
    stake_arbitration(account.value,account, amount,0,0, "");

    // 注册仲裁员, 填写信息
    abr_table.emplace( get_self(), [&]( auto& p ) {
        p.account = account;
        p.pubkey = pubkey;
        p.type = type;
        p.public_info = public_info;
    } );
}

/**
 * 申诉者申诉
 */
void bos_oracle::complain( name applicant, uint64_t service_id, asset amount, std::string reason, uint8_t arbi_method ) {
    require_auth( applicant );
    _complain(  applicant,  service_id,  amount,  reason,  arbi_method ) ;
}

void bos_oracle::_complain( name applicant, uint64_t service_id, asset amount, std::string reason, uint8_t arbi_method ) {
   
    check( arbi_method == arbi_method_type::public_arbitration || arbi_method_type::multiple_rounds, "`arbi_method` can only be 1 or 2." );

    // 检查申诉的服务的服务状态
    data_services svctable(get_self(), get_self().value);
    auto svc_iter = svctable.find(service_id);
    check(svc_iter != svctable.end(), "service does not exist");
    check(svc_iter->status == service_status::service_in, "service status shoule be service_in");
   

    // 申诉者表
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

    // 创建申诉者
    auto appeal_id = 0;
    complainant_tb.emplace( get_self(), [&]( auto& p ) {
        p.appeal_id = complainant_tb.available_primary_key();
        p.service_id = service_id;
        p.status = complainant_status::wait_for_accept;
        p.is_sponsor = is_sponsor;
        p.applicant = applicant;
        p.appeal_time = time_point_sec(now());
        p.reason = reason;
        appeal_id = p.appeal_id;
    } );

 // TODO
    // transfer(applicant, arbitrat_account, amount, "complain deposit.");
    stake_arbitration(appeal_id,applicant, amount,0,0, "");   

    // add_freeze
    const uint64_t duration = eosio::days(1).to_seconds();
    add_delay(service_id, applicant, time_point_sec(now()), duration, amount);

    // Arbitration case application
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbicaseapp_tb_by_svc = arbicaseapp_tb.template get_index<"svc"_n>();
    auto arbicaseapp_iter = arbicaseapp_tb_by_svc.find( service_id );
    auto arbi_id = appeal_id;//arbicaseapp_tb.available_primary_key();
    // 仲裁案件不存在或者存在但是状态为开始仲裁, 那么创建一个仲裁案件
    if (arbicaseapp_iter == arbicaseapp_tb_by_svc.end() || 
        (arbicaseapp_iter != arbicaseapp_tb_by_svc.end() && arbicaseapp_iter->arbi_step == arbi_step_type::arbi_started)) {
        arbicaseapp_tb.emplace( get_self(), [&]( auto& p ) {
            p.arbitration_id = arbi_id;
            p.appeal_id = appeal_id;
            p.service_id = service_id;
            p.arbi_method = arbi_method;
            p.evidence_info = reason;
            p.arbi_step = arbi_step_type::arbi_init; // 仲裁状态为初始化状态, 等待应诉
            p.required_arbitrator = 5;
            p.deadline = time_point_sec(now() + 3600);
            p.add_applicant(applicant); // 增加申诉者
        } );
    } else {
        auto arbi_iter = arbicaseapp_tb.find(arbicaseapp_iter->arbitration_id);
        check(arbi_iter != arbicaseapp_tb.end(), "Can not find such arbitration.");
        // 仲裁案件存在, 为此案件新增一个申诉者
        arbicaseapp_tb.modify(arbi_iter, get_self(), [&]( auto& p ) {
            p.add_applicant(applicant);
        } );
    }

    // Data provider
    auto svcprovider_tb = data_service_provisions( get_self(), service_id );
    check(svcprovider_tb.begin() != svcprovider_tb.end(), "Such service has no providers.");

    // Service data providers
    bool hasProvider = false;
    // 对所有的数据提供者发送通知, 通知数据提供者应诉
    for(auto iter = svcprovider_tb.begin(); iter != svcprovider_tb.end(); iter++)
    {
        if(!iter->stop_service) {
            hasProvider = true;
            auto notify_amount = eosio::asset(1, core_symbol());
            // Transfer to provider
            auto memo = "arbitration_id: " + std::to_string(arbi_id)
                + ", service_id: " + std::to_string(service_id) 
                + ", state_amount: " + amount.to_string();
            transfer(get_self(), iter->account, notify_amount, memo);
        }
    }
    check(hasProvider, "no provider");
    timeout_deferred(arbi_id, 0,arbitration_timer_type::resp_appeal_timeout, eosio::hours(10).to_seconds());
}

/**
 * (数据提供者/数据使用者)应诉
 */
void bos_oracle::respcase( name respondent, uint64_t arbitration_id, asset amount,uint64_t process_id) {
    require_auth( respondent );
    _respcase(  respondent,  arbitration_id,  amount, process_id);
}

void bos_oracle::_respcase( name respondent, uint64_t arbitration_id, asset amount,uint64_t process_id) {
   
    // 检查仲裁案件状态
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbi_iter = arbicaseapp_tb.find( arbitration_id );
    check(arbi_iter != arbicaseapp_tb.end(), "Can not find such arbitration case application.");
    check(arbi_iter->arbi_step != arbi_step_type::arbi_started, "arbitration setp shoule not be arbi_started");

    // 修改仲裁案件状态为: 正在选择仲裁员
    arbicaseapp_tb.modify( arbi_iter, get_self(), [&]( auto& p ) {
        p.arbi_step = arbi_step_type::arbi_choosing_arbitrator;
    } );

    auto arbiprocess_tb = arbitration_processs( get_self(), get_self().value );
    auto arbipro_iter = arbiprocess_tb.find( process_id );
    if ( arbipro_iter == arbiprocess_tb.end() ) {
        uint64_t processid = 0;
        // 第一次应诉, 创建第1个仲裁过程
        arbiprocess_tb.emplace( get_self(), [&]( auto& p ) {
            p.process_id = arbiprocess_tb.available_primary_key();
            p.arbitration_id = arbitration_id;
            p.num_id = 1; // 仲裁过程为第一轮
            p.required_arbitrator = pow(2, p.num_id) + 1; // 每一轮需要的仲裁员的个数
            p.add_respondent(respondent);
            p.arbi_method = arbi_iter->arbi_method;
            processid = p.process_id;
        } );
        // 随机选择仲裁员
        random_chose_arbitrator(arbitration_id, process_id, arbi_iter->service_id, 3);
    } else {
        // 新增应诉者
        arbiprocess_tb.modify( arbipro_iter, get_self(), [&]( auto& p ) {
            p.arbitration_id = arbitration_id;
            p.add_respondent(respondent);
        } );
    }
}

void bos_oracle::uploadeviden( name applicant, uint64_t process_id, std::string evidence ) {
    require_auth( applicant );
    auto arbiprocess_tb = arbitration_processs( get_self(), get_self().value );
    auto arbipro_iter = arbiprocess_tb.find( process_id );
    check( arbipro_iter != arbiprocess_tb.end(), "Can not find such process.");

    // 申诉者上传仲裁证据
    arbiprocess_tb.modify( arbipro_iter, get_self(), [&]( auto& p ) {
        p.evidence_info = evidence;
    } );
}

/**
 * 仲裁员上传仲裁结果
 */
void bos_oracle::uploadresult( name arbitrator, uint64_t arbitration_id, uint64_t result, uint64_t process_id ) {
    require_auth( arbitrator );
    check(result == 0 || result == 1, "`result` can only be 0 or 1.");

    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbi_iter = arbicaseapp_tb.find( arbitration_id );
    check(arbi_iter != arbicaseapp_tb.end(), "Can not find such arbitration case application.");
    bool public_arbi = arbi_iter->arbi_method == arbi_method_type::public_arbitration;

    // 仲裁员上传本轮仲裁结果
    auto arbiprocess_tb = arbitration_processs( get_self(), get_self().value );
    auto arbipro_iter = arbiprocess_tb.find( process_id );
    check( arbipro_iter != arbiprocess_tb.end(), "Can not find such process.");
    arbiprocess_tb.modify( arbipro_iter, get_self(), [&]( auto& p ) {
        p.add_result(result);
    } );

    // 计算本轮结果, 条件为上传结果的人数 >= 本轮所需要的仲裁员人数 / 2 + 1
    // 满足条件取消计算结果定时器, 直接计算上传结果
    // Add result to arbitration_results
    add_arbitration_result(arbitrator, arbitration_id, result, process_id);

    if (arbipro_iter->result_size() >= arbipro_iter->required_arbitrator / 2 + 1) {
        auto timer_type = public_arbi ? arbitration_timer_type::public_upload_result_timeout : arbitration_timer_type::upload_result_timeout;
        uint128_t deferred_id = make_deferred_id(arbitration_id, timer_type);
        cancel_deferred(deferred_id);
        handle_upload_result(arbitrator, arbitration_id, process_id);
    }
}

/**
 * 处理上传结果
 */
void bos_oracle::handle_upload_result(name arbitrator, uint64_t arbitration_id, uint64_t process_id) {
    // 仲裁案件检查
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbi_iter = arbicaseapp_tb.find( arbitration_id );
    check( arbi_iter != arbicaseapp_tb.end(), "Can not find such arbitration case application." );

    // 仲裁过程检查
    auto arbiprocess_tb = arbitration_processs( get_self(), get_self().value );
    auto arbipro_iter = arbiprocess_tb.find( process_id );
    check( arbipro_iter != arbiprocess_tb.end(), "Can not find such process." );

    // 此处1表示申诉者赢, 应诉者输
    uint64_t arbi_result = 0;
    if (arbipro_iter->total_result() >= arbipro_iter->required_arbitrator / 2) {
        arbi_result = 1;
    } else {
        arbi_result = 0;
    }
    // 修改本轮的结果
    arbiprocess_tb.modify( arbipro_iter, get_self(), [&]( auto& p ) {
        p.arbitration_result = arbi_result;
    } );

    // 看是否有人再次申诉, 大众仲裁不允许再申诉
    if (arbi_iter->arbi_method == arbi_method_type::multiple_rounds) {
        timeout_deferred(arbitration_id, process_id, arbitration_timer_type::reappeal_timeout, eosio::hours(10).to_seconds());
    }
}

/**
 * 仲裁员应邀
 */
void bos_oracle::acceptarbi( name arbitrator,  uint64_t arbitration_id, uint64_t process_id ) {
    require_auth( arbitrator );

    // 修改仲裁案件状态
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbi_iter = arbicaseapp_tb.find( arbitration_id );
    check(arbi_iter != arbicaseapp_tb.end(), "Can not find such arbitration.");
    bool public_arbi = arbi_iter->arbi_method == arbi_method_type::public_arbitration; // 是否为大众仲裁
    // auto amount = public_arbi ? 2 * amount : amount;

    // TODO
    // transfer(arbitrator, arbitrat_account, amount, "acceptarbi deposit.");
    // stake_arbitration(arbitration_id,arbitrator, amount,0,0, "reappeal.");

    arbicaseapp_tb.modify( arbi_iter, get_self(), [&]( auto& p ) {
        p.arbi_step = public_arbi ? arbi_step_type::arbi_public_responded 
            : arbi_step_type::arbi_responded; // 状态为有仲裁员确认
    } );

    // 将仲裁员添加此轮应诉的仲裁者中
    auto arbiprocess_tb = arbitration_processs(get_self(), get_self().value);
    auto arbiprocess_iter = arbiprocess_tb.find(process_id);
    check(arbiprocess_iter != arbiprocess_tb.end(), "Can not find such arbitration process.");
    arbiprocess_tb.modify( arbiprocess_iter, get_self(), [&]( auto& p ) {
        p.add_arbitrator(arbitrator);
    } );

    // 如果仲裁员人数满足需求, 那么开始仲裁
    if (arbiprocess_iter->arbitrators.size() >= arbiprocess_iter->required_arbitrator) {
        arbicaseapp_tb.modify( arbi_iter, get_self(), [&]( auto& p ) {
            p.arbi_step = public_arbi ? arbi_step_type::arbi_public_started : arbi_step_type::arbi_started;
        } );

        // 等待仲裁员上传结果
        if (public_arbi) {
            upload_result_timeout_deferred(arbitrator, arbitration_id, 0, arbitration_timer_type::public_upload_result_timeout, eosio::hours(10).to_seconds());
        } else {
            upload_result_timeout_deferred(arbitrator, arbitration_id, 0, arbitration_timer_type::upload_result_timeout, eosio::hours(10).to_seconds());
        }
    } else {
        if (arbiprocess_iter->arbiresp_deadline.utc_seconds < now()) {
            // 如果仲裁员没有在指定时间内应诉, 那么继续选择仲裁员
            auto arbi_to_chose = arbiprocess_iter->required_arbitrator - arbiprocess_iter->arbitrators.size();
            random_chose_arbitrator(arbitration_id, process_id, arbi_iter->service_id, arbi_to_chose);
        }
    }
}

/**
 * 再申诉, 申诉者可以为数据提供者或者数据使用者
 */
void bos_oracle::reappeal( name applicant, uint64_t arbitration_id, uint64_t service_id,
    uint64_t process_id, bool is_provider, asset amount, std::string reason ) {
    // 检查再申诉服务状态
    require_auth( applicant );
    _reappeal(  applicant,  arbitration_id,  service_id, process_id,  is_provider,  amount,  reason ) ;
    }

void bos_oracle::_reappeal( name applicant, uint64_t arbitration_id, uint64_t service_id, uint64_t process_id, bool is_provider, asset amount,  std::string reason ) {
    // 检查再申诉服务状态

    // 检查仲裁案件
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbicaseapp_iter = arbicaseapp_tb.find( arbitration_id );
    check (arbicaseapp_iter != arbicaseapp_tb.end(), "Can not find such arbitration case application.");
    check (arbicaseapp_iter->arbi_method == arbi_method_type::multiple_rounds, "Can only be multiple rounds arbitration.");

    // 改变仲裁状态为再申诉
    arbicaseapp_tb.modify(arbicaseapp_iter, get_self(), [&]( auto& p ) {
        p.add_applicant(applicant);
        p.is_provider = is_provider;
        p.arbi_step = arbi_step_type::arbi_reappeal;
    } );

    // 检查被申诉的数据服务状态为服务中
    data_services svctable(get_self(), get_self().value);
    auto svc_iter = svctable.find(service_id);
    check(svc_iter != svctable.end(), "service does not exist");
    check(svc_iter->status == service_status::service_in, "service status shoule be service_in");

    // TODO.
    // transfer(applicant, arbitrat_account, amount, "reappeal.");
    stake_arbitration(arbitration_id,applicant, amount,process_id,is_provider, "reappeal.");

    // 新增申诉者资料
    auto complainant_tb = complainants( get_self(), get_self().value );
    auto appeal_id = 0;
    complainant_tb.emplace( get_self(), [&]( auto& p ) {
        p.appeal_id = complainant_tb.available_primary_key();
        p.service_id = service_id;
        p.status = complainant_status::wait_for_accept;
        p.is_sponsor = false;
        p.is_provider = is_provider;
        p.arbitration_id = arbitration_id;
        p.applicant = applicant;
        p.appeal_time = time_point_sec(now());
        p.reason = reason;
        appeal_id = p.appeal_id;
    } );

    // add_freeze
    const uint64_t duration = eosio::days(1).to_seconds();
    add_delay(service_id, applicant, time_point_sec(now()), duration, amount);

    if(!is_provider) {
        // 如果是数据使用者再申诉, 那么通知所有的数据提供者
        auto svcprovider_tb = data_service_provisions( get_self(), service_id );
        check(svcprovider_tb.begin() != svcprovider_tb.end(), "Such service has no providers.");

        // Service data providers
        bool hasProvider = false;
        for(auto iter = svcprovider_tb.begin(); iter != svcprovider_tb.end(); iter++) {
            if(!iter->stop_service) {
                hasProvider = true;
                auto notify_amount = eosio::asset(1, core_symbol());
                // Transfer to provider
                auto memo = "arbitration_id: " + std::to_string(arbitration_id)
                    + ", service_id: " + std::to_string(service_id) 
                    + ", state_amount: " + amount.to_string();
                transfer(get_self(), iter->account, notify_amount, memo);
            }
        }
        check(hasProvider, "no provier");
    }

    // 等待对方应诉
    timeout_deferred(arbitration_id, process_id, arbitration_timer_type::resp_appeal_timeout, eosio::hours(10).to_seconds());
}

/**
 * 再次应诉
 * last_process_id 上一轮的仲裁过程ID
 */
void bos_oracle::rerespcase( name respondent, uint64_t arbitration_id, asset amount, uint64_t last_process_id) {
    require_auth( respondent );
    _rerespcase(  respondent,  arbitration_id,  amount, last_process_id);
}
void bos_oracle::_rerespcase( name respondent, uint64_t arbitration_id,  asset amount,uint64_t last_process_id) {
    // 检查仲裁案件状态
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbi_iter = arbicaseapp_tb.find( arbitration_id );
    check(arbi_iter != arbicaseapp_tb.end(), "Can not find such arbitration case application.");
    check (arbi_iter->arbi_method == arbi_method_type::multiple_rounds, "Can only be multiple rounds arbitration.");

    // TODO
    // transfer(arbitrator, arbitrat_account, amount, "acceptarbi deposit.");
    stake_arbitration(arbitration_id,respondent, amount,0,0, "_rerespcase.");

    // 修改仲裁案件状态为: 正在选择仲裁员
    arbicaseapp_tb.modify( arbi_iter, get_self(), [&]( auto& p ) {
        p.arbi_step = arbi_step_type::arbi_choosing_arbitrator;
    } );

    auto arbiprocess_tb = arbitration_processs( get_self(), get_self().value );
    auto arbipro_iter = arbiprocess_tb.find( last_process_id );
    check( arbipro_iter != arbiprocess_tb.end(), "Can not find such process" );

    uint64_t required_arbitrator = 0;
    uint64_t process_id = 0;
    // 新增一个仲裁过程, 更改仲裁轮次和需要的仲裁员人数
    arbiprocess_tb.emplace( get_self(), [&]( auto& p ) {
        p.process_id = arbiprocess_tb.available_primary_key();
        p.arbitration_id = arbitration_id;
        p.num_id = arbipro_iter->num_id + 1; // 仲裁过程为上一轮轮次 + 1
        p.required_arbitrator = pow(2, p.num_id) + 1; // 每一轮需要的仲裁员的个数
        p.add_respondent(respondent);
        p.arbi_method = arbi_iter->arbi_method;
        required_arbitrator = p.required_arbitrator;
        process_id = p.process_id;
    } );
    // 随机选择仲裁员
    random_chose_arbitrator(arbitration_id, process_id, arbi_iter->service_id, required_arbitrator);
}

/**
 * 随机选择仲裁员
 */
void bos_oracle::random_chose_arbitrator(uint64_t arbitration_id, uint64_t process_id, uint64_t service_id, uint64_t arbi_to_chose)  {
    vector<name> arbitrators = random_arbitrator(arbitration_id, process_id, arbi_to_chose);
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbi_iter = arbicaseapp_tb.find( arbitration_id );
    check( arbi_iter != arbicaseapp_tb.end(), "Can not find such arbitration." );

    auto notify_amount = eosio::asset(1, core_symbol());
    // 通知选择的仲裁员
    for (auto arbitrator : arbitrators) {
        auto memo = "arbitration_id: " + std::to_string(arbitration_id)
            + ", service_id: " + std::to_string(service_id);
        transfer(get_self(), arbitrator, notify_amount, memo);
    }

    // 等待仲裁员响应
    if (arbi_iter->arbi_method == arbi_method_type::multiple_rounds) {
        timeout_deferred(arbitration_id, process_id, arbitration_timer_type::resp_arbitrate_timeout, eosio::hours(10).to_seconds());
    } else if (arbi_iter->arbi_method == arbi_method_type::public_arbitration) {
        // 等待大众仲裁员响应
        timeout_deferred(arbitration_id, process_id, arbitration_timer_type::public_resp_arbitrate_timeout, eosio::hours(10).to_seconds());
    }
}

/**
 * 为某一个仲裁的某一轮随机选择 `arbi_to_chose` 个仲裁员
 */
vector<name> bos_oracle::random_arbitrator(uint64_t arbitration_id, uint64_t process_id, uint64_t arbi_to_chose)  {
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbicaseapp_iter = arbicaseapp_tb.find( arbitration_id );
    check( arbicaseapp_iter != arbicaseapp_tb.end(), "Can not find such arbitration" );

    auto arbiprocess_tb = arbitration_processs( get_self(), get_self().value );
    auto arbipro_iter = arbiprocess_tb.find( process_id );
    check( arbipro_iter != arbiprocess_tb.end(), "Can not find such arbitration process");

    auto chosen_arbitrators = arbipro_iter->arbitrators; // 本轮次已经选择的仲裁员
    std::vector<name> chosen_from_arbitrators; // 需要从哪里选择出来仲裁员的地方
    std::set<name> arbitrators_set;

    // 遍历仲裁员表, 找出可以选择的仲裁员
    auto arb_table = arbitrators( get_self(), get_self().value );
    for (auto iter = arb_table.begin(); iter != arb_table.end(); iter++)
    {
        auto chosen = std::find(chosen_arbitrators.begin(), chosen_arbitrators.end(), iter->account);
        if (chosen == chosen_arbitrators.end() && !iter->is_malicious) {
            chosen_from_arbitrators.push_back(iter->account);
        }
    }

    // 专业仲裁第一轮, 人数指数倍增加, 2^1+1, 2^2+1, 2^3+1, 2^num+1, num为冲裁的轮次
    // 专业仲裁人数不够, 走大众仲裁
    // 人数不够情况有两种: 1.最开始不够; 2.随机选择过程中不够;
    // 大众仲裁人数为专业仲裁2倍, 启动过程一样, 阶段重新设置, 大众冲裁结束不能再申诉, 进入大众仲裁需要重新抵押, 抵押变为2倍
    // 增加抵押金, 记录方法为大众仲裁, 仲裁个数为2倍, 增加抵押金
    if(chosen_from_arbitrators.size() < arbi_to_chose) {
        if (arbicaseapp_iter->arbi_method ==  arbi_method_type::multiple_rounds) {
            // 专业仲裁人数不够, 走大众仲裁
            std::vector<name> final_arbi;
            arbicaseapp_tb.modify(arbicaseapp_iter, get_self(), [&]( auto& p ) {
                p.arbi_step = arbi_step_type::arbi_public_choosing_arbitrator; //大众仲裁阶段开始
                p.arbi_method = arbi_method_type::public_arbitration; //仲裁案件变为大众仲裁
            } );
            // 修改本轮仲裁过程为大众仲裁
            arbiprocess_tb.modify( arbipro_iter, get_self(), [&]( auto& p ) {
                p.required_arbitrator = 2 * arbi_to_chose; // 该轮需要的仲裁员的个数
                p.arbi_method = arbi_method_type::public_arbitration; //本轮变为大众仲裁
            } );
            // 挑选专业仲裁员过程中人数不够，进入大众仲裁，开始随机挑选大众仲裁
            random_chose_arbitrator(arbitration_id, process_id, arbicaseapp_iter->service_id, arbi_to_chose * 2);
            return final_arbi;
        } else if (arbicaseapp_iter->arbi_method == arbi_method_type::public_arbitration) {
            // 大众仲裁人数不够, TODO
        }
    }

    // 挑选 `arbi_to_chose` 个仲裁员
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
            auto chosen = std::find(chosen_from_arbitrators.begin(), chosen_from_arbitrators.end(), arbitrator);
            chosen_from_arbitrators.erase(chosen);
            // 保存被选择的仲裁员
            arbiprocess_tb.modify( arbipro_iter, get_self(), [&]( auto& p ) {
                p.add_random_arbitrator(arbitrator);
                if (arbitrators_set.size() == arbi_to_chose) {
                    // 刚好选择完毕仲裁员, 那么设置这些仲裁员需要在指定时间内应诉的时间
                    p.arbiresp_deadline = time_point_sec(now() + arbiresp_deadline);
                }
            } );
        }
    }

    std::vector<name> final_arbi(arbitrators_set.begin(), arbitrators_set.end());
    return final_arbi;
}

/**
 * 新增仲裁结果表
 */
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

/**
 * 更新仲裁正确率
 */
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

uint128_t bos_oracle::make_deferred_id(uint64_t arbitration_id, uint8_t timer_type) 
{
    return (uint128_t(arbitration_id) << 64) | timer_type;
}

void bos_oracle::timeout_deferred(uint64_t arbitration_id, uint64_t process_id, uint8_t timer_type, uint64_t time_length) 
{
    transaction t;
    t.actions.emplace_back(permission_level{_self, active_permission}, _self,
                           "timertimeout"_n,
                           std::make_tuple(arbitration_id, process_id, timer_type));
    t.delay_sec = time_length;
    uint128_t deferred_id = make_deferred_id(arbitration_id, timer_type);
    cancel_deferred(deferred_id);
    t.send(deferred_id, get_self());
}

void bos_oracle::upload_result_timeout_deferred(name arbitrator, uint64_t arbitration_id, uint64_t process_id,
    uint8_t timer_type, uint64_t time_length) 
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

void bos_oracle::uploaddefer(name arbitrator, uint64_t arbitration_id, uint64_t process_id, uint8_t timer_type)
{
    if (timer_type == arbitration_timer_type::upload_result_timeout || 
        timer_type == arbitration_timer_type::public_upload_result_timeout ) {
        handle_upload_result(arbitrator, arbitration_id, process_id);
    }
}

void bos_oracle::timertimeout(uint64_t arbitration_id, uint64_t process_id, uint8_t timer_type)
{
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbicaseapp_iter = arbicaseapp_tb.find(arbitration_id);
    check(arbicaseapp_iter != arbicaseapp_tb.end(), "Can not find such arbitration.");
    
    auto arbiprocess_tb = arbitration_processs(get_self(), get_self().value);
    switch(timer_type)
    {
        case arbitration_timer_type::appeal_timeout: {
            if(arbicaseapp_iter->arbi_step == arbi_step_type::arbi_init) {
                handle_arbitration(arbitration_id);
                handle_arbitration_result(arbitration_id);
            }
            break;
        }
        case arbitration_timer_type::reappeal_timeout: { // 有人再次申诉
            if(arbicaseapp_iter->arbi_step == arbi_step_type::arbi_reappeal) {
                // 启动下一轮, 随机选择仲裁员
                auto arbiprocess_iter = arbiprocess_tb.find(process_id);
                check(arbicaseapp_iter != arbicaseapp_tb.end(), "Can not find such process.");
                random_chose_arbitrator(arbitration_id, process_id, arbicaseapp_iter->service_id, arbiprocess_iter->required_arbitrator);
            } else {
                // 没人再次申诉, 记录最后一次仲裁过程
                arbicaseapp_tb.modify(arbicaseapp_iter, get_self(), [&]( auto& p ) {
                    p.last_process_id = process_id;
                });
            }
            break;
        }
        case arbitration_timer_type::resp_appeal_timeout: {
            // 如果冲裁案件状态仍然为初始化状态, 说明没有数据提供者应诉, 直接处理仲裁结果
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
            auto arbiprocess_iter = arbiprocess_tb.find(process_id);
            check(arbicaseapp_iter != arbicaseapp_tb.end(), "Can not find such process.");

            // 如果状态为还在选择仲裁员, 那么继续选择仲裁员
            if(arbicaseapp_iter->arbi_step == arbi_step_type::arbi_choosing_arbitrator) {
                random_chose_arbitrator(arbitration_id, process_id, arbicaseapp_iter->service_id, arbiprocess_iter->required_arbitrator);
            }
            break;
        }
        case arbitration_timer_type::public_resp_arbitrate_timeout: {
            auto arbiprocess_iter = arbiprocess_tb.find(process_id);
            check(arbicaseapp_iter != arbicaseapp_tb.end(), "Can not find such process.");

            // 如果状态为还在选择仲裁员, 那么继续选择仲裁员
            if(arbicaseapp_iter->arbi_step == arbi_step_type::arbi_public_choosing_arbitrator) {
                random_chose_arbitrator(arbitration_id, process_id, arbicaseapp_iter->service_id, arbiprocess_iter->required_arbitrator);
            }
            break;
        }
        case arbitration_timer_type::upload_result_timeout: {
            break;
        }
        case arbitration_timer_type::public_upload_result_timeout: {
            break;
        }
    }
}

/**
 * 处理仲裁案件结果
 */
void bos_oracle::handle_arbitration(uint64_t arbitration_id) {
    // 找到这个仲裁
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbicaseapp_iter = arbicaseapp_tb.find(arbitration_id);
    check(arbicaseapp_iter != arbicaseapp_tb.end(), "Can not find such arbitration.");

    // 找到最后一个仲裁过程
    auto arbiprocess_tb = arbitration_processs(get_self(), get_self().value);
    auto arbiprocess_iter = arbiprocess_tb.find(arbicaseapp_iter->last_process_id);
    check(arbiprocess_iter != arbiprocess_tb.end(), "Can not find such arbitration process.");

    // 更新仲裁案件结果
    arbicaseapp_tb.modify(arbicaseapp_iter, get_self(), [&]( auto& p ) {
        p.arbi_step = arbi_step_type::arbi_end;
        p.final_result = arbiprocess_iter->arbitration_result;
        if(p.is_provider && p.final_result) {
          p.final_winner = final_winner_type::provider;
        } else {
          p.final_winner = final_winner_type::consumer;
        }
    } );

    // Calculate arbitration correction.
    update_arbitration_correcction(arbitration_id);
}

/**
 * 处理仲裁案件最终结果
 */
void bos_oracle::handle_arbitration_result(uint64_t arbitration_id) {
    auto arbicaseapp_tb = arbicaseapps(get_self(), get_self().value);
    auto arbi_iter = arbicaseapp_tb.find(arbitration_id);
    check(arbi_iter != arbicaseapp_tb.end(), "Can not find such arbitration.");

    const uint64_t final_result_provider = 0;
    bool is_provider = arbi_iter->final_winner != final_result_provider;
    std::tuple<std::vector<name>, asset> slash_stake_accounts =
        get_balances(arbitration_id, is_provider);
    int64_t slash_amount = std::get<1>(slash_stake_accounts).amount;
    
    // if final winner is not provider then slash  all service providers' stakes 
    if (is_provider) {
      uint64_t service_id = arbi_iter->service_id;

      std::tuple<std::vector<name>, asset> service_stakes =
          get_provider_service_stakes(service_id);
      const std::vector<name> &accounts = std::get<0>(service_stakes);
      const asset &amount = std::get<1>(service_stakes);
      slash_amount += amount.amount;//  add slash service stake from all service providers
      slash_service_stake(service_id, accounts, amount);
    }

    double dividend_percent = 0.8;
    double slash_amount_dividend_part = slash_amount * dividend_percent;
    double slash_amount_fee_part = slash_amount * (1 - dividend_percent);
    check(slash_amount_dividend_part > 0 && slash_amount_fee_part > 0, "");

    // award stake accounts
    std::tuple<std::vector<name>, asset> award_stake_accounts =
        get_balances(arbitration_id, !is_provider);

    // slash all losers' arbitration stake 
    slash_arbitration_stake(arbitration_id, std::get<0>(slash_stake_accounts));

    // pay all winners' award
    pay_arbitration_award(arbitration_id, std::get<0>(award_stake_accounts),
                          slash_amount_dividend_part);

    // pay all arbitrators' arbitration fee
    pay_arbitration_fee(arbitration_id, arbi_iter->arbitrators,
                        slash_amount_fee_part);
}

/**
 * @brief 
 * 
 * @param arbitration_id 
 */
void bos_oracle::handle_rearbitration_result(uint64_t arbitration_id) {
    auto arbicaseapp_tb = arbicaseapps( get_self(), get_self().value );
    auto arbicaseapp_iter = arbicaseapp_tb.find(arbitration_id);
    check(arbicaseapp_iter != arbicaseapp_tb.end(), "Can not find such arbitration.");

    if(arbicaseapp_iter->is_provider) {
      arbicaseapp_tb.modify(arbicaseapp_iter, get_self(), [&]( auto& p ) {
        p.arbi_step = arbi_step_type::arbi_reappeal_timeout_end;
        p.final_winner = final_winner_type::provider;
      } );
    }

    handle_arbitration_result(arbitration_id);
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
void bos_oracle::add_balance(name owner, asset value, uint64_t arbitration_id, bool is_provider) {
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
  while (type_itr != upper) {
    if (type_itr->is_provider == is_provider) {
      accounts.push_back(type_itr->account);
      stakes += type_itr->balance;
    }

    type_itr++;
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
  asset stakes= asset(0, core_symbol());

  for (const auto &p : provisionstable) {
    if (p.status == provision_status::provision_reg ) {
      providers.push_back(p.account);
      stakes +=p.amount;
    }
  }

  return std::make_tuple(providers,stakes);
}

/**
 * @brief 
 * 
 * @param service_id 
 * @param slash_accounts 
 * @param amount 
 */
void bos_oracle::slash_service_stake(uint64_t service_id, const std::vector<name>& slash_accounts, const asset &amount ) {
 
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
    check(provision_itr != provisionstable.end(),
          "account does not subscribe services");
    check(provider_itr->total_stake_amount >= provision_itr->amount,
          "account does not subscribe services");

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
void bos_oracle::slash_arbitration_stake(uint64_t arbitration_id, std::vector<name>& slash_accounts) {
  arbitration_stake_accounts stake_acnts(_self, arbitration_id);
  for (auto &a : slash_accounts) {
    auto acc = stake_acnts.find(a.value);
    check(acc != stake_acnts.end(), "");

    stake_acnts.modify(acc, same_payer,
                       [&](auto &a) { a.balance = asset(0, core_symbol()); });
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
 uint64_t award_size = award_accounts.size();
  check(award_size > 0, "");
  int64_t average_award_amount =
      static_cast<int64_t>(dividend_amount / award_size);
  if (average_award_amount > 0) {
       
    for (auto &a : award_accounts) {
      add_income( a ,asset(average_award_amount,core_symbol()));
    }
  }

}

void bos_oracle::add_income(name account ,asset quantity)
{
    arbitration_income_accounts stake_acnts(_self, account.value);
      auto acc = stake_acnts.find(quantity.symbol.code().raw());
      if(acc == stake_acnts.end())
      {
    stake_acnts.emplace(_self, [&](auto &a) {
        a.income = quantity;
        a.claim = asset(0, quantity.symbol);
      });
      }
      else
      {
 stake_acnts.modify(acc, same_payer, [&](auto &a) {
        a.income += quantity;
      });
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
  auto abr_table = arbitrators( get_self(), get_self().value );

  for(auto& a: fee_accounts)
  {
    add_income( a ,asset(static_cast<int64_t>(fee_amount),core_symbol()));
  }
}

 void bos_oracle::stake_arbitration(uint64_t id, name account, asset amount,uint64_t round_count,bool is_provider,string memo)
 {
       stake_records staketable(_self,round_count);

     auto stake_itr = staketable.find(id);
      if(stake_itr == staketable.end())
      {
    staketable.emplace(_self, [&](auto &a) {
        a.record_id = id;
        a.account = account;
        a.amount = amount;
        a.stake_time = time_point_sec(now());
      });
      }
      else
      {
          print("repeat id stake aribration",id,account,amount.amount,round_count,memo);
            staketable.modify(stake_itr, same_payer, [&](auto &a) {
        a.amount += amount;
      });
      }

        add_balance(account,amount,id,is_provider);

 }

 void bos_oracle::check_stake_arbitration(uint64_t id, name account,uint64_t round_count)
 {
     stake_records staketable(_self,round_count);

     auto stake_itr = staketable.find(id);
      check(stake_itr != staketable.end(),"no stake");
 }

 void bos_oracle::unstakearbi(uint64_t appeal_id, name account,
                              asset amount, std::string memo) {
  require_auth(account);

  check (amount.amount > 0,"stake amount could not be  equal to zero") ;

  arbitration_stake_accounts stake_acnts(_self, appeal_id);

    auto acc = stake_acnts.find(account.value);
    check(acc != stake_acnts.end(), "no account found");
    check(acc->balance.amount >= amount.amount, "overdrawn stake balance");

    stake_acnts.modify(acc, same_payer,
                       [&](auto &a) { a.balance -= amount; });
  

  transfer(account, provider_account, amount, "");
  
}

void bos_oracle::claimarbi(name account, name receive_account) {
  require_auth(account);
  arbitration_income_accounts incometable(_self, account.value);

  auto income_itr = incometable.find(core_symbol().code().raw());
  check(income_itr != incometable.end(), "no income by account");

    asset new_income = income_itr->income - income_itr->claim;
  check(new_income.amount > 0, "no income ");

  incometable.modify(income_itr, same_payer,
                       [&](auto &p) { p.claim += new_income; });

  transfer(consumer_account, receive_account, new_income, "claim arbi");
  
}