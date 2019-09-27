#include <boost/test/unit_test.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/testing/tester.hpp>
// #include "eosio.system_tester.hpp"
#include "contracts.hpp"
#include "test_symbol.hpp"

#include "Runtime/Runtime.h"

#include <fc/variant_object.hpp>
#include <vector>

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace fc;
using namespace std;

using mvo = fc::mutable_variant_object;

class bos_oracle_tester : public tester {
 public:
   bos_oracle_tester() {
      produce_blocks(2);
      create_accounts({N(eosio.token), N(eosio.ram), N(eosio.ramfee), N(eosio.stake), N(eosio.bpay), N(eosio.vpay), N(eosio.saving), N(eosio.names), N(eosio.rex)});
      create_accounts({N(alice), N(bob), N(carol), N(dapp), N(dappuser), N(oracle.bos), N(dappuser.bos), N(dapp.bos), N(provider.bos), N(consumer.bos), N(arbitrat.bos), N(riskctrl.bos)});
      produce_blocks(2);

      set_code(N(oracle.bos), contracts::oracle_wasm());
      set_abi(N(oracle.bos), contracts::oracle_abi().data());
      set_code(N(dappuser.bos), contracts::consumer_wasm());
      set_abi(N(dappuser.bos), contracts::consumer_abi().data());

      produce_blocks();

      const auto& accnt = control->db().get<account_object, by_name>(N(oracle.bos));
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_ser.set_abi(abi, abi_serializer_max_time);

      set_code(N(eosio.token), contracts::token_wasm());
      set_abi(N(eosio.token), contracts::token_abi().data());

      create_currency(N(eosio.token), config::system_account_name, core_sym::from_string("10000000000.0000"));
      issue(config::system_account_name, core_sym::from_string("1000000000.0000"));
      BOOST_REQUIRE_EQUAL(core_sym::from_string("1000000000.0000"), get_balance("eosio") + get_balance("eosio.ramfee") + get_balance("eosio.stake") + get_balance("eosio.ram"));

      create_currency(N(eosio.token), config::system_account_name, eosio::chain::asset::from_string("10000000000.0000 BQS"));
      issue(config::system_account_name, eosio::chain::asset::from_string("1000000000.0000 BQS"));

      set_code(config::system_account_name, contracts::system_wasm());
      set_abi(config::system_account_name, contracts::system_abi().data());
      base_tester::push_action(config::system_account_name, N(init), config::system_account_name, mutable_variant_object()("version", 0)("core", CORE_SYM_STR));
      produce_blocks();

      set_code(N(dapp.bos), contracts::token_wasm());
      set_abi(N(dapp.bos), contracts::token_abi().data());

      create_currency(N(dapp.bos), N(dappuser.bos), core_sym::from_string("10000000000.0000"));
      issuex(N(dapp.bos), N(dappuser.bos), core_sym::from_string("1000000000.0000"), N(dappuser.bos));
      produce_blocks();

      create_account_with_resources(N(alice1111111), N(eosio), core_sym::from_string("1.0000"), false);
      create_account_with_resources(N(bob111111111), N(eosio), core_sym::from_string("0.4500"), false);
      create_account_with_resources(N(carol1111111), N(eosio), core_sym::from_string("1.0000"), false);

      transfer("eosio", "alice1111111", ("3000.0000"), "eosio");
      transfer("eosio", "bob111111111", ("3000.0000"), "eosio");
      transfer("eosio", "carol1111111", ("3000.0000"), "eosio");
      transfer("eosio", "alice", ("3000.0000"), "eosio");
      transfer("eosio", "bob", ("3000.0000"), "eosio");
      transfer("eosio", "carol", ("3000.0000"), "eosio");
      transfer("eosio", "dappuser.bos", ("3000.0000"), "eosio");
      transfer("eosio", "dappuser", ("3000.0000"), "eosio");
      transfer("eosio", "dapp", ("3000.0000"), "eosio");

      transferex(N(eosio.token), "eosio", "dappuser.bos", ("1000000000.0000 BQS"), "eosio");

      std::vector<string> accounts_prefix = {"provider", "consumer", "appellants", "arbitrators"};
      for (auto& a : accounts_prefix) {
         for (int j = 1; j <= 5; ++j) {
            std::string acc_name = a + std::string(12 - a.size(), std::to_string(j)[0]);
            create_account_with_resources(name(acc_name.c_str()), N(eosio), core_sym::from_string("200.0000"), false);
            produce_blocks(1);
            transfer("eosio", acc_name.c_str(), ("20000.0000"), "eosio");
         }
      }

      for (int i = 1; i <= 5; ++i) {
         for (int j = 1; j <= 5; ++j) {
            std::string arbi_name = "arbitrator" + std::to_string(i) + std::to_string(j);
            create_account_with_resources(name(arbi_name.c_str()), N(eosio), core_sym::from_string("200.0000"), false);
            produce_blocks(1);
            transfer("eosio", arbi_name.c_str(), ("20000.0000"), "eosio");
         }
      }

      for (int i = 1; i <= 5; ++i) {
         std::string con_name = "conconsumer" + std::to_string(i);
         create_account_with_resources(name(con_name.c_str()), N(eosio), core_sym::from_string("200.0000"), false);
         produce_blocks(1);
         transfer("eosio", con_name.c_str(), ("20000.0000"), "eosio");
         produce_blocks(1);
         name consumer = name(con_name.c_str());
         set_code(consumer, contracts::consumer_wasm());
         set_abi(consumer, contracts::consumer_abi().data());
         produce_blocks(1);
      }
   }

   transaction_trace_ptr create_account_with_resources(account_name a, account_name creator, asset ramfunds, bool multisig, asset net = core_sym::from_string("10.0000"),
                                                       asset cpu = core_sym::from_string("10.0000")) {
      signed_transaction trx;
      set_transaction_headers(trx);

      authority owner_auth;
      if (multisig) {
         // multisig between account's owner key and creators active permission
         owner_auth = authority(2, {key_weight{get_public_key(a, "owner"), 1}}, {permission_level_weight{{creator, config::active_name}, 1}});
      } else {
         owner_auth = authority(get_public_key(a, "owner"));
      }

      trx.actions.emplace_back(vector<permission_level>{{creator, config::active_name}},
                               newaccount{.creator = creator, .name = a, .owner = owner_auth, .active = authority(get_public_key(a, "active"))});

      trx.actions.emplace_back(get_action(N(eosio), N(buyram), vector<permission_level>{{creator, config::active_name}}, mvo()("payer", creator)("receiver", a)("quant", ramfunds)));

      trx.actions.emplace_back(get_action(N(eosio), N(delegatebw), vector<permission_level>{{creator, config::active_name}},
                                          mvo()("from", creator)("receiver", a)("stake_net_quantity", net)("stake_cpu_quantity", cpu)("transfer", 0)));

      set_transaction_headers(trx);
      trx.sign(get_private_key(creator, "active"), control->get_chain_id());
      return push_transaction(trx);
   }
   void create_currency(name contract, name manager, asset maxsupply) {
      auto act = mutable_variant_object()("issuer", manager)("maximum_supply", maxsupply);

      base_tester::push_action(contract, N(create), contract, act);
   }
   void issuex(name contract, name to, const asset& amount, name manager = config::system_account_name) {
      base_tester::push_action(contract, N(issue), manager, mutable_variant_object()("to", to)("quantity", amount)("memo", ""));
   }

   void issue(name to, const asset& amount, name manager = config::system_account_name) {
      base_tester::push_action(N(eosio.token), N(issue), manager, mutable_variant_object()("to", to)("quantity", amount)("memo", ""));
   }
   void transfer(name from, name to, const string& amount, name manager = config::system_account_name, const std::string& memo = "") {
      base_tester::push_action(N(eosio.token), N(transfer), manager, mutable_variant_object()("from", from)("to", to)("quantity", core_sym::from_string(amount))("memo", memo));
   }

   void transferex(name contract, name from, name to, const string& amount, name manager = config::system_account_name, const std::string& memo = "") {
      base_tester::push_action(contract, N(transfer), manager, mutable_variant_object()("from", from)("to", to)("quantity", eosio::chain::asset::from_string(amount))("memo", memo));
   }

   asset get_balance(const account_name& act) {
      // return get_currency_balance( config::system_account_name, symbol(CORE_SYMBOL), act );
      // temporary code. current get_currency_balancy uses table name N(accounts) from currency.h
      // generic_currency table name is N(account).
      const auto& db = control->db();
      const auto* tbl = db.find<table_id_object, by_code_scope_table>(boost::make_tuple(N(eosio.token), act, N(accounts)));
      share_type result = 0;

      // the balance is implied to be 0 if either the table or row does not exist
      if (tbl) {
         const auto* obj = db.find<key_value_object, by_scope_primary>(boost::make_tuple(tbl->id, symbol(CORE_SYM).to_symbol_code()));
         if (obj) {
            // balance is the first field in the serialization
            fc::datastream<const char*> ds(obj->value.data(), obj->value.size());
            fc::raw::unpack(ds, result);
         }
      }
      return asset(result, symbol(CORE_SYM));
   }

   /// dappuser.bos
   void fetchdata(uint64_t service_id, uint64_t update_number, uint64_t request_id) {
      base_tester::push_action(N(dappuser.bos), N(fetchdata), N(dappuser.bos),
                               mutable_variant_object()("oracle", "oracle.bos")("service_id", service_id)("update_number", update_number)("request_id", request_id));
   }
   void consumer_transfer(name from, name to, asset quantity, string memo) {
      base_tester::push_action(N(dappuser.bos), N(transfer), N(dappuser.bos), mutable_variant_object()("from", from)("to", to)("quantity", quantity)("memo", memo));
   }

   void consumer_rollback(name who, asset value, string memo) {
      base_tester::push_action(N(dappuser.bos), N(dream), N(dappuser.bos), mutable_variant_object()("who", who)("value", value)("memo", memo));
   }

   action_result push_action(const account_name& signer, const action_name& name, const variant_object& data) {
      //       push_permission_update_auth_action(N(provider.bos));
      // push_permission_update_auth_action(N(consumer.bos));
      // push_permission_update_auth_action(N(arbitrat.bos));
      // push_permission_update_auth_action(N(riskctrl.bos));

      string action_type_name = abi_ser.get_action_type(name);

      action act;
      act.account = N(oracle.bos);
      act.name = name;
      act.data = abi_ser.variant_to_binary(action_type_name, data, abi_serializer_max_time);

      return base_tester::push_action(std::move(act), uint64_t(signer));
   }

   auto push_permission_update_auth_action(const account_name& signer) {
      auto auth = authority(eosio::testing::base_tester::get_public_key(signer, "active"));
      auth.accounts.push_back(permission_level_weight{{N(oracle.bos), config::eosio_code_name}, 1});

      return base_tester::push_action(N(eosio), N(updateauth), signer, mvo()("account", signer)("permission", "active")("parent", "owner")("auth", auth));
   }

   // provider
   fc::variant get_data_service(const uint64_t& service_id) {
      vector<char> data = get_row_by_account(N(oracle.bos), N(oracle.bos), N(dataservices), service_id);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("data_service", data, abi_serializer_max_time);
   }

   fc::variant get_data_service_fee(const uint64_t& service_id, const uint8_t& fee_type) {
      vector<char> data = get_row_by_account(N(oracle.bos), service_id, N(servicefees), fee_type);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("data_service_fee", data, abi_serializer_max_time);
   }

   fc::variant get_data_provider(const name& account) {
      vector<char> data = get_row_by_account(N(oracle.bos), N(oracle.bos), N(providers), account);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("data_provider", data, abi_serializer_max_time);
   }

   uint64_t get_oracle_id(const uint8_t& id_type) {
      vector<char> data = get_row_by_account(N(oracle.bos), N(oracle.bos), N(oracleids), id_type);
      return data.empty() ? 0 : abi_ser.binary_to_variant("oracle_id", data, abi_serializer_max_time)["id"].template as<uint64_t>();
   }

   fc::variant get_data_service_provision(const uint64_t& service_id, const name& account) {
      vector<char> data = get_row_by_account(N(oracle.bos), service_id, N(svcprovision), account);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("data_service_provision", data, abi_serializer_max_time);
   }

   fc::variant get_svc_provision_cancel_apply(const uint64_t& service_id, const name& provider) {
      vector<char> data = get_row_by_account(N(oracle.bos), service_id, N(cancelapplys), provider);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("svc_provision_cancel_apply", data, abi_serializer_max_time);
   }

   fc::variant get_data_service_provision_log(const uint64_t& service_id, const uint64_t& log_id) {
      vector<char> data = get_row_by_account(N(oracle.bos), service_id, N(provisionlog), log_id);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("data_service_provision_log", data, abi_serializer_max_time);
   }

