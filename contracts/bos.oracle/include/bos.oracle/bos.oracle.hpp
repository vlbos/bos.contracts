#pragma once
/*

  bos_oracle

 

*/

#include "bos.oracle/tables/arbitration.hpp"
#include "bos.oracle/tables/consumer.hpp"
#include "bos.oracle/tables/oracle.hpp"
#include "bos.oracle/tables/oracle_api.hpp"
#include "bos.oracle/tables/oraclize.hpp"
#include "bos.oracle/tables/provider.hpp"
#include "bos.oracle/tables/riskcontrol.hpp"
#include "bos.oracle/tables/singletons.hpp"
#include "bos.oracle/tables/arbitration.hpp"

#include <eosio/eosio.hpp>
using namespace eosio;

class [[eosio::contract("bos.oracle")]] bos_oracle : public eosio::contract {
public:
  static constexpr eosio::name provider_account{"provider.bos"_n};
  static constexpr eosio::name consumer_account{"consumer.bos"_n};
  static constexpr eosio::name riskctrl_account{"riskctrl.bos"_n};
  static constexpr eosio::name arbitrat_account{"arbitrat.bos"_n};
  static constexpr eosio::name token_account{"eosio.token"_n};
  static constexpr eosio::name active_permission{"active"_n};
  static constexpr symbol _core_symbol = symbol(symbol_code("EOS"), 4);
  static constexpr uint64_t arbi_process_time_limit = 3600;
  static constexpr uint64_t arbiresp_deadline_days= 1; // 仲裁员响应的截止时间 天
  static constexpr double default_arbitration_correct_rate = 0.6f;
  static time_point_sec current_time_point_sec();

  using contract::contract;
  bos_oracle(name receiver, name code, datastream<const char *> ds)
      : contract(receiver, code, ds), requests(_self, _self.value),
        token("boracletoken"_n), oraclizes_table(_self, _self.value),
        _oracle_fee(_self, _self.value) {
    _fee_state = _oracle_fee.exists() ? _oracle_fee.get() : bos_oracle_fee{};
  }
  ~bos_oracle() { _oracle_fee.set(_fee_state, _self); }
  // Check if calling account is a qualified oracle
  bool check_oracle(const name owner);
  // Ensure account cannot push data more often than every 60 seconds
  void check_last_push(const name owner);

  // Push oracle message on top of queue, pop oldest element if queue size is
  // larger than X
  void update_eosusd_oracle(const name owner, const uint64_t value);

  // Write datapoint
  [[eosio::action]] void write(const name owner, const uint64_t value);

  // Update oracles list
  [[eosio::action]] void setoracles(const std::vector<name> &oracleslist);

  // Clear all data
  [[eosio::action]] void clear();

  /// bos.oraclize begin
  ///
  ///
  request_table requests;
  name token;
  oracle_identities oraclizes_table;

  // oraclize(name receiver, name code, datastream<const char*> ds ) :
  // contract( receiver,  code, ds ), requests(_self, _self.value),
  // token("boracletoken"_n), oracles_table(_self, _self.value) {}

  [[eosio::action]] void addoracle(name oracle);

  [[eosio::action]] void removeoracle(name oracle);

  // @abi action
  [[eosio::action]] void ask(name administrator, name contract, string task,
                             uint32_t update_each, string memo, bytes args);

  // @abi action
  [[eosio::action]] void disable(name administrator, name contract, string task,
                                 string memo);

  // @abi action
  [[eosio::action]] void once(name administrator, name contract, string task,
                              string memo, bytes args);

  // @abi action
  [[eosio::action]] void push(name oracle, name contract, string task,
                              string memo, bytes data);

  void set(const request &value, name bill_to_account);

  using addoracle_action =
      eosio::action_wrapper<"addoracle"_n, &bos_oracle::addoracle>;
  using removeoracle_action =
      eosio::action_wrapper<"removeoracle"_n, &bos_oracle::removeoracle>;
  using ask_action = eosio::action_wrapper<"ask"_n, &bos_oracle::ask>;
  using disable_action =
      eosio::action_wrapper<"disable"_n, &bos_oracle::disable>;
  using once_action = eosio::action_wrapper<"once"_n, &bos_oracle::once>;
  using push_action = eosio::action_wrapper<"push"_n, &bos_oracle::push>;
  ///
  ///
  /// bos.oraclize end
  /// bos.provider begin
  ///
  ///
  //

