#pragma once
/*

  bos_oracle

  Author: Guillaume "Gnome" Babin-Tremblay - EOS Titan

  Website: https://eostitan.com
  Email: guillaume@eostitan.com

  Github: https://github.com/eostitan/bos_oracle/

  Published under MIT License

*/

#include "bos.oracle/tables/arbitration.hpp"
#include "bos.oracle/tables/consumer.hpp"
#include "bos.oracle/tables/oracle.hpp"
#include "bos.oracle/tables/oraclize.hpp"
#include "bos.oracle/tables/provider.hpp"
#include "bos.oracle/tables/riskcontrol.hpp"
#include "bos.oracle/tables/singletons.hpp"
#include <eosiolib/chain.h>
#include <eosiolib/eosio.hpp>
#include <eosiolib/fixedpoint.hpp>

using namespace eosio;

class [[eosio::contract("bos.oracle")]] bos_oracle : public eosio::contract {
public:
  static constexpr eosio::name provider_account{"provider.bos"_n};
  static constexpr eosio::name consumer_account{"consumer.bos"_n};
  static constexpr eosio::name riskctrl_account{"riskctrl.bos"_n};
  static constexpr eosio::name token_account{"eosio.token"_n};
  static constexpr eosio::name active_permission{"active"_n};
  static constexpr symbol _core_symbol = symbol(symbol_code("EOS"), 4);
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
      uint64_t service_id, name account, asset stake_amount,
      asset service_price, uint64_t fee_type, std::string data_format,
      uint64_t data_type, std::string criteria, uint64_t acceptance,
      std::string declaration, uint64_t injection_method,
      time_point_sec duration, uint64_t provider_limit, uint64_t update_cycle,
      time_point_sec update_start_time);

  [[eosio::action]] void unregservice(uint64_t service_id,
                                      std::string signature, name account,
                                      uint64_t is_suspense);

  [[eosio::action]] void execaction(uint64_t service_id, uint64_t action_type);

  [[eosio::action]] void stakeamount(uint64_t service_id, uint64_t provider_id,
                                     name account, std::string signature,
                                     asset stake_amount);

  [[eosio::action]] void pushdata(uint64_t service_id, name provider,
                                  name contract_account, name action_name,
                                  uint64_t data_json, uint64_t request_id);
  [[eosio::action]] void multipush(uint64_t service_id, name provider,
                                   uint64_t data_json, bool is_request);
  [[eosio::action]] void addfeetype(uint64_t service_id,
                                    std::vector<uint8_t> fee_types,
                                    std::vector<asset> service_prices);

  [[eosio::action]] void claim(name account, name receive_account);

  using regiservice_action =
      eosio::action_wrapper<"regservice"_n, &bos_oracle::regservice>;
  using unregister_action =
      eosio::action_wrapper<"unregservice"_n, &bos_oracle::unregservice>;
  using execaction_action =
      eosio::action_wrapper<"execaction"_n, &bos_oracle::execaction>;
  using stakeamount_action =
      eosio::action_wrapper<"stakeamount"_n, &bos_oracle::stakeamount>;
  using pushdata_action =
      eosio::action_wrapper<"pushdata"_n, &bos_oracle::pushdata>;

  using multipush_action =
      eosio::action_wrapper<"multipush"_n, &bos_oracle::multipush>;

  using addfeetype_action =
      eosio::action_wrapper<"addfeetype"_n, &bos_oracle::addfeetype>;

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

  [[eosio::action]] void payservice(uint64_t service_id, name contract_account,
                                    name action_name, name account,
                                    asset amount, std::string memo);
  [[eosio::action]] void confirmpay(uint64_t service_id, name contract_account,
                                    name action_name, asset amount);
  using subscribe_action =
      eosio::action_wrapper<"subscribe"_n, &bos_oracle::subscribe>;
  using requestdata_action =
      eosio::action_wrapper<"requestdata"_n, &bos_oracle::requestdata>;
  using payservice_action =
      eosio::action_wrapper<"payservice"_n, &bos_oracle::payservice>;

  using confirmpay_action =
      eosio::action_wrapper<"confirmpay"_n, &bos_oracle::confirmpay>;
  ///
  ///
  /// bos.consumer end
  /// bos.riskctrl begin
  ///
  ///
  [[eosio::action]] void deposit(name from, name to, asset quantity,
                                 string memo, bool is_notify);
  [[eosio::action]] void withdraw(name from, name to, asset quantity,
                                  string memo);

  using deposit_action =
      eosio::action_wrapper<"deposit"_n, &bos_oracle::deposit>;
  using withdraw_action =
      eosio::action_wrapper<"withdraw"_n, &bos_oracle::withdraw>;
  ///
  ///
  /// bos.riskctrl end
private:
  oracle_fee_singleton _oracle_fee;
  bos_oracle_fee _fee_state;

  // provider
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

  /// consumer
  std::vector<std::tuple<name, name>> get_subscription_list(
      uint64_t service_id);
  std::vector<std::tuple<name, name, uint64_t>> get_request_list(
      uint64_t service_id, uint64_t request_id);
  std::tuple<uint64_t, uint64_t> get_consumption(
      uint64_t service_id);
  /// risk control
  void transfer(name from, name to, asset quantity, string memo);
  void add_freeze_delay(uint64_t service_id, name account,
                        time_point_sec start_time, time_point_sec duration,
                        asset amount, uint64_t status, uint64_t type);
  uint64_t add_guarantee(uint64_t service_id, name account,
                         time_point_sec start_time, time_point_sec duration,
                         asset amount, uint64_t status);
  void sub_balance(name owner, asset value);
  void add_balance(name owner, asset value, name ram_payer);

  /// common
  symbol core_symbol() const { return _core_symbol; };
};