   fc::variant get_push_record(const uint64_t& service_id) {
      vector<char> data = get_row_by_account(N(oracle.bos), N(oracle.bos), N(pushrecords), service_id);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("push_record", data, abi_serializer_max_time);
   }

   fc::variant get_provider_push_record(const uint64_t& service_id, const name& account) {
      vector<char> data = get_row_by_account(N(oracle.bos), service_id, N(ppushrecords), account);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("provider_push_record", data, abi_serializer_max_time);
   }

   fc::variant get_data_service_subscription(const uint64_t& service_id, const name& contract_account) {
      vector<char> data = get_row_by_account(N(oracle.bos), service_id, N(subscription), contract_account);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("data_service_subscription", data, abi_serializer_max_time);
   }

   fc::variant get_data_service_request(const uint64_t& service_id, const uint64_t& request_id) {
      vector<char> data = get_row_by_account(N(oracle.bos), service_id, N(request), request_id);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("data_service_request", data, abi_serializer_max_time);
   }

   fc::variant get_service_consumptions(const uint64_t& service_id) {
      vector<char> data = get_row_by_account(N(oracle.bos), service_id, N(consumptions), service_id);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("service_consumptions", data, abi_serializer_max_time);
   }
   // risk control
   fc::variant get_riskcontrol_account(account_name acc, const string& symbolname) {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      ilog("============code:${s}:acc:${a};c${o}", ("s", symbol_code)("a", acc.value)("o", N(oracle.bos)));
      vector<char> data = get_row_by_account(N(oracle.bos), acc, N(riskaccounts), symbol_code);
      if (data.empty()) {
         ilog("=====empty=======code:${s}:acc:${a}", ("s", symbol_code)("a", acc.value));
      }
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("riskcontrol_account", data, abi_serializer_max_time);
   }

   fc::variant get_data_service_stake(const uint64_t& service_id) {
      vector<char> data = get_row_by_account(N(oracle.bos), service_id, N(servicestake), service_id);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("data_service_stake", data, abi_serializer_max_time);
   }

   fc::variant get_transfer_freeze_delay(const uint64_t& service_id, const uint64_t& transfer_id) {
      vector<char> data = get_row_by_account(N(oracle.bos), service_id, N(freezedelays), transfer_id);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("transfer_freeze_delay", data, abi_serializer_max_time);
   }

   fc::variant get_risk_guarantee(const uint64_t& service_id, const uint64_t& risk_id) {
      vector<char> data = get_row_by_account(N(oracle.bos), service_id, N(riskguarante), risk_id);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("risk_guarantee", data, abi_serializer_max_time);
   }

   fc::variant get_account_freeze_log(const uint64_t& service_id, const uint64_t& log_id) {
      vector<char> data = get_row_by_account(N(oracle.bos), service_id, N(freezelog), log_id);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("account_freeze_log", data, abi_serializer_max_time);
   }

   fc::variant get_account_freeze_stat(const uint64_t& service_id, const name& account) {
      vector<char> data = get_row_by_account(N(oracle.bos), service_id, N(freezestats), account);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("account_freeze_stat", data, abi_serializer_max_time);
   }

   fc::variant get_service_freeze_stat(const uint64_t& service_id) {
      vector<char> data = get_row_by_account(N(oracle.bos), service_id, N(svcfrozestat), service_id);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("service_freeze_stat", data, abi_serializer_max_time);
   }

   /// api
   fc::variant get_oracle_data(const uint64_t& service_id, const uint64_t& update_number) {
      vector<char> data = get_row_by_account(N(oracle.bos), service_id, N(oracledata), update_number);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("oracle_data_record", data, abi_serializer_max_time);
   }
   // arbitration
   fc::variant get_appeal_request(const uint64_t& service_id, const uint64_t& appeal_id) {
      vector<char> data = get_row_by_account(N(oracle.bos), service_id, N(appealreq), appeal_id);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("appeal_request", data, abi_serializer_max_time);
   }

   fc::variant get_arbitrator(const name& account) {
      vector<char> data = get_row_by_account(N(oracle.bos), N(oracle.bos), N(arbitrators), account);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("arbitrator", data, abi_serializer_max_time);
   }

   fc::variant get_arbitration_case(const uint64_t& service_id, const uint64_t& arbitration_id) {
      vector<char> data = get_row_by_account(N(oracle.bos), service_id, N(arbicase), arbitration_id);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("arbitration_case", data, abi_serializer_max_time);
   }

   fc::variant get_arbitration_process(const uint64_t& arbitration_id, const uint8_t& round) {
      vector<char> data = get_row_by_account(N(oracle.bos), arbitration_id, N(arbiprocess), round);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("arbitration_process", data, abi_serializer_max_time);
   }

   fc::variant get_arbitration_evidence(const uint64_t& arbitration_id, const uint8_t& evidence_id) {
      vector<char> data = get_row_by_account(N(oracle.bos), arbitration_id, N(arbievidence), evidence_id);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("arbitration_evidence", data, abi_serializer_max_time);
   }

   fc::variant get_arbitration_result(const uint64_t& arbitration_id, const uint8_t& result_id) {
      vector<char> data = get_row_by_account(N(oracle.bos), arbitration_id, N(arbiresults), result_id);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("arbitration_result", data, abi_serializer_max_time);
   }

   fc::variant get_fair_award(const uint64_t& arbitration_id, const uint8_t& award_id) {
      vector<char> data = get_row_by_account(N(oracle.bos), arbitration_id, N(fairawards), award_id);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("fair_award", data, abi_serializer_max_time);
   }

   fc::variant get_arbitration_stake_account(const uint64_t& arbitration_id, account_name acc) {
      vector<char> data = get_row_by_account(N(oracle.bos), arbitration_id, N(arbistakeacc), acc);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("arbitration_stake_account", data, abi_serializer_max_time);
   }

   fc::variant get_arbitration_unstake_account(const uint64_t& arbitration_id, account_name acc) {
      vector<char> data = get_row_by_account(N(oracle.bos), (uint64_t(0x1) << 63) | arbitration_id, N(arbistakeacc), acc);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("arbitration_stake_account", data, abi_serializer_max_time);
   }

   fc::variant get_arbitration_stake_record(const uint64_t& arbitration_id, const uint8_t& round, account_name acc) {
      vector<char> data = get_row_by_account(N(oracle.bos), ((arbitration_id << 3) | round), N(arbistakes), acc);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("arbitration_stake_record", data, abi_serializer_max_time);
   }

   fc::variant get_arbitration_income_account(account_name acc, const string& symbolname) {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      vector<char> data = get_row_by_account(N(oracle.bos), acc, N(arbiincomes), symbol_code);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("arbitration_income_account", data, abi_serializer_max_time);
   }

   // provider
   action_result regservice(name account, asset base_stake_amount, std::string data_format, uint8_t data_type, std::string criteria, uint8_t acceptance, uint8_t injection_method, uint32_t duration,
                            uint8_t provider_limit, uint32_t update_cycle) {

      return push_action(account, N(regservice),
                         mvo()("account", account)("base_stake_amount", base_stake_amount)("data_format", data_format)("data_type", data_type)("criteria", criteria)("acceptance", acceptance)(
                             "injection_method", injection_method)("duration", duration)("provider_limit", provider_limit)("update_cycle", update_cycle));
   }

   action_result unregservice(uint64_t service_id, name account, uint8_t status) {
      return push_action(account, N(unregservice), mvo()("service_id", service_id)("account", account)("status", status));
   }

   action_result execaction(uint64_t service_id, uint8_t action_type) { return push_action(N(oracle.bos), N(execaction), mvo()("service_id", service_id)("action_type", action_type)); }

   action_result pushdata(uint64_t service_id, name provider, uint64_t update_number, uint64_t request_id, const string& data) {
      return push_action(provider, N(pushdata), mvo()("service_id", service_id)("provider", provider)("update_number", update_number)("request_id", request_id)("data", data));
   }

   action_result oraclepush(uint64_t service_id, uint64_t update_number, uint64_t request_id, const string& data, name contract_account) {
      return push_action(N(oracle.bos), N(oraclepush), mvo()("service_id", service_id)("update_number", update_number)("request_id", request_id)("data", data)("contract_account", contract_account));
   }

   action_result starttimer(uint64_t service_id, uint64_t update_number, uint64_t request_id) {
      return push_action(N(oracle.bos), N(starttimer), mvo()("service_id", service_id)("update_number", update_number)("request_id", request_id));
   }

   action_result cleardata(uint64_t service_id, uint32_t time_length) { return push_action(N(oracle.bos), N(cleardata), mvo()("service_id", service_id)("time_length", time_length)); }

   action_result addfeetypes(uint64_t service_id, std::vector<uint8_t> fee_types, std::vector<asset> service_prices) {
      return push_action(N(oracle.bos), N(addfeetypes), mvo()("service_id", service_id)("fee_types", fee_types)("service_prices", service_prices));
   }

   action_result claim(name account, name receive_account) { return push_action(account, N(claim), mvo()("account", account)("receive_account", receive_account)); }
   // consumer
   action_result subscribe(uint64_t service_id, name contract_account, name account, std::string memo) {
      return push_action(account, N(subscribe), mvo()("service_id", service_id)("contract_account", contract_account)("account", account)("memo", memo));
   }

   action_result requestdata(uint64_t service_id, name contract_account, name requester, std::string request_content) {
      return push_action(requester, N(requestdata), mvo()("service_id", service_id)("contract_account", contract_account)("requester", requester)("request_content", request_content));
   }

   // riskcontrol
   action_result withdraw(uint64_t service_id, name from, name to, asset quantity, string memo) {
      return push_action(N(oracle.bos), N(withdraw), mvo()("service_id", service_id)("from", from)("to", to)("quantity", quantity)("memo", memo));
   }

   // arbitration
   action_result acceptarbi(name arbitrator, uint64_t arbitration_id) { return push_action(arbitrator, N(acceptarbi), mvo()("arbitrator", arbitrator)("arbitration_id", arbitration_id)); }

   action_result uploadresult(name arbitrator, uint64_t arbitration_id, uint8_t result, std::string comment) {
      return push_action(arbitrator, N(uploadresult), mvo()("arbitrator", arbitrator)("arbitration_id", arbitration_id)("result", result)("comment", comment));
   }

   action_result uploadeviden(name account, uint64_t arbitration_id, std::string evidence) {
      return push_action(account, N(uploadeviden), mvo()("account", account)("arbitration_id", arbitration_id)("evidence", evidence));
   }

   action_result unstakearbi(uint64_t arbitration_id, name account, asset amount, std::string memo) {
      return push_action(account, N(unstakearbi), mvo()("arbitration_id", arbitration_id)("account", account)("amount", amount)("memo", memo));
   }

   action_result claimarbi(uint64_t arbitration_id, name account, name receive_account) { return push_action(account, N(claimarbi), mvo()("account", account)("receive_account", receive_account)); }

   action_result timertimeout(uint64_t arbitration_id, uint8_t round, uint8_t timer_type) {
      return push_action(N(oracle.bos), N(timertimeout), mvo()("arbitration_id", arbitration_id)("round", round)("timer_type", timer_type));
   }

   action_result setstatus(uint64_t arbitration_id, uint8_t status) { return push_action(N(oracle.bos), N(setstatus), mvo()("arbitration_id", arbitration_id)("status", status)); }
   action_result importwps(const std::vector<name>& auditors) { return push_action(N(oracle.bos), N(importwps), mvo()("auditors", auditors)); }

   uint64_t reg_service(name account, uint8_t data_type = 0) {
      //  name account = N(alice);
      //  uint64_t service_id =0;
      // uint8_t data_type = 0;
      uint8_t status = 0;
      uint8_t injection_method = 0;
      uint8_t acceptance = 3;
      uint32_t duration = 30;
      uint8_t provider_limit = 3;
      uint32_t update_cycle = 300;
      uint64_t appeal_freeze_period = 0;
      uint64_t exceeded_risk_control_freeze_period = 0;
      uint64_t guarantee_id = 0;
      asset amount = core_sym::from_string("1000.0000");
      asset risk_control_amount = core_sym::from_string("0.0000");
      asset pause_service_stake_amount = core_sym::from_string("0.0000");
      std::string data_format = "";
      std::string criteria = "";
      std::string declaration = "";

      auto token = regservice(account, amount, data_format, data_type, criteria, acceptance, injection_method, duration, provider_limit, update_cycle);

      uint64_t service_id = get_oracle_id(0);

      std::string a = "provider";

      for (int j = 1; j <= 5; ++j) {
         std::string acc_name = a + std::string(12 - a.size(), std::to_string(j)[0]);
         name acc = name(acc_name.c_str());

         stake_asset(service_id, acc, core_sym::from_string("1000.0000"));

         produce_blocks(1);
      }

      a = "consumer";
      for (int j = 1; j <= 5; ++j) {
         std::string acc_name = a + std::string(12 - a.size(), std::to_string(j)[0]);
         name acc = name(acc_name.c_str());
         subscribe_service(service_id, acc, acc);
         pay_service(service_id, acc, core_sym::from_string("10.0000"));

         produce_blocks(1);
      }

      return service_id;
   }