  [[eosio::action]] void regservice(
      uint64_t service_id, name account, asset amount,
      asset service_price, uint64_t fee_type, std::string data_format,
      uint64_t data_type, std::string criteria, uint64_t acceptance,
      std::string declaration, uint64_t injection_method, uint64_t duration,
      uint64_t provider_limit, uint64_t update_cycle,
      time_point_sec update_start_time);

  [[eosio::action]] void stakeasset(uint64_t service_id, name account,
                                     asset amount, std::string memo);

  [[eosio::action]] void unstakeasset(uint64_t service_id, name account,
                                     asset amount, std::string memo);

  [[eosio::action]] void addfeetypes(uint64_t service_id,
                                     std::vector<uint8_t> fee_types,
                                     std::vector<asset> service_prices);
  [[eosio::action]] void addfeetype(uint64_t service_id, uint8_t fee_type,
                                    asset service_price);

 

  [[eosio::action]] void pushdata(uint64_t service_id, name provider,
                                  uint64_t update_number,
                                  uint64_t request_id, string data_json);
  [[eosio::action]] void innerpush(uint64_t service_id, name provider,
                                  name contract_account, name action_name,
                                  uint64_t request_id, string data_json);
  
  [[eosio::action]] void innerpublish(uint64_t service_id, name provider,
                          uint64_t update_number,
                          uint64_t request_id, string data_json);

  [[eosio::action]] void oraclepush(uint64_t service_id, uint64_t update_number,
                                     uint64_t request_id, string data_json,name contract_account);
  [[eosio::action]] void claim(name account, name receive_account);

  [[eosio::action]] void execaction(uint64_t service_id, uint64_t action_type);

  [[eosio::action]] void unregservice(uint64_t service_id, name account,
                                      uint64_t status);
  [[eosio::action]] void starttimer();

  using regservice_action =
      eosio::action_wrapper<"regservice"_n, &bos_oracle::regservice>;

  using unstakeasset_action =
      eosio::action_wrapper<"unstakeasset"_n, &bos_oracle::unstakeasset>;

  using addfeetypes_action =
      eosio::action_wrapper<"addfeetypes"_n, &bos_oracle::addfeetypes>;
  using addfeetype_action =
      eosio::action_wrapper<"addfeetype"_n, &bos_oracle::addfeetype>;

   using pushdata_action =
      eosio::action_wrapper<"pushdata"_n, &bos_oracle::pushdata>;
  using innerpush_action =
      eosio::action_wrapper<"innerpush"_n, &bos_oracle::innerpush>;

  using innerpublish_action =
      eosio::action_wrapper<"innerpublish"_n, &bos_oracle::innerpublish>;
  using unipublish_action =
      eosio::action_wrapper<"oraclepush"_n, &bos_oracle::oraclepush>;
  using claim_action =
      eosio::action_wrapper<"claim"_n, &bos_oracle::addfeetype>;

  using execaction_action =
      eosio::action_wrapper<"execaction"_n, &bos_oracle::execaction>;

  using unregister_action =
      eosio::action_wrapper<"unregservice"_n, &bos_oracle::unregservice>;
  
  using starttimer_action =
      eosio::action_wrapper<"starttimer"_n, &bos_oracle::starttimer>;
  
  ///
  ///
  /// bos.provider end
  /// bos.consumer begin
  ///
  ///

  [[eosio::action]] void subscribe(
      uint64_t service_id, name contract_account, name action_name,
      std::string publickey, name account, asset amount, std::string memo);

  [[eosio::action]] void requestdata(uint64_t service_id, name contract_account,
                                     name action_name, name requester,
                                     std::string request_content);

  [[eosio::action]] void payservice(uint64_t service_id, name contract_account, asset amount, std::string memo);
  using subscribe_action =
      eosio::action_wrapper<"subscribe"_n, &bos_oracle::subscribe>;
  using requestdata_action =
      eosio::action_wrapper<"requestdata"_n, &bos_oracle::requestdata>;
  using payservice_action =
      eosio::action_wrapper<"payservice"_n, &bos_oracle::payservice>;

 
  ///
  ///
  /// bos.consumer end
  /// bos.riskctrl begin
  ///
  ///

  
  /**
   * @brief 
   * 
   * @param from 
   * @param to 
   * @param quantity 
   * @param memo 
   * @param is_notify 
   */
  [[eosio::action]] void deposit( name from, name to,asset quantity, string memo, bool is_notify);
  [[eosio::action]] void withdraw(uint64_t service_id, name from, name to,
                                  asset quantity, string memo);