   /// stake asset
   void stake_asset(uint64_t service_id, name account, asset amount) {
      std::string memo = "0," + std::to_string(service_id);
      transfer(account, N(oracle.bos), amount.to_string().substr(0, amount.to_string().size() - 4), account.to_string().c_str(), memo);
   }

   /// add fee type
   void add_fee_type(uint64_t service_id) {
      // uint64_t service_id = new_service_id;
      std::vector<uint8_t> fee_types = {0, 1};
      std::vector<asset> service_prices = {core_sym::from_string("1.0000"), core_sym::from_string("2.0000")};
      auto token = addfeetypes(service_id, fee_types, service_prices);
   }

   // subscribe service
   void subscribe_service(uint64_t service_id, name account) { subscribe_service(service_id, account, N(dappuser.bos)); }
   // subscribe service
   void subscribe_service(uint64_t service_id, name account, name contract_account) {
      //  service_id = new_service_id;
      //  name contract_account = N(dappuser.bos);
      //  name account = N(bob);
      asset amount = core_sym::from_string("10.0000");
      std::string memo = "";
      auto subs = subscribe(service_id, contract_account, account, memo);
   }

   /// pay service
   void pay_service(uint64_t service_id, name contract_account, asset amount) {
      std::string memo = "1," + std::to_string(service_id);
      transfer(contract_account, N(oracle.bos), amount.to_string().substr(0, amount.to_string().size() - 4), contract_account.to_string().c_str(), memo);
   }

   /// push data
   void push_data(uint64_t service_id, name provider, uint64_t request_id) {
      // service_id = new_service_id;
      // name provider = N(alice);
      name contract_account = N(dappuser.bos);
      const string data = "test data json";
      // uint64_t request_id = 0;

      auto rdata = pushdata(service_id, provider, 1, request_id, data);
   }

   /// request data
   void request_data(uint64_t service_id, name account) {
      //   service_id = new_service_id;
      name contract_account = N(dappuser.bos);
      //   name account = N(bob);
      std::string request_content = "request once";
      auto req = requestdata(service_id, contract_account, account, request_content);
   }

   /// deposit
   void _deposit(uint64_t service_id, name to) {
      //   uint64_t service_id = new_service_id;
      name from = N(dappuser);
      //   name to = N(bob);
      std::string quantity = "1.0000";
      bool is_notify = false;
      std::string memo = "2,dappuser," + to.to_string() + ",0";
      transfer(from, N(oracle.bos), quantity, from.to_string().c_str(), memo);
   }

   /// withdraw
   void _withdraw(uint64_t service_id, name from) {
      //   uint64_t service_id = new_service_id;
      //   name from = N(bob);
      name to = N(dappuser);
      asset quantity = core_sym::from_string("0.1000");
      std::string memo = "";
      auto token = withdraw(service_id, from, to, quantity, memo);

      auto app_balance = get_riskcontrol_account(from, "4,BOS");
      REQUIRE_MATCHING_OBJECT(app_balance, mvo()("balance", "0.9000 BOS"));
   }

   /// arbitration
   uint64_t reg_svc_for_arbi() {
      name account = N(provider1111);
      uint64_t service_id = reg_service(account);
      // BOOST_TEST0 == service_id);
      add_fee_type(service_id);
      return service_id;
   }

   void re_stake_svc_for_arbi(uint64_t service_id) {
      std::string a = "provider";

      for (int j = 1; j <= 5; ++j) {
         std::string acc_name = a + std::string(12 - a.size(), std::to_string(j)[0]);
         name acc = name(acc_name.c_str());

         stake_asset(service_id, acc, core_sym::from_string("1000.0000"));

         produce_blocks(1);
      }
   }

   /// reg arbitrator
   void reg_arbi() {
      name to = N(oracle.bos);
      push_permission_update_auth_action(to);
      produce_blocks(1);
      for (int i = 1; i <= 5; ++i) {
         for (int j = 1; j <= 5; ++j) {
            std::string memo = "4,1";
            std::string arbi_name = "arbitrator" + std::to_string(i) + std::to_string(j);
            name from = name(arbi_name.c_str());
            transfer(from, to, "10000.0000", arbi_name.c_str(), memo);
            produce_blocks(1);
            auto arbitrator = get_arbitrator(from);
            BOOST_TEST_REQUIRE(from == arbitrator["account"].as<name>());
         }
      }

      std::vector<string> accounts_prefix = {"arbitrators"};
      for (auto& a : accounts_prefix) {
         for (int j = 1; j <= 5; ++j) {
            std::string arbi_name = a + std::string(12 - a.size(), std::to_string(j)[0]);
            std::string memo = "4,1";
            name from = name(arbi_name.c_str());
            transfer(from, to, "10000.0000", arbi_name.c_str(), memo);
            produce_blocks(1);
            auto arbitrator = get_arbitrator(from);
            BOOST_TEST_REQUIRE(from == arbitrator["account"].as<name>());
         }
      }
   }

   /// appeal
   void _appeal(uint64_t arbitration_id, std::string appeal_name, uint8_t round, string amount, uint8_t role_type) {
      push_permission_update_auth_action(N(provider.bos));
      push_permission_update_auth_action(N(arbitrat.bos));

      name to = N(oracle.bos);
      std::string memo = "3,1,'evidence','info','reason'," + std::to_string(role_type);
      // std::string appeal_name = "appellants11";
      name from = name(appeal_name.c_str());
      transfer(from, to, amount, appeal_name.c_str(), memo);
      produce_blocks(1);
      auto arbis = get_arbitration_process(arbitration_id, round);
      uint64_t id = arbis["round"].as<uint8_t>();
      BOOST_TEST_REQUIRE(round == id);

      auto stakes = get_arbitration_stake_account(arbitration_id, from);
      auto balance = stakes["balance"].as<asset>();
      BOOST_TEST_REQUIRE(core_sym::from_string(amount) == balance);
   }

   /// resp appeal
   void resp_appeal(uint64_t arbitration_id, string provider_name, string amount) {
      name to = N(oracle.bos);

      std::string memo = "5,1,''";
      // std::string provider_name = "provider1111";
      name from = name(provider_name.c_str());
      transfer(from, to, amount, provider_name.c_str(), memo);
      produce_blocks(1);
   }

   /// accept invitation
   void accept_invitation(uint64_t arbitration_id, uint8_t round) {
      uint8_t arbi_count = pow(2, round) + 1;
      uint64_t service_id = arbitration_id;
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      vector<name> arbivec = arbis["chosen_arbitrators"].as<vector<name>>();
      BOOST_TEST_REQUIRE(arbi_count == arbivec.size());
      for (auto& arbi : arbivec) {
         acceptarbi(arbi, arbitration_id);
         produce_blocks(1);
      }
   }

   void test_accept_invitation(uint64_t arbitration_id, uint8_t round) {
      uint8_t arbi_count = pow(2, round) + 1;
      uint64_t service_id = arbitration_id;
      auto arbis = get_arbitration_process(arbitration_id, round);
      vector<name> arbivec = arbis["invited_arbitrators"].as<vector<name>>();
      // BOOST_TESTarbi_count == arbivec.size());
      for (auto& arbi : arbivec) {
         produce_blocks(1);
      }
   }

   /// upload result
   void upload_result(uint64_t arbitration_id, uint8_t round, uint8_t result) {
      uint8_t arbi_count = pow(2, round) + 1;
      auto arbis = get_arbitration_process(arbitration_id, round);
      vector<name> arbivec = arbis["arbitrators"].as<vector<name>>();
      BOOST_TEST_REQUIRE(arbi_count == arbivec.size());
      for (auto& arbi : arbivec) {
         uploadresult(arbi, arbitration_id, result, "");
         produce_blocks(1);
      }
   }

   /// get final result
   void get_result(uint64_t arbitration_id, uint8_t result) {
      // BOOST_TEST0 == arbitration_id);
      uint64_t service_id = arbitration_id;
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      uint8_t final_result = arbis["final_result"].as<uint8_t>();
      BOOST_TEST_REQUIRE(result == final_result);
      produce_blocks(1);
   }

   abi_serializer abi_ser;
   abi_serializer token_abi_ser;
   abi_serializer dapp_abi_ser;
};

BOOST_AUTO_TEST_SUITE(bos_oracle_tests)

BOOST_FIXTURE_TEST_CASE(reg_test, bos_oracle_tester)
try {

   name account = N(alice);
   uint64_t service_id = 0;
   uint8_t data_type = 0;
   uint8_t status = 0;
   uint8_t injection_method = 0;
   uint8_t acceptance = 3;
   uint32_t duration = 30;
   uint8_t provider_limit = 3;
   uint32_t update_cycle = 600;
   uint64_t last_update_number = 0;
   uint64_t appeal_freeze_period = 0;
   uint64_t exceeded_risk_control_freeze_period = 0;
   uint64_t guarantee_id = 0;
   asset amount = core_sym::from_string("1000.0000");
   asset risk_control_amount = core_sym::from_string("0.0000");
   asset pause_service_stake_amount = core_sym::from_string("0.0000");
   std::string data_format = "";
   std::string criteria = "";
   std::string declaration = "";
   //   bool freeze_flag = false;
   //   bool emergency_flag = false;

   auto token = regservice(account, amount, data_format, data_type, criteria, acceptance, injection_method, duration, provider_limit, update_cycle);

   // BOOST_TEST("" == "reg service after");

   uint64_t new_service_id = get_oracle_id(0);

   auto services = get_data_service(new_service_id);
   // REQUIRE_MATCHING_OBJECT(
   //     services, mvo()("service_id", new_service_id)("data_type", data_type)("status", status)("injection_method", injection_method)("acceptance", acceptance)("duration", duration)(
   //                   "provider_limit", provider_limit)("update_cycle", update_cycle)("last_update_number", last_update_number)("appeal_freeze_period", appeal_freeze_period)(
   //                   "exceeded_risk_control_freeze_period", exceeded_risk_control_freeze_period)("guarantee_id", guarantee_id)("base_stake_amount", amount)("risk_control_amount",
   //                   risk_control_amount)( "pause_service_stake_amount", pause_service_stake_amount)("data_format", data_format)("criteria", criteria)("declaration",
   //                   declaration)("update_start_time", update_start_time));

   // BOOST_TEST("" == "reg service after");
   //  BOOST_TEST_REQUIRE( amount == get_data_provider(account)["total_stake_amount"].as<asset>() );
   // BOOST_REQUIRE_EQUAL( success(), vote(N(producvoterc), vector<account_name>(producer_names.begin(),
   // producer_names.begin()+26)) ); BOOST_REQUIRE( 0 < get_producer_info2(producer_names[11])["votepay_share"].as_double() );

   produce_blocks(1);
   /// add fee type
   {
      uint64_t service_id = new_service_id;
      std::vector<uint8_t> fee_types = {0, 1};
      std::vector<asset> service_prices = {core_sym::from_string("1.0000"), core_sym::from_string("2.0000")};
      auto token = addfeetypes(service_id, fee_types, service_prices);

      int fee_type = 1;
      auto fee = get_data_service_fee(service_id, fee_types[fee_type]);
      REQUIRE_MATCHING_OBJECT(fee, mvo()("service_id", service_id)("fee_type", fee_types[fee_type])("service_price", service_prices[fee_type]));

      produce_blocks(1);
   }

   /// stake asset
   {
      uint64_t service_id = new_service_id;
      name account = N(alice);
      asset amount = core_sym::from_string("1000.0000");
      string memo = "";
      //   push_action();
      // BOOST_TEST("" == "push_permission_update_auth_action before");
      // push_permission_update_auth_action(account);
      // BOOST_TEST("" == "push_permission_update_auth_action");
      stake_asset(service_id, account, amount);
      stake_asset(service_id, N(alice1111111), amount);
      stake_asset(service_id, N(bob111111111), amount);
      BOOST_TEST_REQUIRE(amount == get_data_provider(account)["total_stake_amount"].as<asset>());
   }

   // subscribe service
   {
      service_id = new_service_id;
      name contract_account = N(dappuser.bos);
      asset amount = core_sym::from_string("0000.0000");
      std::string memo = "";
      // BOOST_TEST(0 == service_id);
      name account = N(bob);
      auto subs = subscribe(service_id, contract_account, account, memo);
      // BOOST_TEST("" == "11ss");
      auto subscription1 = get_data_service_subscription(service_id, account);
      // BOOST_TEST("" == "ss");
      BOOST_TEST_REQUIRE(amount == subscription1["payment"].as<asset>());
      BOOST_TEST_REQUIRE(account == subscription1["account"].as<name>());
   }

   // BOOST_TEST("" == "subscription");
   produce_blocks(1);
   /// pay service
   {
      name account = N(bob);
      uint64_t service_id = new_service_id;
      name contract_account = N(dappuser.bos);
      asset amount = core_sym::from_string("10.0000");
      std::string memo = "";
      pay_service(service_id, account, amount);
   }
   // BOOST_TEST("" == "pay_service");
   /// push data
   {
      service_id = new_service_id;
      name provider = N(alice);
      name contract_account = N(dappuser.bos);
      const string data = "test data json";
      uint64_t request_id = 0;
      push_permission_update_auth_action(N(oracle.bos));
      produce_blocks(1);
      auto rdata = pushdata(service_id, provider, 1, request_id, data);
   }

   // BOOST_TEST("" == "====pushdata");

   produce_blocks(1);

   /// request data
   {
      service_id = new_service_id;
      name contract_account = N(dappuser.bos);
      name account = N(bob);
      std::string request_content = "request once";
      auto req = requestdata(service_id, contract_account, account, request_content);
   }
   produce_blocks(1);

   // BOOST_TEST("" == "====requestdata");

   produce_blocks(1);
   /// deposit
   {
      uint64_t service_id = new_service_id;
      name from = N(dappuser);
      name to = N(bob);
      string quantity = "1.0000";
      bool is_notify = false;
      std::string memo = "2,dappuser,bob,0";
      transfer(from, N(oracle.bos), quantity, from.to_string().c_str(), memo);

      auto app_balance = get_riskcontrol_account(to, "4,BOS");
      REQUIRE_MATCHING_OBJECT(app_balance, mvo()("balance", "1.0000 BOS"));
   }

   // BOOST_TEST("" == "====deposit ");
   /// withdraw
   {
      uint64_t service_id = new_service_id;
      name from = N(bob);
      name to = N(dappuser);
      asset quantity = core_sym::from_string("0.1000");
      std::string memo = "";
      auto token = withdraw(service_id, from, to, quantity, memo);

      auto app_balance = get_riskcontrol_account(from, "4,BOS");
      REQUIRE_MATCHING_OBJECT(app_balance, mvo()("balance", "0.9000 BOS"));
   }

   // BOOST_TEST("" == "====withdraw ");
   // produce_blocks(2*24*60*60);
   {
      name account = N(alice);
      name receive_account = N(alice);
      push_permission_update_auth_action(N(consumer.bos));
      auto token = claim(account, receive_account);

      BOOST_REQUIRE_EQUAL(core_sym::from_string("2000.2666"), get_balance("alice"));
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(unreg_pause_test, bos_oracle_tester)
try {

   name account = N(alice);

   uint64_t service_id = reg_service(account);
   asset amount = core_sym::from_string("1000.0000");
   stake_asset(service_id, account, amount);

   const uint8_t status_cancel = 1;
   const uint8_t status_pause = 2;
   auto token = unregservice(service_id, account, status_pause);
   BOOST_TEST_REQUIRE(status_pause == get_data_service_provision(service_id, account)["status"].as<uint8_t>());

   //   BOOST_TEST_REQUIRE( status_pause == get_svc_provision_cancel_apply(service_id)["status"].as<uint8_t>() );
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(execaction_test, bos_oracle_tester)
try {

   name account = N(alice);

   uint64_t service_id = reg_service(account);
   const uint8_t freeze_action_type = 5;
   const uint8_t emergency_action_type = 6;
   auto token = execaction(service_id, freeze_action_type);
   BOOST_TEST_REQUIRE(freeze_action_type == get_data_service(service_id)["status"].as<uint8_t>());
   token = execaction(service_id, emergency_action_type);
   BOOST_TEST_REQUIRE(emergency_action_type == get_data_service(service_id)["status"].as<uint8_t>());
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(stakeasset_test, bos_oracle_tester)
try {

   name account = N(alice);

   uint64_t new_service_id = reg_service(account);
   uint64_t new_second_service_id = reg_service(account);

   // BOOST_TEST("" == "====reg test true");
   produce_blocks(1);
   /// stake asset
   {
      uint64_t service_id = new_service_id;
      name account = N(alice);
      asset amount = core_sym::from_string("1000.0000");
      string memo = "";
      stake_asset(service_id, account, amount);
      BOOST_TEST_REQUIRE(amount == get_data_provider(account)["total_stake_amount"].as<asset>());

      asset add_amount = core_sym::from_string("10.0001");
      stake_asset(service_id, account, add_amount);
      BOOST_TEST_REQUIRE((amount + add_amount) == get_data_provider(account)["total_stake_amount"].as<asset>());
   }

   /// stake asset
   {
      uint64_t service_id = new_second_service_id;
      name account = N(alice);
      asset amount = core_sym::from_string("1000.0000");
      string memo = "";
      BOOST_REQUIRE_EXCEPTION(stake_asset(service_id, account, core_sym::from_string("100.0000")), eosio_assert_message_exception,
                              eosio_assert_message_is("stake amount could not be less than  the base_stake amount of the service"));
      // BOOST_REQUIRE_EQUAL(wasm_assert_msg("stake amount could not be less than  the base_stake amount of the service"), stake_asset(service_id, account, core_sym::from_string("100.0000")));
      stake_asset(service_id, account, amount);
      auto services = get_data_provider(account)["services"].as<vector<uint64_t>>();
      BOOST_TEST_REQUIRE(services.size() == 2);
      BOOST_TEST_REQUIRE(new_service_id == services[0]);
      BOOST_TEST_REQUIRE(new_second_service_id == services[1]);
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(pushdata_test, bos_oracle_tester)
try {

   /// reg service
   name account = N(alice);

   uint64_t service_id = reg_service(account);
   add_fee_type(service_id);
   stake_asset(service_id, N(alice), core_sym::from_string("1000.0000"));
   subscribe_service(service_id, N(bob));
   pay_service(service_id, N(bob), core_sym::from_string("10.0000"));

   /// push data
   {
      name provider = N(alice);
      name contract_account = N(dappuser.bos);
      const string data = "test data json";
      uint64_t request_id = 0;

      auto rdata = pushdata(service_id, provider, 1, request_id, data);
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(cleardata_test, bos_oracle_tester)
try {

   /// reg service
   name account = N(alice);

   uint64_t service_id = reg_service(account,1);
   add_fee_type(service_id);
   stake_asset(service_id, N(alice), core_sym::from_string("1000.0000"));
   subscribe_service(service_id, N(bob));
   pay_service(service_id, N(bob), core_sym::from_string("10.0000"));

   /// push data
   {
      name provider = N(alice);
      name contract_account = N(dappuser.bos);
      const string data = "test data json";
      uint64_t request_id = 0;

      auto rdata = pushdata(service_id, provider, 1, request_id, data);

      produce_block(fc::days(3));
      produce_blocks(10);
      auto cdata = cleardata(service_id, 10800);
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(push_deterministic_data_to_table_timeout_test, bos_oracle_tester)
try {

   /// reg service
   name account = N(alice);

   uint64_t service_id = reg_service(N(provider1111));
   add_fee_type(service_id);

   /// publish data
   {
      auto services = get_data_service(service_id);

      // produce_block(fc::hours(1));
      uint32_t update_cycle = services["update_cycle"].as<uint32_t>();
      BOOST_REQUIRE(update_cycle > 0);
      uint32_t duration = services["duration"].as<uint32_t>();
      time_point_sec update_start_time = services["update_start_time"].as<time_point_sec>();
      BOOST_REQUIRE(duration > 0);
      time_point_sec current_time = time_point_sec(control->head_block_time());

      uint64_t update_number = (current_time.sec_since_epoch() - update_start_time.sec_since_epoch()) / update_cycle + 1;
      uint32_t current_duration_begin_time = time_point_sec(update_start_time + (update_number - 1) * update_cycle).sec_since_epoch();
      uint32_t current_duration_end_time = current_duration_begin_time + duration;
      if (current_time.sec_since_epoch() > current_duration_end_time) {
         produce_block(fc::seconds(current_duration_begin_time + update_cycle - current_time.sec_since_epoch()));
      }

      current_time = time_point_sec(control->head_block_time());

      update_number = (current_time.sec_since_epoch() - update_start_time.sec_since_epoch()) / update_cycle + 1;
      current_duration_begin_time = time_point_sec(update_start_time + (update_number - 1) * update_cycle).sec_since_epoch();
      current_duration_end_time = current_duration_begin_time + duration;

      const string data = "publish test data json";
      uint64_t request_id = 0;

      // BOOST_TEST(0 == update_number);
      // BOOST_TEST(0 == current_duration_end_time);
      // BOOST_TEST(0 == current_time.sec_since_epoch());
      std::string a = "provider";
      for (int j = 1; j <= 4; ++j) {
         std::string acc_name = a + std::string(12 - a.size(), std::to_string(j)[0]);
         name acc = name(acc_name.c_str());
         auto rdata = pushdata(service_id, acc, update_number, request_id, data);
         produce_blocks(2);
      }

      current_time = time_point_sec(control->head_block_time());
      while (current_time.sec_since_epoch() <= current_duration_end_time + 12) {
         // BOOST_TEST(0 == current_time.sec_since_epoch());
         produce_block(fc::seconds(1));
         current_time = time_point_sec(control->head_block_time());
      }

      // current_time = time_point_sec(control->head_block_time());
      // BOOST_TEST(0 == current_time.sec_since_epoch());
      // auto oracledata = get_oracle_data(service_id, update_number);

      // uint64_t update_number_from_api = oracledata["update_number"].as<uint64_t>();
      // BOOST_TEST_REQUIRE(update_number_from_api == update_number);
      fetchdata(service_id, update_number, 0);
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(push_deterministic_data_to_table_test, bos_oracle_tester)
try {

   /// reg service
   name account = N(alice);

   uint64_t service_id = reg_service(N(provider1111));
   add_fee_type(service_id);

   /// publish data
   {
      auto services = get_data_service(service_id);

      // produce_block(fc::hours(1));
      uint32_t update_cycle = services["update_cycle"].as<uint32_t>();
      BOOST_REQUIRE(update_cycle > 0);
      uint32_t duration = services["duration"].as<uint32_t>();
      time_point_sec update_start_time = services["update_start_time"].as<time_point_sec>();
      BOOST_REQUIRE(duration > 0);
      time_point_sec current_time = time_point_sec(control->head_block_time());

      uint64_t update_number = (current_time.sec_since_epoch() - update_start_time.sec_since_epoch()) / update_cycle + 1;
      uint32_t current_duration_begin_time = time_point_sec(update_start_time + (update_number - 1) * update_cycle).sec_since_epoch();
      uint32_t current_duration_end_time = current_duration_begin_time + duration;
      if (current_time.sec_since_epoch() > current_duration_end_time) {
         produce_block(fc::seconds(duration));
      }

      current_time = time_point_sec(control->head_block_time());

      update_number = (current_time.sec_since_epoch() - update_start_time.sec_since_epoch()) / update_cycle + 1;

      const string data = "publish test data json";
      uint64_t request_id = 0;

      std::string a = "provider";
      for (int j = 1; j <= 5; ++j) {
         std::string acc_name = a + std::string(12 - a.size(), std::to_string(j)[0]);
         name acc = name(acc_name.c_str());
         auto rdata = pushdata(service_id, acc, update_number, request_id, data);
         produce_blocks(2);
      }

      //  BOOST_REQUIRE_EXCEPTION( acceptarbi(N(alice), arbitration_id),
      //                    eosio_assert_message_exception, eosio_assert_message_is("could not find such an arbitrator in current chosen arbitration." ) );
      BOOST_REQUIRE_EQUAL(wasm_assert_msg("update_number should be greater than last_number of the service"), pushdata(service_id, N(provider1111), update_number, request_id, data));

      // BOOST_TEST0 == 8);
      // auto oracledata = get_oracle_data(service_id, update_number);

      // uint64_t update_number_from_api = oracledata["update_number"].as<uint64_t>();
      // BOOST_TEST_REQUIRE(update_number_from_api == update_number);
      // BOOST_TEST0 == update_number);
      fetchdata(service_id, update_number, 0);
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(push_differ_deterministic_data_to_table_test, bos_oracle_tester)
try {

   /// reg service
   name account = N(alice);

   uint64_t service_id = reg_service(N(provider1111));
   add_fee_type(service_id);

   /// publish data
   {
      auto services = get_data_service(service_id);

      uint32_t update_cycle = services["update_cycle"].as<uint32_t>();
      BOOST_REQUIRE(update_cycle > 0);
      uint32_t duration = services["duration"].as<uint32_t>();
      time_point_sec update_start_time = services["update_start_time"].as<time_point_sec>();
      BOOST_REQUIRE(duration > 0);
      time_point_sec current_time = time_point_sec(control->head_block_time());

      uint64_t update_number = (current_time.sec_since_epoch() - update_start_time.sec_since_epoch()) / update_cycle + 1;
      uint32_t current_duration_begin_time = time_point_sec(update_start_time + (update_number - 1) * update_cycle).sec_since_epoch();
      uint32_t current_duration_end_time = current_duration_begin_time + duration;
      if (current_time.sec_since_epoch() > current_duration_end_time) {
         produce_block(fc::seconds(duration));
      }

      current_time = time_point_sec(control->head_block_time());

      update_number = (current_time.sec_since_epoch() - update_start_time.sec_since_epoch()) / update_cycle + 1;

      const string data = "publish test data json";
      uint64_t request_id = 0;

      std::string a = "provider";
      for (int j = 1; j <= 4; ++j) {
         std::string acc_name = a + std::string(12 - a.size(), std::to_string(j)[0]);
         name acc = name(acc_name.c_str());
         auto rdata = pushdata(service_id, acc, update_number, request_id, data);
         produce_blocks(2);
      }

      const string differ_data = " publish test differ data json";

      auto rdata = pushdata(service_id, N(provider5555), update_number, request_id, differ_data);

      // BOOST_TEST0 == 8);
      // auto oracledata = get_oracle_data(service_id, update_number);
      // uint64_t update_number_from_api = oracledata["update_number"].as<uint64_t>();
      // BOOST_TEST_REQUIRE(update_number_from_api == update_number);
      // BOOST_TEST0 == update_number);
      fetchdata(service_id, update_number, 0);
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(push_non_deterministic_data_to_table_test, bos_oracle_tester)
try {

   /// reg service
   name account = N(alice);
   uint64_t service_id = reg_service(N(provider1111), 1);
   add_fee_type(service_id);

   /// publish data
   {
      auto services = get_data_service(service_id);

      // produce_block(fc::hours(1));
      uint32_t update_cycle = services["update_cycle"].as<uint32_t>();
      BOOST_REQUIRE(update_cycle > 0);
      uint32_t duration = services["duration"].as<uint32_t>();
      time_point_sec update_start_time = services["update_start_time"].as<time_point_sec>();
      BOOST_REQUIRE(duration > 0);
      time_point_sec current_time = time_point_sec(control->head_block_time());

      uint64_t update_number = (current_time.sec_since_epoch() - update_start_time.sec_since_epoch()) / update_cycle + 1;
      uint32_t current_duration_begin_time = time_point_sec(update_start_time + (update_number - 1) * update_cycle).sec_since_epoch();
      uint32_t current_duration_end_time = current_duration_begin_time + duration;
      if (current_time.sec_since_epoch() > current_duration_end_time) {
         produce_block(fc::seconds(duration));
      }

      current_time = time_point_sec(control->head_block_time());

      update_number = (current_time.sec_since_epoch() - update_start_time.sec_since_epoch()) / update_cycle + 1;

      const string data = "publish test data json";
      uint64_t request_id = 0;

      std::string a = "provider";
      for (int j = 1; j <= 5; ++j) {
         std::string acc_name = a + std::string(12 - a.size(), std::to_string(j)[0]);
         name acc = name(acc_name.c_str());
         auto rdata = pushdata(service_id, acc, update_number, request_id, data);
         produce_blocks(2);
      }

      //  BOOST_REQUIRE_EXCEPTION( acceptarbi(N(alice), arbitration_id),
      //                    eosio_assert_message_exception, eosio_assert_message_is("could not find such an arbitrator in current chosen arbitration." ) );
      // BOOST_REQUIRE_EQUAL(wasm_assert_msg("update_number should be greater than last_number of the service"), pushdata(service_id, N(provider1111), update_number, request_id, data));

      // BOOST_TEST0 == 8);
      // auto oracledata = get_oracle_data(service_id, update_number);

      // uint64_t update_number_from_api = oracledata["update_number"].as<uint64_t>();
      // BOOST_TEST_REQUIRE(update_number_from_api == update_number);
      // BOOST_TEST0 == update_number);
      fetchdata(service_id, update_number, 0);
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(addfeetype_test, bos_oracle_tester)
try {
   name account = N(alice);

   uint64_t new_service_id = reg_service(account);

   /// add fee type
   {
      uint64_t service_id = new_service_id;
      std::vector<uint8_t> fee_types = {0, 1};
      std::vector<asset> service_prices = {core_sym::from_string("1.0000"), core_sym::from_string("2.0000")};
      auto token = addfeetypes(service_id, fee_types, service_prices);

      int fee_type = 1;
      auto fee = get_data_service_fee(service_id, fee_types[fee_type]);
      REQUIRE_MATCHING_OBJECT(fee, mvo()("service_id", service_id)("fee_type", fee_types[fee_type])("service_price", service_prices[fee_type]));
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(subscribe_test, bos_oracle_tester)
try {
   name account = N(alice);

   uint64_t service_id = reg_service(account);
   add_fee_type(service_id);
   stake_asset(service_id, N(alice), core_sym::from_string("1000.0000"));

   // subscribe service
   {
      name contract_account = N(dappuser.bos);
      name account = N(bob);
      asset amount = core_sym::from_string("0.0000");
      std::string memo = "";
      // BOOST_TEST(0 == service_id);

      auto subs = subscribe(service_id, contract_account, account, memo);

      // BOOST_TEST("" == "11ss");
      auto subscription = get_data_service_subscription(service_id, account);
      // BOOST_TEST("" == "ss");
      BOOST_TEST_REQUIRE(amount == subscription["payment"].as<asset>());
      BOOST_TEST_REQUIRE(account == subscription["account"].as<name>());
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(requestdata_test, bos_oracle_tester)
try {

   name account = N(alice);

   uint64_t service_id = reg_service(account);

   add_fee_type(service_id);
   stake_asset(service_id, N(alice), core_sym::from_string("1000.0000"));
   subscribe_service(service_id, N(bob));
   pay_service(service_id, N(bob), core_sym::from_string("10.0000"));

   /// request data
   {
      name contract_account = N(dappuser.bos);
      name account = N(bob);
      std::string request_content = "request once";
      auto req = requestdata(service_id, contract_account, account, request_content);
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(payservice_test, bos_oracle_tester)
try {
   name account = N(alice);

   uint64_t service_id = reg_service(account);

   add_fee_type(service_id);
   stake_asset(service_id, N(alice), core_sym::from_string("1000.0000"));
   subscribe_service(service_id, N(bob));

   /// pay service
   {
      name contract_account = N(dappuser.bos);
      asset amount = core_sym::from_string("10.0000");
      std::string memo = "";
      pay_service(service_id, N(bob), amount);
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(claim_test, bos_oracle_tester)
try {
   name account = N(alice);

   uint64_t service_id = reg_service(account);

   add_fee_type(service_id);
   stake_asset(service_id, N(alice), core_sym::from_string("1000.0000"));
   subscribe_service(service_id, N(bob));
   pay_service(service_id, N(bob), core_sym::from_string("10.0000"));
   request_data(service_id, N(bob));
   push_data(service_id, N(alice), 0);
   _deposit(service_id, N(bob));
   _withdraw(service_id, N(bob));

   {
      name account = N(alice);
      name receive_account = N(alice);
      push_permission_update_auth_action(N(consumer.bos));
      auto token = claim(account, receive_account);

      BOOST_REQUIRE_EQUAL(core_sym::from_string("2000.1333"), get_balance("alice"));
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(deposit_test, bos_oracle_tester)
try {
   name account = N(alice);

   uint64_t service_id = reg_service(account);

   add_fee_type(service_id);
   stake_asset(service_id, N(alice), core_sym::from_string("1000.0000"));
   subscribe_service(service_id, N(bob));
   pay_service(service_id, N(bob), core_sym::from_string("10.0000"));
   /// deposit
   {
      name from = N(dappuser);
      name to = N(bob);
      std::string quantity = "1.0000";
      bool is_notify = false;
      // auto token = deposit(service_id, from, to, quantity, memo, is_notify);
      std::string memo = "2,dappuser,bob,0";
      transfer(from, N(oracle.bos), quantity, from.to_string().c_str(), memo);

      auto app_balance = get_riskcontrol_account(to, "4,BOS");
      REQUIRE_MATCHING_OBJECT(app_balance, mvo()("balance", quantity + " BOS"));
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(withdraw_test, bos_oracle_tester)
try {
   name account = N(alice);

   uint64_t service_id = reg_service(account);

   add_fee_type(service_id);
   stake_asset(service_id, N(alice), core_sym::from_string("1000.0000"));
   subscribe_service(service_id, N(bob));
   pay_service(service_id, N(bob), core_sym::from_string("10.0000"));
   request_data(service_id, N(bob));
   push_data(service_id, N(alice), 0);
   _deposit(service_id, N(bob));

   /// withdraw
   {
      name from = N(bob);
      name to = N(dappuser);
      asset quantity = core_sym::from_string("0.1000");
      std::string memo = "";
      auto token = withdraw(service_id, from, to, quantity, memo);

      auto app_balance = get_riskcontrol_account(from, "4,BOS");
      REQUIRE_MATCHING_OBJECT(app_balance, mvo()("balance", "0.9000 BOS"));
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_import_arbitrator_test, bos_oracle_tester)
try {

   name to = N(oracle.bos);
   std::vector<name> auditors;
   /// reg arbitrator
   {

      produce_blocks(1);
      for (int i = 1; i <= 5; ++i) {
         for (int j = 1; j <= 5; ++j) {
            std::string arbi_name = "wpsauditor" + std::to_string(i) + std::to_string(j);
            name from = name(arbi_name.c_str());
            auditors.push_back(from);
         }
      }

      importwps(auditors);
      to = auditors[0];
      auto arbitrator = get_arbitrator(to);
      BOOST_TEST_REQUIRE(to == arbitrator["account"].as<name>());
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_reg_arbitrator_test, bos_oracle_tester)
try {
   name to = N(oracle.bos);
   /// reg arbitrator
   {
      push_permission_update_auth_action(to);
      produce_blocks(1);
      for (int i = 1; i <= 5; ++i) {
         for (int j = 1; j <= 5; ++j) {
            std::string memo = "4,1";
            std::string arbi_name = "arbitrator" + std::to_string(i) + std::to_string(j);
            name from = name(arbi_name.c_str());
            transfer(from, to, "10000.0000", arbi_name.c_str(), memo);
            produce_blocks(1);
            auto arbitrator = get_arbitrator(from);
            BOOST_TEST_REQUIRE(from == arbitrator["account"].as<name>());
         }
      }
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_reg_malicious_arbitrator_test, bos_oracle_tester)
try {
   name to = N(oracle.bos);
   {
      // push_permission_update_auth_action(to);
      produce_blocks(1);
      // for (
      int i = 1;
      //  i <= 5; ++i) {
      //    for (
      int j = 1;
      // j <= 5; ++j) {
      std::string memo = "4,1";
      std::string arbi_name = "dappuser.bos"; //+ std::to_string(i) + std::to_string(j);
      name from = name(arbi_name.c_str());
      transferex(N(dapp.bos), from, to, "10000.0000 BOS", from, memo);
      produce_blocks(1);

      transferex(N(eosio.token), from, to, "10000.0000 BQS", from, memo);
      produce_blocks(1);

      consumer_transfer(from, to, core_sym::from_string("10000.0000"), memo);

      transfer(from, from, "10000.0000", from, memo);
      // auto arbitrator = get_arbitrator(from);
      // BOOST_TEST_REQUIRE(from == arbitrator["account"].as<name>());
      //    }
      // }
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_reappeal_timeout_test, bos_oracle_tester)
try {

   name to = N(oracle.bos);
   /// reg arbitrator
   reg_arbi();

   uint64_t service_id = reg_svc_for_arbi();
   uint64_t arbitration_id = service_id;
   uint8_t round = 1;
   std::string appeal_name = "appellants11";
   string amount = "200.0000";
   uint8_t role_type = 1; /// consumer
   /// appeal
   _appeal(arbitration_id, appeal_name, round, amount, role_type);

   /// resp appeal
   std::string provider_name = "provider1111";

   resp_appeal(arbitration_id, provider_name, amount);

   /// accept invitation
   accept_invitation(arbitration_id, round);

   uint8_t result = 1;
   /// upload result
   upload_result(arbitration_id, round, result);

   produce_blocks(60 * 60 * 2 + 10);

   /// get final result
   {
      // get_result(arbitration_id, result);
      // BOOST_TEST0 == arbitration_id);
      uint64_t service_id = arbitration_id;
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      uint8_t final_result = arbis["final_result"].as<uint8_t>();
      BOOST_TEST_REQUIRE(result == final_result);
      produce_blocks(1);
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_reappeal_timeout_resp_from_no_provider_test, bos_oracle_tester)
try {

   name to = N(oracle.bos);
   /// reg arbitrator
   reg_arbi();

   uint64_t service_id = reg_svc_for_arbi();
   uint64_t arbitration_id = service_id;
   uint8_t round = 1;
   std::string appeal_name = "appellants11";
   string amount = "200.0000";
   uint8_t role_type = 1; /// consumer
   /// appeal
   _appeal(arbitration_id, appeal_name, round, amount, role_type);

   /// resp appeal
   std::string provider_name = "provider1111";
   std::string no_provider_name = "dapp";

   resp_appeal(arbitration_id, provider_name, amount);
   resp_appeal(arbitration_id, no_provider_name, amount);

   /// accept invitation
   accept_invitation(arbitration_id, round);

   uint8_t result = 2;
   /// upload result
   upload_result(arbitration_id, round, result);

   auto stakes = get_arbitration_stake_account(arbitration_id, name(no_provider_name.c_str()));
   auto balance = stakes["balance"].as<asset>();
   BOOST_TEST_REQUIRE(core_sym::from_string(amount) == balance);

   produce_blocks(60 * 60 * 2 + 10);

   /// get final result
   {

      auto stakes = get_arbitration_unstake_account(arbitration_id, name(no_provider_name.c_str()));
      auto balance = stakes["balance"].as<asset>();
      BOOST_TEST_REQUIRE(core_sym::from_string(amount) == balance);

      // get_result(arbitration_id, result);
      // BOOST_TEST0 == arbitration_id);
      uint64_t service_id = arbitration_id;
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      uint8_t final_result = arbis["final_result"].as<uint8_t>();
      BOOST_TEST_REQUIRE(result == final_result);
      produce_blocks(1);
   }
}
FC_LOG_AND_RETHROW()
BOOST_FIXTURE_TEST_CASE(arbi_reappeal_timeout_over_reappeal_new_case_test, bos_oracle_tester)
try {

   name to = N(oracle.bos);
   /// reg arbitrator
   reg_arbi();

   uint64_t service_id = reg_svc_for_arbi();
   uint64_t arbitration_id = service_id;
   uint8_t round = 1;
   std::string appeal_name = "appellants11";
   string amount = "200.0000";
   uint8_t role_type = 1; /// consumer
   /// appeal
   _appeal(arbitration_id, appeal_name, round, amount, role_type);

   /// resp appeal
   std::string provider_name = "provider1111";

   resp_appeal(arbitration_id, provider_name, amount);

   /// accept invitation
   accept_invitation(arbitration_id, round);

   uint8_t result = 1;
   /// upload result
   upload_result(arbitration_id, round, result);

   produce_blocks(60 * 60 * 2 + 10);

   /// get final result
   {
      // get_result(arbitration_id, result);
      // BOOST_TEST0 == arbitration_id);
      uint64_t service_id = arbitration_id;
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      uint8_t final_result = arbis["final_result"].as<uint8_t>();
      BOOST_TEST_REQUIRE(result == final_result);
      produce_blocks(1);
   }

   re_stake_svc_for_arbi(service_id);

   produce_blocks(1);

   /// appeal
   {
      name to = N(oracle.bos);
      std::string memo = "3,1,'evidence','info','reason'," + std::to_string(role_type);
      // std::string appeal_name = "appellants11";
      name from = name(appeal_name.c_str());
      transfer(from, to, amount, appeal_name.c_str(), memo);
      produce_blocks(1);
      auto arbis = get_arbitration_process(arbitration_id, round);
      uint64_t id = arbis["round"].as<uint8_t>();
      BOOST_TEST_REQUIRE(round == id);

      auto stakes = get_arbitration_stake_account(arbitration_id, from);
      auto balance = stakes["balance"].as<asset>();
      BOOST_TEST_REQUIRE(core_sym::from_string(amount) == balance);
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_reappeal_timeout_over_reappeal_new_case_result_reverse_test, bos_oracle_tester)
try {

   name to = N(oracle.bos);
   /// reg arbitrator
   reg_arbi();

   uint64_t service_id = reg_svc_for_arbi();
   uint64_t arbitration_id = service_id;
   uint8_t round = 1;
   std::string appeal_name = "appellants11";
   string amount = "200.0000";
   uint8_t role_type = 1; /// consumer
   /// appeal
   _appeal(arbitration_id, appeal_name, round, amount, role_type);

   /// resp appeal
   std::string provider_name = "provider1111";

   resp_appeal(arbitration_id, provider_name, amount);

   /// accept invitation
   accept_invitation(arbitration_id, round);

   uint8_t result = 1;
   /// upload result
   upload_result(arbitration_id, round, result);

   produce_blocks(60 * 60 * 2 + 10);

   /// get final result
   {
      // get_result(arbitration_id, result);
      // BOOST_TEST0 == arbitration_id);
      uint64_t service_id = arbitration_id;
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      uint8_t final_result = arbis["final_result"].as<uint8_t>();
      BOOST_TEST_REQUIRE(result == final_result);

      auto stakes = get_arbitration_unstake_account(arbitration_id, name(appeal_name.c_str()));
      auto balance = stakes["balance"].as<asset>();
      BOOST_TEST_REQUIRE(core_sym::from_string(amount) == balance);

      produce_blocks(1);
   }

   re_stake_svc_for_arbi(service_id);

   produce_blocks(1);

   role_type = 1; /// consumer
   /// appeal
   _appeal(arbitration_id, appeal_name, round, amount, role_type);

   /// resp appeal

   resp_appeal(arbitration_id, provider_name, amount);

   /// accept invitation
   accept_invitation(arbitration_id, round);

   result = 2;
   /// upload result
   upload_result(arbitration_id, round, result);

   produce_blocks(60 * 60 * 2 + 10);

   /// get final result
   {
      // get_result(arbitration_id, result);
      // BOOST_TEST0 == arbitration_id);
      uint64_t service_id = arbitration_id;
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      uint8_t final_result = arbis["final_result"].as<uint8_t>();
      BOOST_TEST_REQUIRE(result == final_result);

      produce_blocks(1);
   }

   string acc = appeal_name;
   {
      BOOST_TEST(core_sym::from_string("19600.0004") == get_balance(acc));
      name account = name(acc.c_str());
      auto token = unstakearbi(arbitration_id, account, core_sym::from_string(amount), "");

      BOOST_REQUIRE_EQUAL(core_sym::from_string("19800.0004"), get_balance(acc));
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_muilti_appeal_test, bos_oracle_tester)
try {

   name to = N(oracle.bos);
   /// reg arbitrator
   reg_arbi();

   uint64_t service_id = reg_svc_for_arbi();
   uint64_t arbitration_id = service_id;
   uint8_t round = 1;
   std::string appeal_name = "appellants11";
   std::string second_appeal_name = "appellants22";
   string amount = "200.0000";
   uint8_t role_type = 1; /// consumer
   /// appeal
   _appeal(arbitration_id, appeal_name, round, amount, role_type);

   /// appeal
   _appeal(arbitration_id, second_appeal_name, round, amount, role_type);

   /// resp appeal
   std::string provider_name = "provider1111";
   std::string second_provider_name = "provider2222";

   resp_appeal(arbitration_id, provider_name, amount);
   resp_appeal(arbitration_id, second_provider_name, amount);

   /// accept invitation
   accept_invitation(arbitration_id, round);

   uint8_t result = 1;
   /// upload result
   upload_result(arbitration_id, round, result);

   produce_blocks(60 * 60 * 2 + 10);

   /// get final result
   {
      // get_result(arbitration_id, result);
      // BOOST_TEST0 == arbitration_id);
      uint64_t service_id = arbitration_id;
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      uint8_t final_result = arbis["final_result"].as<uint8_t>();
      BOOST_TEST_REQUIRE(result == final_result);
      produce_blocks(1);
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_resp_appeal_timeout_test, bos_oracle_tester)
try {
   name to = N(oracle.bos);
   /// reg arbitrator
   reg_arbi();

   uint64_t service_id = reg_svc_for_arbi();
   uint64_t arbitration_id = service_id;
   uint8_t round = 1;
   std::string appeal_name = "appellants11";
   string amount = "200.0000";
   uint8_t role_type = 1; /// consumer
   /// appeal
   _appeal(arbitration_id, appeal_name, round, amount, role_type);

   /// resp appeal
   std::string provider_name = "provider1111";

   resp_appeal(arbitration_id, provider_name, amount);

   /// accept invitation
   accept_invitation(arbitration_id, round);

   uint8_t result = 1;
   /// upload result
   upload_result(arbitration_id, round, result);

   round = 2;
   amount = "400.0000";
   /// reappeal
   {
      std::string memo = "3,1,'evidence','info','reason',2";
      std::string appeal_name = "provider1111";
      name from = name(appeal_name.c_str());
      transfer(from, to, amount, appeal_name.c_str(), memo);
      produce_blocks(1);
      auto arbis = get_arbitration_process(arbitration_id, round);
      uint64_t id = arbis["round"].as<uint8_t>();
      BOOST_TEST_REQUIRE(round == id);

      auto stakes = get_arbitration_stake_account(arbitration_id, from);
      auto balance = stakes["balance"].as<asset>();
      BOOST_TEST_REQUIRE(core_sym::from_string("600.0000") == balance);
   }

   produce_blocks(60 * 60 * 2 + 10);

   /// get final result
   {
      uint8_t result = 1;
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      uint64_t arbitration_result = arbis["arbitration_result"].as<uint8_t>();
      BOOST_TEST_REQUIRE(result == arbitration_result);
      uint64_t final_result = arbis["final_result"].as<uint8_t>();
      BOOST_TEST_REQUIRE(2 == final_result);
      produce_blocks(1);
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_reappeal_role_type_wrong_test, bos_oracle_tester)
try {
   name to = N(oracle.bos);
   /// reg arbitrator
   reg_arbi();

   uint64_t service_id = reg_svc_for_arbi();
   uint64_t arbitration_id = service_id;
   uint8_t round = 1;
   std::string appeal_name = "appellants11";
   string amount = "200.0000";
   uint8_t role_type = 1; /// consumer

   /// appeal wrong role type
   {
      std::string memo = "3,1,'evidence','info','reason',2";
      std::string appeal_name = "provider1111";
      name from = name(appeal_name.c_str());
      BOOST_REQUIRE_EXCEPTION(transfer(from, to, amount, appeal_name.c_str(), memo), eosio_assert_message_exception, eosio_assert_message_is("only support consume(1) role type  in the first round"));
      // BOOST_REQUIRE_EQUAL(wasm_assert_msg("only support consume(1) role type  in the first round"), transfer(from, to, amount, appeal_name.c_str(), memo));
   }

   /// appeal
   _appeal(arbitration_id, appeal_name, round, amount, role_type);

   /// resp appeal
   std::string provider_name = "provider1111";

   resp_appeal(arbitration_id, provider_name, amount);

   /// accept invitation
   accept_invitation(arbitration_id, round);

   uint8_t result = 1;
   /// upload result
   upload_result(arbitration_id, round, result);

   round = 2;
   amount = "400.0000";
   /// reappeal wrong role type
   {
      std::string memo = "3,1,'evidence','info','reason',1";
      std::string appeal_name = "provider1111";
      name from = name(appeal_name.c_str());
      BOOST_REQUIRE_EXCEPTION(transfer(from, to, amount, appeal_name.c_str(), memo), eosio_assert_message_exception,
                              eosio_assert_message_is("role type  could not be  the winner of the previous round"));
      // BOOST_REQUIRE_EQUAL(wasm_assert_msg("role type  could not be  the winner of the previous round"), transfer(from, to, amount, appeal_name.c_str(), memo));

      produce_blocks(1);
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_reappeal_test, bos_oracle_tester)
try {
   name to = N(oracle.bos);
   /// reg arbitrator
   reg_arbi();

   uint64_t service_id = reg_svc_for_arbi();
   uint64_t arbitration_id = service_id;
   uint8_t round = 1;
   std::string appeal_name = "appellants11";
   string amount = "200.0000";
   uint8_t role_type = 1; /// consumer
   /// appeal
   _appeal(arbitration_id, appeal_name, round, amount, role_type);

   /// resp appeal
   std::string provider_name = "provider1111";

   resp_appeal(arbitration_id, provider_name, amount);

   test_accept_invitation(arbitration_id, round);
   /// accept invitation
   accept_invitation(arbitration_id, round);

   uint8_t result = 1;
   /// upload result
   upload_result(arbitration_id, round, result);

   round = 2;
   amount = "400.0000";
   /// reappeal
   {
      std::string memo = "3,1,'evidence','info','reason',2";
      std::string appeal_name = "provider1111";
      name from = name(appeal_name.c_str());
      transfer(from, to, amount, appeal_name.c_str(), memo);
      produce_blocks(1);

      auto appeal_req_t = get_appeal_request(arbitration_id << 3 | round, from.value);
      uint64_t role_type_t = appeal_req_t["role_type"].as<uint8_t>();
      BOOST_TEST(6 == role_type_t);
      auto arbis = get_arbitration_process(arbitration_id, round);
      uint64_t id = arbis["round"].as<uint8_t>();
      BOOST_TEST_REQUIRE(round == id);

      auto stakes = get_arbitration_stake_account(arbitration_id, from);
      auto balance = stakes["balance"].as<asset>();
      BOOST_TEST_REQUIRE(core_sym::from_string("600.0000") == balance);
   }

   /// resp reappeal
   {
      std::string memo = "5,1,''";
      std::string provider_name = "appellants11";
      name from = name(provider_name.c_str());
      transfer(from, to, amount, provider_name.c_str(), memo);
      produce_blocks(1);
   }

   test_accept_invitation(arbitration_id, round);

   /// reaccept invitation
   {
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      vector<name> arbivec = arbis["chosen_arbitrators"].as<vector<name>>();
      BOOST_TEST_REQUIRE(8 == arbivec.size());
      for (uint8_t i = 3; i < arbivec.size(); i++) {
         acceptarbi(arbivec[i], arbitration_id);
         produce_blocks(1);
      }
   }

   result = 1;
   /// reupload result
   {
      auto arbis = get_arbitration_process(arbitration_id, round);
      vector<name> arbivec = arbis["arbitrators"].as<vector<name>>();
      BOOST_TEST_REQUIRE(5 == arbivec.size());
      for (auto& arbi : arbivec) {
         uploadresult(arbi, arbitration_id, result, "");
         produce_blocks(1);
      }
   }

   round = 3;
   amount = "800.0000";
   /// reappeal
   {
      std::string memo = "3,1,'evidence','info','reason',2";
      std::string appeal_name = "provider1111";
      name from = name(appeal_name.c_str());
      transfer(from, to, amount, appeal_name.c_str(), memo);
      produce_blocks(1);
      auto arbis = get_arbitration_process(arbitration_id, round);
      uint64_t id = arbis["round"].as<uint8_t>();
      BOOST_TEST_REQUIRE(round == id);

      auto stakes = get_arbitration_stake_account(arbitration_id, from);
      auto balance = stakes["balance"].as<asset>();
      BOOST_TEST_REQUIRE(core_sym::from_string("1400.0000") == balance);
   }

   /// resp reappeal
   {
      std::string memo = "5,1,''";
      std::string provider_name = "appellants11";
      name from = name(provider_name.c_str());
      transfer(from, to, amount, provider_name.c_str(), memo);
      produce_blocks(1);
   }

   // test_accept_invitation(arbitration_id, round);

   /// reaccept invitation
   {
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      vector<name> arbivec = arbis["chosen_arbitrators"].as<vector<name>>();
      BOOST_TEST_REQUIRE(17 == arbivec.size());
      for (uint8_t i = 8; i < arbivec.size(); i++) {
         acceptarbi(arbivec[i], arbitration_id);
         produce_blocks(1);
      }
   }

   result = 2;
   /// reupload result
   {
      auto arbis = get_arbitration_process(arbitration_id, round);
      vector<name> arbivec = arbis["arbitrators"].as<vector<name>>();
      BOOST_TEST_REQUIRE(9 == arbivec.size());
      for (auto& arbi : arbivec) {
         uploadresult(arbi, arbitration_id, result, "");
         produce_blocks(1);
      }
   }

   /// get arbitration result
   {
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      uint64_t arbitration_result = arbis["arbitration_result"].as<uint8_t>();
      BOOST_TEST_REQUIRE(result == arbitration_result);
      uint64_t final_result = arbis["final_result"].as<uint8_t>();
      BOOST_TEST_REQUIRE(result == final_result);
      produce_blocks(1);
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_accept_timeout_test, bos_oracle_tester)
try {
   name to = N(oracle.bos);
   /// reg arbitrator
   reg_arbi();

   uint64_t service_id = reg_svc_for_arbi();
   uint64_t arbitration_id = service_id;
   uint8_t round = 1;
   std::string appeal_name = "appellants11";
   string amount = "200.0000";
   uint8_t role_type = 1; /// consumer
   /// appeal
   _appeal(arbitration_id, appeal_name, round, amount, role_type);

   /// resp appeal
   std::string provider_name = "provider1111";

   resp_appeal(arbitration_id, provider_name, amount);

   test_accept_invitation(arbitration_id, round);
   /// accept invitation
   // accept_invitation(arbitration_id, round);
   {
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      vector<name> arbivec = arbis["chosen_arbitrators"].as<vector<name>>();
      BOOST_TEST_REQUIRE(3 == arbivec.size());
      for (uint8_t i = 0; i < arbivec.size() - 1; i++) {
         acceptarbi(arbivec[i], arbitration_id);
         produce_blocks(1);
      }
   }
   produce_blocks(60 * 60 * 2 + 10);
   {
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      vector<name> arbivec = arbis["chosen_arbitrators"].as<vector<name>>();
      BOOST_TEST_REQUIRE(4 == arbivec.size());
      for (uint8_t i = 3; i < arbivec.size(); i++) {
         acceptarbi(arbivec[i], arbitration_id);
         produce_blocks(1);
      }
   }

   uint8_t result = 1;
   /// upload result
   upload_result(arbitration_id, round, result);

   produce_blocks(60 * 60 * 2 + 10);

   /// get arbitration result
   {
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      uint64_t arbitration_result = arbis["arbitration_result"].as<uint8_t>();
      BOOST_TEST_REQUIRE(result == arbitration_result);
      uint64_t final_result = arbis["final_result"].as<uint8_t>();
      BOOST_TEST_REQUIRE(result == final_result);
      produce_blocks(1);
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_upload_result_timeout_test, bos_oracle_tester)
try {
   name to = N(oracle.bos);
   /// reg arbitrator
   reg_arbi();

   uint64_t service_id = reg_svc_for_arbi();
   uint64_t arbitration_id = service_id;
   uint8_t round = 1;
   std::string appeal_name = "appellants11";
   string amount = "200.0000";
   uint8_t role_type = 1; /// consumer
   /// appeal
   _appeal(arbitration_id, appeal_name, round, amount, role_type);

   /// resp appeal
   std::string provider_name = "provider1111";

   resp_appeal(arbitration_id, provider_name, amount);

   /// accept invitation
   // accept_invitation(arbitration_id, round);
   {
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      vector<name> arbivec = arbis["chosen_arbitrators"].as<vector<name>>();
      BOOST_TEST_REQUIRE(3 == arbivec.size());
      for (uint8_t i = 0; i < arbivec.size(); i++) {
         acceptarbi(arbivec[i], arbitration_id);
         produce_blocks(1);
      }
   }
   uint8_t result = 1;
   /// upload result
   // upload_result(arbitration_id, round, result);
   {
      auto arbis = get_arbitration_process(arbitration_id, round);
      vector<name> arbivec = arbis["arbitrators"].as<vector<name>>();
      BOOST_TEST_REQUIRE(3 == arbivec.size());

      for (uint8_t i = 0; i < arbivec.size() - 1; i++) {
         uploadresult(arbivec[i], arbitration_id, i + 1, "");
         produce_blocks(1);
      }
   }

   produce_blocks(60 * 60 * 2 + 10);

   {
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      vector<name> arbivec = arbis["chosen_arbitrators"].as<vector<name>>();
      for (uint8_t i = 3; i < arbivec.size(); i++) {
         acceptarbi(arbivec[i], arbitration_id);
         produce_blocks(1);
      }
   }

   {
      auto arbis = get_arbitration_process(arbitration_id, round);
      vector<name> arbivec = arbis["arbitrators"].as<vector<name>>();
      for (uint8_t i = 3; i < arbivec.size(); i++) {
         uploadresult(arbivec[i], arbitration_id, result, "");
         produce_blocks(1);
      }
   }

   produce_blocks(60 * 60 * 2 + 10);

   /// get arbitration result
   {
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      uint64_t arbitration_result = arbis["arbitration_result"].as<uint8_t>();
      BOOST_TEST_REQUIRE(result == arbitration_result);
      uint64_t final_result = arbis["final_result"].as<uint8_t>();
      BOOST_TEST_REQUIRE(result == final_result);
      produce_blocks(1);
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_upload_result_same_timeout_test, bos_oracle_tester)
try {
   name to = N(oracle.bos);
   /// reg arbitrator
   reg_arbi();

   uint64_t service_id = reg_svc_for_arbi();
   uint64_t arbitration_id = service_id;
   uint8_t round = 1;
   std::string appeal_name = "appellants11";
   string amount = "200.0000";
   uint8_t role_type = 1; /// consumer
   /// appeal
   _appeal(arbitration_id, appeal_name, round, amount, role_type);

   /// resp appeal
   std::string provider_name = "provider1111";

   resp_appeal(arbitration_id, provider_name, amount);

   /// accept invitation
   // accept_invitation(arbitration_id, round);
   {
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      vector<name> arbivec = arbis["chosen_arbitrators"].as<vector<name>>();
      BOOST_TEST_REQUIRE(3 == arbivec.size());
      for (uint8_t i = 0; i < arbivec.size(); i++) {
         acceptarbi(arbivec[i], arbitration_id);
         produce_blocks(1);
      }
   }
   uint8_t result = 1;
   /// upload result
   // upload_result(arbitration_id, round, result);
   {
      auto arbis = get_arbitration_process(arbitration_id, round);
      vector<name> arbivec = arbis["arbitrators"].as<vector<name>>();
      BOOST_TEST_REQUIRE(3 == arbivec.size());

      for (uint8_t i = 0; i < arbivec.size() - 1; i++) {
         uploadresult(arbivec[i], arbitration_id, result, "");
         produce_blocks(1);
      }
   }

   produce_blocks(60 * 60 * 2 + 10);

   produce_blocks(60 * 60 * 2 + 10);
   /// get arbitration result
   {
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      uint64_t arbitration_result = arbis["arbitration_result"].as<uint8_t>();
      BOOST_TEST_REQUIRE(result == arbitration_result);
      uint64_t final_result = arbis["final_result"].as<uint8_t>();
      BOOST_TEST_REQUIRE(result == final_result);
      produce_blocks(1);
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_unchosen_arbitrator_test, bos_oracle_tester)
try {
   name to = N(oracle.bos);
   /// reg arbitrator
   reg_arbi();

   uint64_t service_id = reg_svc_for_arbi();
   uint64_t arbitration_id = service_id;
   uint8_t round = 1;
   std::string appeal_name = "appellants11";
   string amount = "200.0000";
   uint8_t role_type = 1; /// consumer
   /// appeal
   _appeal(arbitration_id, appeal_name, round, amount, role_type);

   /// resp appeal
   std::string provider_name = "provider1111";

   resp_appeal(arbitration_id, provider_name, amount);

   /// accept invitation
   {
      //  BOOST_REQUIRE_EXCEPTION( acceptarbi(N(alice), arbitration_id),
      //                    eosio_assert_message_exception, eosio_assert_message_is("could not find such an arbitrator in current chosen arbitration." ) );
      BOOST_REQUIRE_EQUAL(wasm_assert_msg("could not find such an arbitrator in current chosen arbitration."), acceptarbi(N(alice), arbitration_id));
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_uninvited_arbitrator_test, bos_oracle_tester)
try {
   name to = N(oracle.bos);
   /// reg arbitrator
   reg_arbi();

   uint64_t service_id = reg_svc_for_arbi();
   uint64_t arbitration_id = service_id;
   uint8_t round = 1;
   std::string appeal_name = "appellants11";
   string amount = "200.0000";
   uint8_t role_type = 1; /// consumer
   /// appeal
   _appeal(arbitration_id, appeal_name, round, amount, role_type);

   /// resp appeal
   std::string provider_name = "provider1111";

   resp_appeal(arbitration_id, provider_name, amount);

   /// accept invitation
   accept_invitation(arbitration_id, round);

   uint8_t result = 1;
   /// reupload result
   {
      //  BOOST_REQUIRE_EXCEPTION( acceptarbi(N(alice), arbitration_id),
      //                    eosio_assert_message_exception, eosio_assert_message_is("could not find such an arbitrator in current chosen arbitration." ) );
      BOOST_REQUIRE_EQUAL(wasm_assert_msg("could not find such an arbitrator in current invited arbitrators."), uploadresult(N(alice), arbitration_id, result, ""));
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_claimarbi_test, bos_oracle_tester)
try {
   name to = N(oracle.bos);
   /// reg arbitrator
   reg_arbi();
   uint64_t service_id = reg_svc_for_arbi();
   uint64_t arbitration_id = service_id;
   uint8_t round = 1;
   std::string appeal_name = "appellants11";
   string amount = "200.0003";
   uint8_t role_type = 1; /// consumer
   /// appeal
   _appeal(arbitration_id, appeal_name, round, amount, role_type);

   /// resp appeal
   std::string provider_name = "provider1111";
   resp_appeal(arbitration_id, provider_name, amount);

   /// accept invitation
   accept_invitation(arbitration_id, round);
   uint8_t result = 1;
   /// upload result
   upload_result(arbitration_id, round, result);
   produce_blocks(60 * 60 * 2 + 10);
   /// get final result
   {
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      uint64_t final_result = arbis["final_result"].as<uint8_t>();
      get_arbitration_income_account(N(appellants11), "4,BOS");
      BOOST_TEST_REQUIRE(result == final_result);
      produce_blocks(1);
   }

   uint8_t arbi_count = pow(2, round) + 1;
   auto arbis = get_arbitration_process(arbitration_id, round);
   vector<name> arbivec = arbis["arbitrators"].as<vector<name>>();
   BOOST_TEST_REQUIRE(arbi_count == arbivec.size());

   string acc = (*arbivec.begin()).to_string();
   BOOST_TEST(core_sym::from_string("10000.0002") == get_balance(acc));
   {
      name account = name(acc.c_str());
      name receive_account = account;
      auto token = claimarbi(arbitration_id, account, receive_account);

      BOOST_REQUIRE_EQUAL(core_sym::from_string("10066.6669"), get_balance(acc));
   }

   BOOST_TEST(core_sym::from_string("19799.9999") == get_balance(appeal_name));
   {
      name account = name(appeal_name.c_str());
      name receive_account = account;
      auto token = claimarbi(arbitration_id, account, receive_account);

      BOOST_REQUIRE_EQUAL(core_sym::from_string("24799.9999"), get_balance(appeal_name));
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_claimarbi_provider_win_test, bos_oracle_tester)
try {
   name to = N(oracle.bos);
   /// reg arbitrator
   reg_arbi();
   uint64_t service_id = reg_svc_for_arbi();
   uint64_t arbitration_id = service_id;
   uint8_t round = 1;
   std::string appeal_name = "appellants11";
   string amount = "200.0003";
   uint8_t role_type = 1; /// consumer
   /// appeal
   _appeal(arbitration_id, appeal_name, round, amount, role_type);

   /// resp appeal
   std::string provider_name = "provider1111";
   resp_appeal(arbitration_id, provider_name, amount);

   /// accept invitation
   accept_invitation(arbitration_id, round);
   uint8_t result = 2;
   /// upload result
   upload_result(arbitration_id, round, result);
   produce_blocks(60 * 60 * 2 + 10);
   /// get final result
   {
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      uint64_t final_result = arbis["final_result"].as<uint8_t>();
      get_arbitration_income_account(N(appellants11), "4,BOS");
      BOOST_TEST_REQUIRE(result == final_result);
      produce_blocks(1);
   }

   uint8_t arbi_count = pow(2, round) + 1;
   auto arbis = get_arbitration_process(arbitration_id, round);
   vector<name> arbivec = arbis["arbitrators"].as<vector<name>>();
   BOOST_TEST_REQUIRE(arbi_count == arbivec.size());

   string acc = (*arbivec.begin()).to_string();
   BOOST_TEST(core_sym::from_string("10000.0002") == get_balance(acc));
   {
      name account = name(acc.c_str());
      name receive_account = account;
      auto token = claimarbi(arbitration_id, account, receive_account);

      BOOST_REQUIRE_EQUAL(core_sym::from_string("10066.6669"), get_balance(acc));
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_unstakearbi_test, bos_oracle_tester)
try {
   name to = N(oracle.bos);
   /// reg arbitrator
   reg_arbi();

   uint64_t service_id = reg_svc_for_arbi();
   uint64_t arbitration_id = service_id;
   uint8_t round = 1;
   std::string appeal_name = "appellants11";
   string amount = "200.0000";
   uint8_t role_type = 1; /// consumer
   /// appeal
   _appeal(arbitration_id, appeal_name, round, amount, role_type);

   /// resp appeal
   std::string provider_name = "provider1111";

   resp_appeal(arbitration_id, provider_name, amount);

   /// accept invitation
   accept_invitation(arbitration_id, round);

   uint8_t result = 1;
   /// upload result
   upload_result(arbitration_id, round, result);

   produce_blocks(60 * 60 * 2 + 10);
   /// get final result
   {
      uint8_t result = 1;
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      uint64_t final_result = arbis["final_result"].as<uint8_t>();
      BOOST_TEST_REQUIRE(result == final_result);
      produce_blocks(1);
   }

   string acc = appeal_name;
   // BOOST_TESTcore_sym::from_string("19800.0002") == get_balance(acc));
   {

      name account = name(acc.c_str());
      push_permission_update_auth_action(N(arbitrat.bos));
      auto token = unstakearbi(arbitration_id, account, core_sym::from_string(amount), "");

      BOOST_REQUIRE_EQUAL(core_sym::from_string("20000.0002"), get_balance(acc));
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_uploadeviden_test, bos_oracle_tester)
try {
   name to = N(oracle.bos);
   /// reg arbitrator
   reg_arbi();

   uint64_t service_id = reg_svc_for_arbi();
   uint64_t arbitration_id = service_id;
   uint8_t round = 1;
   std::string appeal_name = "appellants11";
   string amount = "200.0000";
   uint8_t role_type = 1; /// consumer
   /// appeal
   _appeal(arbitration_id, appeal_name, round, amount, role_type);

   name from = name(appeal_name.c_str());

   /// upload evidence
   {
      uploadeviden(from, arbitration_id, "evidence ipfs hash link");
      produce_blocks(1);
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_arbi_freeze_stake_test, bos_oracle_tester)
try {
   name to = N(oracle.bos);
   /// reg arbitrator
   reg_arbi();

   uint64_t service_id = reg_svc_for_arbi();
   uint64_t arbitration_id = service_id;
   uint8_t round = 1;
   std::string appeal_name = "appellants11";
   string amount = "200.0000";
   uint8_t role_type = 1; /// consumer

   name provider_name = N(provider1111);

   // BOOST_TESTcore_sym::from_string("0.0000") == get_data_service_provision(service_id, provider_name)["freeze_amount"].as<asset>());
   // BOOST_TESTcore_sym::from_string("0.0000") == get_data_provider(provider_name)["total_freeze_amount"].as<asset>());
   // //BOOST_TESTcore_sym::from_string("19800.0002") == get_balance(acc));

   /// appeal
   _appeal(arbitration_id, appeal_name, round, amount, role_type);

   //  BOOST_REQUIRE_EQUAL(core_sym::from_string("20000.0002"), get_balance(acc));
   BOOST_TEST_REQUIRE(core_sym::from_string("40.0000") == get_data_service_provision(service_id, provider_name)["freeze_amount"].as<asset>());
   BOOST_TEST_REQUIRE(core_sym::from_string("40.0000") == get_data_provider(provider_name)["total_freeze_amount"].as<asset>());
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_freeze_stake_large_test, bos_oracle_tester)
try {
   name to = N(oracle.bos);
   /// reg arbitrator
   reg_arbi();

   uint64_t service_id = reg_svc_for_arbi();
   uint64_t arbitration_id = service_id;
   uint8_t round = 1;
   std::string appeal_name = "appellants11";
   string amount = "5000.0000";
   uint8_t role_type = 1; /// consumer

   name provider_name = N(provider1111);

   // BOOST_TEST(core_sym::from_string("0.0000") == get_data_service_provision(service_id, provider_name)["freeze_amount"].as<asset>());
   // BOOST_TEST(core_sym::from_string("0.0000") == get_data_provider(provider_name)["total_freeze_amount"].as<asset>());

   // //BOOST_TESTcore_sym::from_string("19800.0002") == get_balance(acc));

   /// appeal
   // _appeal(arbitration_id, appeal_name, round, amount, role_type);
   BOOST_REQUIRE_EXCEPTION(_appeal(arbitration_id, appeal_name, round, amount, role_type), eosio_assert_message_exception, eosio_assert_message_is("service status is not service_in when notify"));
   // BOOST_REQUIRE_EQUAL(wasm_assert_msg("no provider"), _appeal(arbitration_id, appeal_name, round, amount, role_type));
   // BOOST_TEST(core_sym::from_string("0.0000") == get_data_service_provision(service_id, provider_name)["freeze_amount"].as<asset>());
   // BOOST_TEST(core_sym::from_string("0.0000") == get_data_provider(provider_name)["total_freeze_amount"].as<asset>());

   //  BOOST_REQUIRE_EQUAL(core_sym::from_string("20000.0002"), get_balance(acc));
   // BOOST_TEST_REQUIRE(core_sym::from_string("1000.0000") == get_data_service_provision(service_id, provider_name)["freeze_amount"].as<asset>());
   // BOOST_TEST_REQUIRE(core_sym::from_string("1000.0000") == get_data_provider(provider_name)["total_freeze_amount"].as<asset>());
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(arbi_unfreeze_asset_test, bos_oracle_tester)
try {

   name to = N(oracle.bos);
   /// reg arbitrator
   reg_arbi();

   uint64_t service_id = reg_svc_for_arbi();
   uint64_t arbitration_id = service_id;
   uint8_t round = 1;
   std::string appeal_name = "appellants11";
   string amount = "200.0000";
   uint8_t role_type = 1; /// consumer
   name provider_name = N(provider1111);

   BOOST_TEST(core_sym::from_string("0.0000") == get_data_service_provision(service_id, provider_name)["freeze_amount"].template as<asset>());
   BOOST_TEST(core_sym::from_string("0.0000") == get_data_provider(provider_name)["total_freeze_amount"].template as<asset>());

   /// appeal
   _appeal(arbitration_id, appeal_name, round, amount, role_type);

   BOOST_TEST(core_sym::from_string("40.0000") == get_data_service_provision(service_id, provider_name)["freeze_amount"].template as<asset>());
   BOOST_TEST(core_sym::from_string("40.0000") == get_data_provider(provider_name)["total_freeze_amount"].template as<asset>());
   /// resp appeal

   resp_appeal(arbitration_id, provider_name.to_string(), amount);

   /// accept invitation
   accept_invitation(arbitration_id, round);

   uint8_t result = 2;

   /// upload result
   upload_result(arbitration_id, round, result);

   produce_blocks(60 * 60 * 2 + 10);

   /// get final result
   {
      // get_result(arbitration_id, result);
      // BOOST_TEST0 == arbitration_id);
      uint64_t service_id = arbitration_id;
      auto arbis = get_arbitration_case(service_id, arbitration_id);
      uint8_t final_result = arbis["final_result"].as<uint8_t>();
      BOOST_TEST_REQUIRE(result == final_result);
      produce_blocks(1);
   }

   BOOST_TEST(core_sym::from_string("0.0000") == get_data_service_provision(service_id, provider_name)["freeze_amount"].template as<asset>());
   BOOST_TEST(core_sym::from_string("0.0000") == get_data_provider(provider_name)["total_freeze_amount"].template as<asset>());

   // //  BOOST_REQUIRE_EQUAL(core_sym::from_string("20000.0002"), get_balance(acc));
   // BOOST_TEST_REQUIRE(core_sym::from_string("40.0000") == get_data_service_provision(service_id, provider_name)["freeze_amount"].as<asset>());
   // BOOST_TEST_REQUIRE(core_sym::from_string("40.0000") == get_data_provider(provider_name)["total_freeze_amount"].as<asset>());
}
FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