  using deposit_action =
      eosio::action_wrapper<"deposit"_n, &bos_oracle::deposit>;
  using withdraw_action =
      eosio::action_wrapper<"withdraw"_n, &bos_oracle::withdraw>;

  ///
  ///
  /// bos.riskctrl end

        // [[eosio::on_notify("eosio.token::transfer")]] 
      void on_transfer(name from, name to, asset quantity, std::string memo);
 /// bos.arbitration begin
  ///
  ///
    // [[eosio::action]] void regarbitrat( name account, public_key pubkey, uint8_t type, asset amount, std::string public_info );

    // [[eosio::action]] void complain( name applicant, uint64_t service_id, asset amount, std::string reason, uint8_t arbi_method , std::string evidence );

    // [[eosio::action]] void respcase( name respondent, uint64_t arbitration_id,asset amount, uint64_t process_id , std::string evidence );

    [[eosio::action]] void acceptarbi( name arbitrator,  uint64_t arbitration_id, uint64_t process_id );

    [[eosio::action]] void uploadeviden( name applicant, uint64_t arbitration_id, uint64_t process_id,std::string evidence );

    [[eosio::action]] void uploadresult( name arbitrator, uint64_t arbitration_id, uint64_t result, uint64_t process_id );

    // [[eosio::action]] void reappeal( name applicant, uint64_t arbitration_id, uint64_t service_id, uint64_t process_id, bool is_provider, asset amount, std::string reason , std::string evidence );
    
    // [[eosio::action]] void rerespcase( name respondent, uint64_t arbitration_id,asset amount, uint64_t process_id, std::string evidence );
    
    [[eosio::action]] void timertimeout(uint64_t arbitration_id, uint64_t process_id, uint8_t timer_type);

    [[eosio::action]] void uploaddefer(name arbitrator, uint64_t arbitration_id, uint64_t process_id, uint8_t timer_type);
    
    [[eosio::action]] void unstakearbi(uint64_t appeal_id, name account,
                                     asset amount, std::string memo);
    [[eosio::action]] void claimarbi(name account, name receive_account);
  /// 
  ///
  /// bos.arbitration end

private:
  oracle_fee_singleton _oracle_fee;
  bos_oracle_fee _fee_state;

  // provider
  void stake_asset(uint64_t service_id, name account,
                                     asset amount);
  void add_times(uint64_t service_id, name account, name contract_account,
                 name action_name, bool is_request);
  std::tuple<uint64_t, uint64_t, uint64_t, uint64_t> get_times(
      uint64_t service_id, name account);
  time_point_sec get_payment_time(uint64_t service_id, name contract_account,
                                  name action_name);
  uint8_t get_subscription_status(uint64_t service_id, name contract_account,
                                  name action_name);
  uint8_t get_service_status(uint64_t service_id);
  void fee_service(uint64_t service_id, name contract_account, name action_name,
                   uint8_t fee_type);
  asset get_price_by_fee_type(uint64_t service_id, uint8_t fee_type);
  uint64_t get_request_by_last_push(uint64_t service_id, name provider);

  std::vector<std::tuple<name, asset>> get_provider_list(uint64_t service_id);

  void freeze_asset(uint64_t service_id, name account, asset amount);
  uint64_t freeze_providers_amount(uint64_t service_id,
                                   const std::set<name> &available_providers,
                                   asset freeze_amount);

  void multipush(uint64_t service_id, name provider,
                                   string data_json, bool is_request);
                                   
  void multipublish(uint64_t service_id, uint64_t update_number,
                             uint64_t request_id,
                           string data_json);
  void publishdata(uint64_t service_id, name provider,
                          uint64_t update_number,
                          uint64_t request_id, string data_json);   

void start_timer();
void check_publish_services();
void check_publish_service(uint64_t service_id,uint64_t update_number,uint64_t request_id);
void save_publish_data(uint64_t service_id,uint64_t update_number,uint64_t request_id,string  value,name provider);
uint64_t get_provider_count(uint64_t service_id);


uint64_t get_publish_provider_count(uint64_t service_id,uint64_t update_number,uint64_t request_id);


string get_publish_data(uint64_t service_id,uint64_t update_number,uint64_t  provider_limit,uint64_t request_id);
std::vector<std::tuple<uint64_t,uint64_t,uint64_t>> get_publish_services_update_number();

  /// consumer
  void pay_service(uint64_t service_id, name contract_account, asset amount);
  std::vector<std::tuple<name, name>> get_subscription_list(
      uint64_t service_id);
  std::vector<std::tuple<name, name, uint64_t>> get_request_list(
      uint64_t service_id, uint64_t request_id);
  std::tuple<uint64_t, uint64_t> get_consumption(uint64_t service_id);

  /// risk control
  void transfer(name from, name to, asset quantity, string memo);
  void oracle_transfer(name from, name to, asset quantity, string memo,bool is_deferred);
  void call_deposit( name from, name to,
                         asset quantity,  bool is_notify);
  void add_freeze_delay(uint64_t service_id, name account,
                        time_point_sec start_time, uint64_t duration,
                        asset amount, uint64_t type);
  void add_freeze(uint64_t service_id, name account, time_point_sec start_time,
                  uint64_t duration, asset amount);
  void add_delay(uint64_t service_id, name account, time_point_sec start_time,
                 uint64_t duration, asset amount);

  uint64_t add_guarantee(uint64_t service_id, name account,
                         asset amount             ,        uint64_t duration);
  void sub_balance(name owner, asset value);
  void add_balance(name owner, asset value, name ram_payer);

  void add_freeze_log(uint64_t service_id, name account, asset amount);

  void add_freeze_stat(uint64_t service_id, name account, asset amount);
  std::tuple<asset, asset> get_freeze_stat(uint64_t service_id, name account);

  std::tuple<asset, asset> stat_freeze_amounts(uint64_t service_id,
                                               name account);

/// arbitration
    void _regarbitrat( name account, public_key pubkey, uint8_t type, asset amount, std::string public_info );
    void _complain( name applicant, uint64_t service_id, asset amount, std::string reason, uint8_t arbi_method, std::string evidence  );
    void _respcase( name respondent, uint64_t arbitration_id,asset amount, uint64_t process_id, std::string evidence  );
    void _reappeal( name applicant, uint64_t arbitration_id, uint64_t service_id, uint64_t process_id, bool is_provider, asset amount, std::string reason , std::string evidence );
    void _rerespcase( name respondent, uint64_t arbitration_id, asset amount,uint64_t process_id, std::string evidence );

    void handle_arbitration(uint64_t arbitration_id);
    void handle_arbitration_result(uint64_t arbitration_id);
    void start_arbitration(arbitrator_type arbitype, uint64_t arbitration_id, uint64_t service_id);
    vector<name> random_arbitrator(uint64_t arbitration_id, uint64_t process_id, uint64_t arbi_to_chose) ;
    void random_chose_arbitrator(uint64_t arbitration_id, uint64_t process_id, uint64_t service_id, uint64_t arbi_to_chose) ;
    void add_arbitration_result(name arbitrator, uint64_t arbitration_id, uint64_t result, uint64_t process_id);
    void update_arbitration_correcction(uint64_t arbitration_id);
    uint128_t make_deferred_id(uint64_t arbitration_id, uint8_t timer_type) ;
    void timeout_deferred(uint64_t arbitration_id, uint64_t process_id, uint8_t timer_type, uint64_t time_length) ;
    void upload_result_timeout_deferred(name arbitrator, uint64_t arbitration_id, uint64_t process_id, uint8_t timer_type, uint64_t time_length) ;
    void handle_upload_result(name arbitrator, uint64_t arbitration_id, uint64_t process_id);
    std::tuple<std::vector<name>, asset> get_balances(uint64_t arbitration_id, bool is_provider);
    std::tuple<std::vector<name>, asset> get_provider_service_stakes(uint64_t service_id);
    void slash_service_stake(uint64_t service_id, const std::vector<name>& slash_accounts, const asset &amount );
    void slash_arbitration_stake(uint64_t arbitration_id, std::vector<name>& slash_accounts);
    void pay_arbitration_award(uint64_t arbitration_id, std::vector<name>& award_accounts, double dividend_amount);
    void pay_arbitration_fee(uint64_t arbitration_id, const std::vector<name>& fee_accounts, double fee_amount);
    void handle_rearbitration_result(uint64_t arbitration_id);
    void sub_balance(name owner, asset value, uint64_t arbitration_id);
    void add_balance(name owner, asset value, uint64_t arbitration_id, bool is_provider);
    void stake_arbitration(uint64_t id, name account, asset amount,uint64_t round_count,bool is_provider,string memo);
    void check_stake_arbitration(uint64_t id, name account,uint64_t round_count);
   void add_income(name account ,asset quantity);

  /// common
  symbol core_symbol() const { return _core_symbol; };
};



