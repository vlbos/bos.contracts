#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
// #include "eosio.system_tester.hpp"
#include "contracts.hpp"
#include "test_symbol.hpp"

#include "Runtime/Runtime.h"

#include <fc/variant_object.hpp>

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace fc;
using namespace std;

using mvo = fc::mutable_variant_object;

class bos_oracle_tester : public tester {
public:

   bos_oracle_tester() {
      produce_blocks( 2 );
  create_accounts({ N(eosio.token), N(eosio.ram), N(eosio.ramfee), N(eosio.stake),
               N(eosio.bpay), N(eosio.vpay), N(eosio.saving), N(eosio.names), N(eosio.rex) });
      create_accounts( { N(alice), N(bob), N(carol), N(dapp), N(dappuser),N(oracle.bos),N(dappuser.bos),N(provider.bos),N(consumer.bos),N(riskctrl.bos)} );
      produce_blocks( 2 );

      set_code( N(oracle.bos), contracts::oracle_wasm() );
      set_abi( N(oracle.bos), contracts::oracle_abi().data() );
      set_code( N(dappuser.bos), contracts::dappuser_wasm() );
      set_abi( N(dappuser.bos), contracts::dappuser_abi().data() );

      produce_blocks();

      const auto& accnt = control->db().get<account_object,by_name>( N(oracle.bos) );
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_ser.set_abi(abi, abi_serializer_max_time);


   set_code( N(eosio.token), contracts::token_wasm() );
   set_abi( N(eosio.token), contracts::token_abi().data() );

   create_currency( N(eosio.token), config::system_account_name, core_sym::from_string("10000000000.0000") );
   issue(config::system_account_name, core_sym::from_string("1000000000.0000"));
   BOOST_REQUIRE_EQUAL( core_sym::from_string("1000000000.0000"),
                        get_balance("eosio") + get_balance("eosio.ramfee") + get_balance("eosio.stake") + get_balance("eosio.ram") );

   set_code( config::system_account_name, contracts::system_wasm() );
   set_abi( config::system_account_name, contracts::system_abi().data() );
   base_tester::push_action( config::system_account_name, N(init),
                             config::system_account_name,  mutable_variant_object()
                              ("version", 0)
                              ("core", CORE_SYM_STR)
   );
   produce_blocks();
   create_account_with_resources( N(alice1111111), N(eosio), core_sym::from_string("1.0000"), false );
   create_account_with_resources( N(bob111111111), N(eosio), core_sym::from_string("0.4500"), false );
   create_account_with_resources( N(carol1111111), N(eosio), core_sym::from_string("1.0000"), false );
   create_account_with_resources(N(provider1111), N(eosio),
                                 core_sym::from_string("1.0000"), false);
   create_account_with_resources(N(provider2222), N(eosio),
                                 core_sym::from_string("1.0000"), false);
   create_account_with_resources(N(provider3333), N(eosio),
                                 core_sym::from_string("1.0000"), false);
   create_account_with_resources(N(provider4444), N(eosio),
                                 core_sym::from_string("1.0000"), false);
   create_account_with_resources(N(provider5555), N(eosio),
                                 core_sym::from_string("1.0000"), false);
   create_account_with_resources(N(consumer1111), N(eosio),
                                 core_sym::from_string("0.4500"), false);
   create_account_with_resources(N(consumer2222), N(eosio),
                                 core_sym::from_string("0.4500"), false);
   create_account_with_resources(N(consumer3333), N(eosio),
                                 core_sym::from_string("0.4500"), false);
   create_account_with_resources(N(consumer4444), N(eosio),
                                 core_sym::from_string("0.4500"), false);
   create_account_with_resources(N(consumer5555), N(eosio),
                                 core_sym::from_string("0.4500"), false);

   create_account_with_resources(N(conconsumer1), N(eosio),
                                 core_sym::from_string("110.4500"), false);
   create_account_with_resources(N(conconsumer2), N(eosio),
                                 core_sym::from_string("110.4500"), false);
   create_account_with_resources(N(conconsumer3), N(eosio),
                                 core_sym::from_string("110.4500"), false);
   create_account_with_resources(N(conconsumer4), N(eosio),
                                 core_sym::from_string("110.4500"), false);
   create_account_with_resources(N(conconsumer5), N(eosio),
                                 core_sym::from_string("110.4500"), false);
 
   transfer("eosio", "alice1111111", ("1000.0000"), "eosio");
   transfer("eosio", "bob111111111", ("1000.0000"), "eosio");
   transfer("eosio", "carol1111111", ("1000.0000"), "eosio");
   transfer("eosio", "provider1111", ("1000.0000"), "eosio");
   transfer("eosio", "provider2222", ("1000.0000"), "eosio");
   transfer("eosio", "provider3333", ("1000.0000"), "eosio");
   transfer("eosio", "provider4444", ("1000.0000"), "eosio");
   transfer("eosio", "provider5555", ("1000.0000"), "eosio");
   transfer("eosio", "consumer1111", ("1000.0000"), "eosio");
   transfer("eosio", "consumer2222", ("1000.0000"), "eosio");
   transfer("eosio", "consumer3333", ("1000.0000"), "eosio");
   transfer("eosio", "consumer4444", ("1000.0000"), "eosio");
   transfer("eosio", "consumer5555", ("1000.0000"), "eosio");

   transfer("eosio", "conconsumer1", ("1000.0000"), "eosio");
   transfer("eosio", "conconsumer2", ("1000.0000"), "eosio");
   transfer("eosio", "conconsumer3", ("1000.0000"), "eosio");
   transfer("eosio", "conconsumer4", ("1000.0000"), "eosio");
   transfer("eosio", "conconsumer5", ("1000.0000"), "eosio");

   transfer("eosio", "alice", ("1000.0000"), "eosio");
   transfer("eosio", "bob", ("1000.0000"), "eosio");
   transfer("eosio", "carol", ("1000.0000"), "eosio");
   transfer("eosio", "dappuser.bos", ("1000.0000"), "eosio");
   transfer("eosio", "dappuser", ("1000.0000"), "eosio");
   transfer("eosio", "dapp", ("1000.0000"), "eosio");


   set_code(N(conconsumer1), contracts::dappuser_wasm());
   set_abi(N(conconsumer1), contracts::dappuser_abi().data());
   set_code(N(conconsumer2), contracts::dappuser_wasm());
   set_abi(N(conconsumer2), contracts::dappuser_abi().data());
   set_code(N(conconsumer3), contracts::dappuser_wasm());
   set_abi(N(conconsumer3), contracts::dappuser_abi().data());
   set_code(N(conconsumer4), contracts::dappuser_wasm());
   set_abi(N(conconsumer4), contracts::dappuser_abi().data());
   set_code(N(conconsumer5), contracts::dappuser_wasm());
   set_abi(N(conconsumer5), contracts::dappuser_abi().data());

   }

   transaction_trace_ptr create_account_with_resources( account_name a, account_name creator, asset ramfunds, bool multisig,
                                                        asset net = core_sym::from_string("10.0000"), asset cpu = core_sym::from_string("10.0000") ) {
      signed_transaction trx;
      set_transaction_headers(trx);

      authority owner_auth;
      if (multisig) {
         // multisig between account's owner key and creators active permission
         owner_auth = authority(2, {key_weight{get_public_key( a, "owner" ), 1}}, {permission_level_weight{{creator, config::active_name}, 1}});
      } else {
         owner_auth =  authority( get_public_key( a, "owner" ) );
      }

      trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                newaccount{
                                   .creator  = creator,
                                   .name     = a,
                                   .owner    = owner_auth,
                                   .active   = authority( get_public_key( a, "active" ) )
                                });

      trx.actions.emplace_back( get_action( N(eosio), N(buyram), vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("payer", creator)
                                            ("receiver", a)
                                            ("quant", ramfunds) )
                              );

      trx.actions.emplace_back( get_action( N(eosio), N(delegatebw), vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("from", creator)
                                            ("receiver", a)
                                            ("stake_net_quantity", net )
                                            ("stake_cpu_quantity", cpu )
                                            ("transfer", 0 )
                                          )
                                );

      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );
      return push_transaction( trx );
   }
   void create_currency( name contract, name manager, asset maxsupply ) {
      auto act =  mutable_variant_object()
         ("issuer",       manager )
         ("maximum_supply", maxsupply );

      base_tester::push_action(contract, N(create), contract, act );
   }
   void issue( name to, const asset& amount, name manager = config::system_account_name ) {
      base_tester::push_action( N(eosio.token), N(issue), manager, mutable_variant_object()
                                ("to",      to )
                                ("quantity", amount )
                                ("memo", "")
                                );
   }
   void transfer( name from, name to, const string& amount, name manager = config::system_account_name ) {
      base_tester::push_action( N(eosio.token), N(transfer), manager, mutable_variant_object()
                                ("from",    from)
                                ("to",      to )
                                ("quantity", core_sym::from_string(amount) )
                                ("memo", "")
                                );
   }

   asset get_balance( const account_name& act ) {
      //return get_currency_balance( config::system_account_name, symbol(CORE_SYMBOL), act );
      //temporary code. current get_currency_balancy uses table name N(accounts) from currency.h
      //generic_currency table name is N(account).
      const auto& db  = control->db();
      const auto* tbl = db.find<table_id_object, by_code_scope_table>(boost::make_tuple(N(eosio.token), act, N(accounts)));
      share_type result = 0;

      // the balance is implied to be 0 if either the table or row does not exist
      if (tbl) {
         const auto *obj = db.find<key_value_object, by_scope_primary>(boost::make_tuple(tbl->id, symbol(CORE_SYM).to_symbol_code()));
         if (obj) {
            // balance is the first field in the serialization
            fc::datastream<const char *> ds(obj->value.data(), obj->value.size());
            fc::raw::unpack(ds, result);
         }
      }
      return asset( result, symbol(CORE_SYM) );
   }

  ///dappuser.bos
    void fetchdata(uint64_t service_id,uint64_t update_number){
      base_tester::push_action( N(dappuser.bos), N(fetchdata),  N(dappuser.bos), mutable_variant_object()
                                ("oracle",    "oracle.bos")
                                ("service_id",    service_id)
                                ("update_number",      update_number )
                                 );
   }


   action_result push_action( const account_name& signer, const action_name &name, const variant_object &data ) {
      string action_type_name = abi_ser.get_action_type(name);

      action act;
      act.account = N(oracle.bos);
      act.name    = name;
      act.data    = abi_ser.variant_to_binary( action_type_name, data,abi_serializer_max_time );

      return base_tester::push_action( std::move(act), uint64_t(signer));
   }

   auto push_permission_update_auth_action( const account_name& signer ) 
   {
       auto auth = authority(eosio::testing::base_tester::get_public_key(signer, "active"));
   auth.accounts.push_back( permission_level_weight{{N(oracle.bos), config::eosio_code_name}, 1} );

    return base_tester::push_action(N(eosio), N(updateauth), signer, mvo()
      ( "account", signer )
      ( "permission", "active" )
      ( "parent", "owner" )
      ( "auth", auth )
   );

   }
 
   //provider
   fc::variant get_data_service( const uint64_t& service_id )
   {
      vector<char> data = get_row_by_account( N(oracle.bos), N(oracle.bos), N(dataservices), service_id );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "data_service", data, abi_serializer_max_time );
   }

   fc::variant get_data_service_fee( const uint64_t& service_id , const uint8_t& fee_type)
   {
      vector<char> data = get_row_by_account( N(oracle.bos), service_id, N(servicefees), fee_type );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "data_service_fee", data, abi_serializer_max_time );
   }

   fc::variant get_data_provider( const name& account )
   {
      vector<char> data = get_row_by_account( N(oracle.bos), N(oracle.bos), N(providers), account );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "data_provider", data, abi_serializer_max_time );
   }

   fc::variant get_provider_service( const name& account,const uint64_t& create_time_sec )
   {
      vector<char> data = get_row_by_account( N(oracle.bos), account, N(provservices), create_time_sec );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "provider_service", data, abi_serializer_max_time );
   }

   uint64_t get_provider_service_id( const name& account,const uint64_t& create_time_sec )
   {
      vector<char> data = get_row_by_account( N(oracle.bos), account, N(provservices), create_time_sec );
      return data.empty() ? 0 : abi_ser.binary_to_variant( "provider_service", data, abi_serializer_max_time )["service_id"].as<uint64_t>();
   }

   fc::variant get_data_service_provision( const uint64_t& service_id,const name& account)
   {
      vector<char> data = get_row_by_account( N(oracle.bos), service_id, N(svcprovision), account );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "data_service_provision", data, abi_serializer_max_time );
   }

   fc::variant get_svc_provision_cancel_apply( const uint64_t& service_id , const name& provider)
   {
      vector<char> data = get_row_by_account( N(oracle.bos), service_id, N(cancelapplys), provider );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "svc_provision_cancel_apply", data, abi_serializer_max_time );
   }

   fc::variant get_data_service_provision_log( const uint64_t& service_id , const uint64_t& log_id)
   {
      vector<char> data = get_row_by_account( N(oracle.bos), service_id, N(provisionlog), log_id );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "data_service_provision_log", data, abi_serializer_max_time );
   }

    fc::variant get_push_record( const uint64_t& service_id )
   {
      vector<char> data = get_row_by_account( N(oracle.bos), N(oracle.bos), N(pushrecords), service_id );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "push_record", data, abi_serializer_max_time );
   }

   fc::variant get_provider_push_record( const uint64_t& service_id , const name& account)
   {
      vector<char> data = get_row_by_account( N(oracle.bos), service_id, N(ppushrecords), account );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "provider_push_record", data, abi_serializer_max_time );
   }

   fc::variant get_action_push_record( const uint64_t& service_id, uint64_t key)
   {
      // uint64_t key = get_hash_key(get_nn_hash( contract_account, action_name);
      vector<char> data = get_row_by_account( N(oracle.bos), service_id, N(apushrecords), key );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "action_push_record", data, abi_serializer_max_time );
   }

   fc::variant get_provider_action_push_record( const uint64_t& service_id , uint64_t key )
   {
      //   uint64_t key = get_hash_key(get_nnn_hash(account, contract_account, action_name);
      vector<char> data = get_row_by_account( N(oracle.bos), service_id, N(papushrecord), key );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "provider_action_push_record", data, abi_serializer_max_time );
   }
//consumer
   fc::variant get_data_consumer(  const name& account)
   {
      vector<char> data = get_row_by_account( N(oracle.bos), N(oracle.bos), N(dataconsumer), account );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "data_consumer", data, abi_serializer_max_time );
   }

   fc::variant get_data_service_subscription( const uint64_t& service_id, const name& contract_account )
   {
      // uint64_t key = get_hash_key(get_nn_hash( contract_account, action_name);
      vector<char> data = get_row_by_account( N(oracle.bos), service_id, N(subscription), contract_account );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "data_service_subscription", data, abi_serializer_max_time );
   }

   fc::variant get_data_service_request( const uint64_t& service_id , const uint64_t& request_id)
   {
      vector<char> data = get_row_by_account( N(oracle.bos), service_id, N(request), request_id );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "data_service_request", data, abi_serializer_max_time );
   }

   fc::variant get_data_service_usage_record( const uint64_t& service_id , const uint64_t& usage_id)
   {
      vector<char> data = get_row_by_account( N(oracle.bos), service_id, N(usagerecords), usage_id );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "data_service_usage_record", data, abi_serializer_max_time );
   }


   fc::variant get_service_consumptions( const uint64_t& service_id )
   {
      vector<char> data = get_row_by_account( N(oracle.bos), service_id, N(consumptions), service_id );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "service_consumptions", data, abi_serializer_max_time );
   }
//risk control
   fc::variant get_riskcontrol_account( account_name acc, const string& symbolname)
   {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      ilog("============code:${s}:acc:${a};c${o}",("s",symbol_code)("a",acc.value)("o",N(oracle.bos)) );
      vector<char> data = get_row_by_account( N(oracle.bos), acc, N(riskaccounts), symbol_code );
      if(data.empty())
      {
          ilog("=====empty=======code:${s}:acc:${a}",("s",symbol_code)("a",acc.value) );
      }
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "riskcontrol_account", data, abi_serializer_max_time );
   }

   fc::variant get_data_service_stake( const uint64_t& service_id )
   {
      vector<char> data = get_row_by_account( N(oracle.bos), service_id, N(servicestake), service_id );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "data_service_stake", data, abi_serializer_max_time );
   }

   fc::variant get_transfer_freeze_delay( const uint64_t& service_id , const uint64_t& transfer_id)
   {
      vector<char> data = get_row_by_account( N(oracle.bos), service_id, N(freezedelays), transfer_id );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "transfer_freeze_delay", data, abi_serializer_max_time );
   }

   fc::variant get_risk_guarantee( const uint64_t& service_id , const uint64_t& risk_id)
   {
      vector<char> data = get_row_by_account( N(oracle.bos), service_id, N(riskguarante), risk_id );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "risk_guarantee", data, abi_serializer_max_time );
   }

   fc::variant get_account_freeze_log( const uint64_t& service_id , const uint64_t& log_id)
   {
      vector<char> data = get_row_by_account( N(oracle.bos), service_id, N(freezelog), log_id );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "account_freeze_log", data, abi_serializer_max_time );
   }

  fc::variant get_account_freeze_stat( const uint64_t& service_id , const name& account)
   {
      vector<char> data = get_row_by_account( N(oracle.bos), service_id, N(freezestats), account );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "account_freeze_stat", data, abi_serializer_max_time );
   }

   fc::variant get_service_freeze_stat(const uint64_t &service_id) {
     vector<char> data = get_row_by_account(N(oracle.bos), service_id,
                                            N(svcfrozestat), service_id);
     return data.empty() ? fc::variant()  : abi_ser.binary_to_variant("service_freeze_stat", data, abi_serializer_max_time);
   }

   fc::variant get_oracle_data(const uint64_t& service_id,
                               const uint64_t& update_number) {
     vector<char> data = get_row_by_account(N(oracle.bos), service_id, N(oracledata), update_number);
     return data.empty() ? fc::variant()  : abi_ser.binary_to_variant("oracle_data_record", data, abi_serializer_max_time);
   }

//provider
   action_result regservice(
      uint64_t service_id, name account, asset amount,
      asset service_price, uint64_t fee_type, std::string data_format,
      uint64_t data_type, std::string criteria, uint64_t acceptance,
      std::string declaration, uint64_t injection_method,
      uint64_t duration, uint64_t provider_limit, uint64_t update_cycle,
      time_point_sec update_start_time) {

      return push_action( account, N(regservice), mvo()
           ( "service_id", service_id)
           ( "account", account)
           ( "amount", amount)
           ( "service_price", service_price)
           ( "fee_type", fee_type)
           ( "data_format", data_format)
           ( "data_type", data_type)
           ( "criteria", criteria)
           ( "acceptance", acceptance)
           ( "declaration", declaration)
           ( "injection_method", injection_method)
           ( "duration", duration)
           ( "provider_limit", provider_limit)
           ( "update_cycle", update_cycle)
           ( "update_start_time", update_start_time)
      );
   }

   action_result unregservice(uint64_t service_id,
                                      name account,
                                      uint64_t status) {
      return push_action(  account, N(unregservice), mvo()
           ( "service_id", service_id)
           ( "account", account)
           ( "status", status)
      );
   }

   action_result execaction(uint64_t service_id, uint64_t action_type) {
      return push_action( N(oracle.bos), N(execaction), mvo()
           ( "service_id", service_id)
           ( "action_type", action_type)
      );

   }

   action_result stakeasset( uint64_t service_id, 
                                     name account, 
                                     asset amount,
                                     string memo){
      return push_action( account, N(stakeasset), mvo()
           ( "service_id", service_id)
           ( "account", account)
           ( "amount", amount)
           ( "memo", memo)
      );
   }

   action_result pushdata(uint64_t service_id, name provider,
                                  name contract_account, name action_name,
                                   uint64_t request_id,const string& data_json){
      return push_action( provider, N(pushdata), mvo()
           ( "service_id", service_id )
           ( "provider", provider )
           ( "contract_account", contract_account )
           ( "action_name", action_name )
           ( "request_id", request_id)
           ( "data_json", data_json )
      );
   }

  action_result innerpush(uint64_t service_id, name provider,
                                  name contract_account, name action_name,
                                   uint64_t request_id,const string& data_json){
      return push_action( provider, N(innerpush), mvo()
           ( "service_id", service_id )
           ( "provider", provider )
           ( "contract_account", contract_account )
           ( "action_name", action_name )
           ( "request_id", request_id)
           ( "data_json", data_json )
      );
   }

   action_result multipush(uint64_t service_id, name provider,
                                  const string& data_json, bool is_request){
      return push_action( provider, N(multipush), mvo()
           ( "service_id", service_id )
           ( "provider", provider )
           ( "data_json", data_json )
           ( "is_request", is_request )
      );
   }

 action_result publishdata(uint64_t service_id, name provider,
                                  uint64_t update_number,
                                   uint64_t request_id,const string& data_json){
      return push_action( provider, N(publishdata), mvo()
           ( "service_id", service_id )
           ( "provider", provider )
           ( "update_number", update_number )
           ( "request_id", request_id)
           ( "data_json", data_json )
      );
   }

  action_result innerpublish(uint64_t service_id, name provider,
                                  uint64_t update_number,
                                   uint64_t request_id,const string& data_json){
      return push_action( provider, N(innerpublish), mvo()
           ( "service_id", service_id )
           ( "provider", provider )
           ( "update_number", update_number )
           ( "request_id", request_id)
           ( "data_json", data_json )
      );
   }

   action_result multipublish(uint64_t service_id, name provider,
                                  const string& data_json, bool is_request){
      return push_action( provider, N(multipublish), mvo()
           ( "service_id", service_id )
           ( "provider", provider )
           ( "data_json", data_json )
           ( "is_request", is_request )
      );
   }

   action_result starttimer(){
      return push_action( N(oracle.bos), N(starttimer), mvo());
   }

   action_result addfeetypes(uint64_t service_id,
                                    std::vector<uint8_t> fee_types,
                                    std::vector<asset> service_prices) {
      return push_action( N(oracle.bos), N(addfeetypes), mvo()
           ( "service_id", service_id )
           ( "fee_types", fee_types )
           ( "service_prices", service_prices )
      );
   }

   action_result addfeetype(uint64_t service_id,
                                    uint8_t fee_type,
                                    asset service_price) {
      return push_action( N(oracle.bos), N(addfeetype), mvo()
           ( "service_id", service_id )
           ( "fee_type", fee_type )
           ( "service_price", service_price )
      );
   }

  action_result claim(name account, name receive_account) {
      return push_action( account, N(claim), mvo()
           ( "account", account )
           ( "receive_account", receive_account )
      );
   }
//consumer
   action_result subscribe(uint64_t service_id, name contract_account,
                           name action_name, std::string publickey,
                           name account, asset amount, std::string memo) {
     return push_action(
         account, N(subscribe),mvo()
         ("service_id", service_id)
         ("contract_account", contract_account)
         ("action_name", action_name)
         ("publickey", publickey)
         ("account", account)
         ("amount", amount)
         ("memo", memo)
         );
   }

   action_result requestdata(uint64_t service_id, name contract_account,
                                     name action_name, name requester,
                                     std::string request_content) {
     return push_action(
         requester, N(requestdata),mvo()
         ("service_id", service_id)
         ("contract_account", contract_account)
         ("action_name", action_name)
         ("requester", requester)
         ("request_content", request_content)
         );
   }

   action_result payservice(uint64_t service_id, name contract_account,
                                     asset amount, std::string memo) {
     return push_action(
         contract_account, N(payservice),mvo()
         ("service_id", service_id)
         ("contract_account", contract_account)
         ("amount", amount)
         ("memo", memo)
         );
   }


   action_result starttimer(uint64_t service_id, name contract_account,
                                    name action_name, asset amount) {
     return push_action(
         N(oracle.bos), N(starttimer),mvo()
         ("service_id", service_id)
         ("contract_account", contract_account)
         ("action_name", action_name)
         ("amount", amount)
         );
   }

   //riskcontrol
   action_result deposit(uint64_t service_id,name from, name to, asset quantity,
                                 string memo, bool is_notify) {
     return push_action(
         N(oracle.bos), N(deposit),mvo()
         ("service_id", service_id)
         ("from", from)
         ("to", to)
         ("quantity", quantity)      
         ("memo", memo)
         ("is_notify", is_notify)
         );
   }

  action_result withdraw(uint64_t service_id,name from, name to, asset quantity,
                                 string memo) {
     return push_action(
         N(oracle.bos), N(withdraw),mvo()
         ("service_id", service_id)
         ("from", from)
         ("to", to)
         ("quantity", quantity)      
         ("memo", memo)
         );
   }
 
uint64_t reg_service(name account,time_point_sec update_start_time)
{
   return reg_service(0, account,update_start_time);
}

uint64_t reg_service(uint64_t service_id,name account,time_point_sec update_start_time)
{
//  name account = N(alice);
//  uint64_t service_id =0;
  uint8_t fee_type = 1;
  uint8_t data_type = 1;
  uint8_t status = 0;
  uint8_t injection_method = 0;
  uint64_t acceptance = 0;
  uint64_t duration = 30;
  uint64_t provider_limit = 3;
  uint64_t update_cycle = 300;
  uint64_t appeal_freeze_period = 0;
  uint64_t exceeded_risk_control_freeze_period = 0;
  uint64_t guarantee_id = 0;
  asset service_price = core_sym::from_string("1.0000");
  asset amount = core_sym::from_string("10.0000");
  asset risk_control_amount = core_sym::from_string("0.0000");
  asset pause_service_stake_amount = core_sym::from_string("0.0000");
  std::string data_format = "";
  std::string criteria = "";
  std::string declaration = "";
  bool freeze_flag = false;
  bool emergency_flag = false;
//   time_point_sec update_start_time = time_point_sec( control->head_block_time() );

  auto token = regservice(service_id, account, amount, service_price,
                          fee_type, data_format, data_type, criteria,
                          acceptance, declaration, injection_method, duration,
                          provider_limit, update_cycle, update_start_time);

  uint64_t create_time_sec =
      static_cast<uint64_t>(update_start_time.sec_since_epoch());

  uint64_t new_service_id = get_provider_service_id(
      account, create_time_sec);

      return new_service_id;
}

/// stake asset
 void stake_asset(uint64_t service_id, name account, asset amount)
  {
//  uint64_t service_id = new_service_id;
//   name account = N(alice);
//   asset amount = core_sym::from_string("1.0000");
  string memo = "";
//   push_action();
   push_permission_update_auth_action(account);
  auto token = stakeasset(service_id, account, amount,memo);
//   BOOST_TEST_REQUIRE( amount == get_data_provider(account)["total_stake_amount"].as<asset>() );
  }

/// add fee type
void add_fee_type(uint64_t service_id)
{
   // uint64_t service_id = new_service_id;
  std::vector<uint8_t> fee_types = {0,1};
  std::vector<asset> service_prices = {core_sym::from_string("1.0000"),core_sym::from_string("2.0000")};
  auto token = addfeetypes(service_id, fee_types, service_prices);

}

 // subscribe service
 void subscribe_service(uint64_t service_id,name account)
  {
     subscribe_service(service_id,account,N(dappuser.bos));
  }
 // subscribe service
 void subscribe_service(uint64_t service_id,name account,name contract_account)
  {
   //  service_id = new_service_id;
   //  name contract_account = N(dappuser.bos);
    name action_name = N(receivejson);
    std::string publickey = "";
   //  name account = N(bob);
    asset amount = core_sym::from_string("10.0000");
    std::string memo = "";
    auto subs = subscribe(service_id, contract_account, action_name, publickey,
                          account, amount, memo);

    }

  ///pay service
  void pay_service(uint64_t service_id, name contract_account, asset amount)
   {
//   uint64_t service_id = new_service_id;
//   name contract_account = N(dappuser.bos);
//   asset amount = core_sym::from_string("10.0000");
  std::string memo = "";
  push_permission_update_auth_action(contract_account);
  auto token = payservice(service_id, contract_account, 
                          amount, memo);
}

  /// push data
  void push_data(uint64_t service_id,name provider,uint64_t request_id)
   {
   // service_id = new_service_id;
   // name provider = N(alice);
   name contract_account = N(dappuser.bos);
   name action_name = N(receivejson);
   const string data_json = "test data json";
   // uint64_t request_id = 0;

   auto data = pushdata(service_id, provider, contract_account, action_name,
                         request_id, data_json);
   }

 /// multipush
 void multi_push(uint64_t service_id,name provider,bool is_request)
   {
   //   uint64_t service_id = new_service_id;
   //   name provider = N(alice);
     const string data_json = "multipush request test data json";
   //   bool is_request = true;

     auto token = multipush(service_id, provider, data_json, is_request);
   }

 /// request data
   void request_data(uint64_t service_id,name account)
   {
   //   service_id = new_service_id;
     name contract_account = N(dappuser.bos);
     name action_name = N(alice);
   //   name account = N(bob);
     std::string request_content = "request once";
     auto req = requestdata(service_id, contract_account, action_name, account,
                            request_content);
   }

 /// deposit
   void _deposit(uint64_t service_id,name to)
   {
   //   uint64_t service_id = new_service_id;
     name from = N(dappuser);
   //   name to = N(bob);
     asset quantity = core_sym::from_string("1.0000");
     std::string memo = "";
     bool is_notify = false;
     auto token = deposit(service_id, from, to, quantity, memo, is_notify);
   }

   /// withdraw
   void _withdraw(uint64_t service_id,name from)
   {
   //   uint64_t service_id = new_service_id;
   //   name from = N(bob);
     name to = N(dappuser);
     asset quantity = core_sym::from_string("0.1000");
     std::string memo = "";
     auto token = withdraw(service_id, from, to, quantity, memo);

     auto app_balance = get_riskcontrol_account(from, "4,TST");
     REQUIRE_MATCHING_OBJECT(app_balance, mvo()("balance", "0.9000 TST"));
   }

   abi_serializer abi_ser;
   abi_serializer token_abi_ser;
   abi_serializer dapp_abi_ser;
};

BOOST_AUTO_TEST_SUITE(bos_oracle_tests)

BOOST_FIXTURE_TEST_CASE( reg_test, bos_oracle_tester ) try {

 name account = N(alice);
 uint64_t service_id =0;
  uint8_t fee_type = 1;
  uint8_t data_type = 1;
  uint8_t status = 0;
  uint8_t injection_method = 0;
  uint64_t acceptance = 0;
  uint64_t duration = 1;
  uint64_t provider_limit = 3;
  uint64_t update_cycle = 1;
  uint64_t appeal_freeze_period = 0;
  uint64_t exceeded_risk_control_freeze_period = 0;
  uint64_t guarantee_id = 0;
  asset service_price = core_sym::from_string("1.0000");
  asset amount = core_sym::from_string("10.0000");
  asset risk_control_amount = core_sym::from_string("0.0000");
  asset pause_service_stake_amount = core_sym::from_string("0.0000");
  std::string data_format = "";
  std::string criteria = "";
  std::string declaration = "";
  bool freeze_flag = false;
  bool emergency_flag = false;
  time_point_sec update_start_time = time_point_sec( control->head_block_time() );

  auto token = regservice(service_id, account, amount, service_price,
                          fee_type, data_format, data_type, criteria,
                          acceptance, declaration, injection_method, duration,
                          provider_limit, update_cycle, update_start_time);
BOOST_TEST("" == "reg service after");
  uint64_t create_time_sec =
      static_cast<uint64_t>(update_start_time.sec_since_epoch());

  uint64_t new_service_id = get_provider_service_id(
      account, create_time_sec);

BOOST_TEST_REQUIRE( new_service_id == get_provider_service(account,create_time_sec)["service_id"].as<uint64_t>() );

  auto services = get_data_service(new_service_id);
  REQUIRE_MATCHING_OBJECT(services,mvo()
      ("service_id", service_id)
      ("fee_type", fee_type)
      ("data_type", data_type)
      ("status", status)
      ("injection_method", injection_method)
      ("acceptance", acceptance)
      ("duration", duration)
      ("provider_limit", provider_limit)
      ("update_cycle", update_cycle)
      ("appeal_freeze_period",appeal_freeze_period)
      ("exceeded_risk_control_freeze_period",exceeded_risk_control_freeze_period)
      ("guarantee_id", guarantee_id)
      ("service_price", service_price)
      ("amount", core_sym::from_string("0.0000"))
      ("risk_control_amount",  risk_control_amount)
      ("pause_service_stake_amount", pause_service_stake_amount)
      ("data_format", data_format)
      ("criteria", criteria)
      ("declaration", declaration)
      ( "freeze_flag", freeze_flag)
      ("emergency_flag",  emergency_flag)
      ("update_start_time",  update_start_time)
  );

BOOST_TEST("" == "reg service after");
//  BOOST_TEST_REQUIRE( amount == get_data_provider(account)["total_stake_amount"].as<asset>() );
      // BOOST_REQUIRE_EQUAL( success(), vote(N(producvoterc), vector<account_name>(producer_names.begin(), producer_names.begin()+26)) );
      // BOOST_REQUIRE( 0 < get_producer_info2(producer_names[11])["votepay_share"].as_double() );
    

  produce_blocks(1);
/// add fee type
{
   uint64_t service_id = new_service_id;
  std::vector<uint8_t> fee_types = {0,1};
  std::vector<asset> service_prices = {core_sym::from_string("1.0000"),core_sym::from_string("2.0000")};
  auto token = addfeetypes(service_id, fee_types, service_prices);

  int fee_type = 1;
  auto fee = get_data_service_fee(service_id,fee_types[fee_type]);
  REQUIRE_MATCHING_OBJECT(fee,mvo()
      ("service_id", service_id)
      ("fee_type", fee_types[fee_type])
      ("service_price", service_prices[fee_type])
  );

  produce_blocks(1);
}

/// stake asset
  {
 uint64_t service_id = new_service_id;
  name account = N(alice);
  asset amount = core_sym::from_string("1.0000");
  string memo = "";
//   push_action();
BOOST_TEST("" == "push_permission_update_auth_action before");
   push_permission_update_auth_action(account);
   BOOST_TEST("" == "push_permission_update_auth_action");
  auto token = stakeasset(service_id, account, amount,memo);
  BOOST_TEST_REQUIRE( amount == get_data_provider(account)["total_stake_amount"].as<asset>() );
  }

  // subscribe service
  {
    service_id = new_service_id;
    name contract_account = N(dappuser.bos);
    name action_name = N(receivejson);
    std::string publickey = "";
    name account = N(bob);
    asset amount = core_sym::from_string("0.0000");
    std::string memo = "";
    auto subs = subscribe(service_id, contract_account, action_name, publickey,
                          account, amount, memo);

    auto consumer = get_data_consumer(account);
    auto time = consumer["create_time"];
    //  BOOST_TEST("" == "1221ss");
    BOOST_REQUIRE(0 == consumer["status"].as<uint8_t>());
    //  BOOST_TEST("" == "11ss");
    auto subscription =
        get_data_service_subscription(service_id, contract_account);
     BOOST_TEST("" == "ss");
    BOOST_TEST_REQUIRE(amount == subscription["payment"].as<asset>());
    BOOST_TEST_REQUIRE(action_name == subscription["action_name"].as<name>());
   BOOST_TEST_REQUIRE(account == subscription["account"].as<name>());
   }

   produce_blocks(1);
   ///pay service
   {
  uint64_t service_id = new_service_id;
  name contract_account = N(dappuser.bos);
  asset amount = core_sym::from_string("10.0000");
  std::string memo = "";
  push_permission_update_auth_action(contract_account);
  auto token = payservice(service_id, contract_account, 
                          amount, memo);
}


   /// push data
   {
   service_id = new_service_id;
   name provider = N(alice);
   name contract_account = N(dappuser.bos);
   name action_name = N(receivejson);
   const string data_json = "test data json";
   uint64_t request_id = 0;

   auto data = pushdata(service_id, provider, contract_account, action_name,
                         request_id, data_json);
   }

BOOST_TEST("" == "====pushdata");
  produce_blocks(1);
   /// multipush
   {
     uint64_t service_id = new_service_id;
     name provider = N(alice);
     const string data_json = "multipush test data json";
     bool is_request = false;

     auto token = multipush(service_id, provider, data_json, is_request);
     
   }

   produce_blocks(1);

BOOST_TEST("" == "====multipush false");
   /// request data
   {
     service_id = new_service_id;
     name contract_account = N(dappuser.bos);
     name action_name = N(alice);
     name account = N(bob);
     std::string request_content = "request once";
     auto req = requestdata(service_id, contract_account, action_name, account,
                            request_content);
   }
   produce_blocks(1);

BOOST_TEST("" == "====requestdata");
   /// multipush
   {
     uint64_t service_id = new_service_id;
     name provider = N(alice);
     const string data_json = "multipush request test data json";
     bool is_request = true;

     auto token = multipush(service_id, provider, data_json, is_request);
   }

   produce_blocks(1);
BOOST_TEST("" == "====multipush true");
   /// deposit
   {
     uint64_t service_id = new_service_id;
     name from = N(dappuser);
     name to = N(bob);
     asset quantity = core_sym::from_string("1.0000");
     std::string memo = "";
     bool is_notify = false;
     auto token = deposit(service_id, from, to, quantity, memo, is_notify);

      auto app_balance = get_riskcontrol_account(to, "4,TST");
   REQUIRE_MATCHING_OBJECT( app_balance, mvo()
      ("balance", "1.0000 TST")
   );


   }

   BOOST_TEST("" == "====deposit ");
   /// withdraw
   {
     uint64_t service_id = new_service_id;
     name from = N(bob);
     name to = N(dappuser);
     asset quantity = core_sym::from_string("0.1000");
     std::string memo = "";
     auto token = withdraw(service_id, from, to, quantity, memo);

     auto app_balance = get_riskcontrol_account(from, "4,TST");
     REQUIRE_MATCHING_OBJECT(app_balance, mvo()("balance", "0.9000 TST"));
   }

   BOOST_TEST("" == "====withdraw ");
   // produce_blocks(2*24*60*60);
   {
      name account = N(alice);
      name receive_account = N(alice);
      push_permission_update_auth_action(N(consumer.bos));
      auto token = claim(account, receive_account);

      BOOST_REQUIRE_EQUAL( core_sym::from_string("1003.8000"), get_balance( "alice" ) );
   }

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( unreg_pause_test, bos_oracle_tester ) try {

  name account = N(alice);
  time_point_sec update_start_time = time_point_sec(control->head_block_time());
  uint64_t service_id = reg_service(account, update_start_time);
  const uint8_t status_cancel = 1;
  const uint8_t status_pause = 2;
  auto token = unregservice(service_id, account, status_pause);
  BOOST_TEST_REQUIRE( status_pause == get_data_service_provision(service_id,account)["status"].as<uint8_t>() );

//   BOOST_TEST_REQUIRE( status_pause == get_svc_provision_cancel_apply(service_id)["status"].as<uint8_t>() );


} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( execaction_test, bos_oracle_tester ) try {

  name account = N(alice);
  time_point_sec update_start_time = time_point_sec(control->head_block_time());
  uint64_t service_id = reg_service(account, update_start_time);
  const uint8_t freeze_action_type = 3;
  const uint8_t emergency_action_type = 4;
  auto token = execaction(service_id,  freeze_action_type);
BOOST_TEST_REQUIRE( true == get_data_service(service_id)["freeze_flag"].as<bool>() );
  token = execaction(service_id,  emergency_action_type);
BOOST_TEST_REQUIRE( true == get_data_service(service_id)["emergency_flag"].as<bool>() );


} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( stakeasset_test, bos_oracle_tester ) try {

name account = N(alice);
time_point_sec update_start_time = time_point_sec( control->head_block_time() );
 uint64_t new_service_id= reg_service(account,update_start_time);

BOOST_TEST("" == "====reg test true");
  produce_blocks(1);
/// stake asset
  {
 uint64_t service_id = new_service_id;
  name account = N(alice);
  asset amount = core_sym::from_string("1.0000");
  string memo = "";
//   push_action();
BOOST_TEST("" == "push_permission_update_auth_action before");
   push_permission_update_auth_action(account);
   BOOST_TEST("" == "push_permission_update_auth_action");
  auto token = stakeasset(service_id, account, amount,memo);
  BOOST_TEST_REQUIRE( amount == get_data_provider(account)["total_stake_amount"].as<asset>() );
  }
 
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( pushdata_test, bos_oracle_tester ) try {

///reg service
name account = N(alice);
time_point_sec update_start_time = time_point_sec( control->head_block_time() );
 uint64_t service_id= reg_service(account,update_start_time);
add_fee_type(service_id);
stake_asset(service_id,N(alice),core_sym::from_string("10.0000"));
subscribe_service(service_id,N(bob));
pay_service(service_id, N(dappuser.bos),core_sym::from_string("10.0000"));

  /// push data
   {
   name provider = N(alice);
   name contract_account = N(dappuser.bos);
   name action_name = N(receivejson);
   const string data_json = "test data json";
   uint64_t request_id = 0;

   auto data = pushdata(service_id, provider, contract_account, action_name,
                         request_id, data_json);
   }

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( multipush_test, bos_oracle_tester ) try {

  name account = N(alice);
  time_point_sec update_start_time = time_point_sec(control->head_block_time());
  uint64_t service_id = reg_service(account, update_start_time);

  add_fee_type(service_id);
  stake_asset(service_id, N(alice), core_sym::from_string("10.0000"));
  subscribe_service(service_id, N(bob));
  pay_service(service_id, N(dappuser.bos), core_sym::from_string("10.0000"));

  /// multipush
  {
     name provider = N(alice);
    const string data_json = "multipush test data json";
    bool is_request = false;

    auto token = multipush(service_id, provider, data_json, is_request);
     
  
     is_request = true;

     token = multipush(service_id, provider, data_json, is_request);
   }

   

} FC_LOG_AND_RETHROW()



BOOST_FIXTURE_TEST_CASE( publishdata_test, bos_oracle_tester ) try {

  /// reg service
  name account = N(alice);
  time_point_sec update_start_time = time_point_sec(control->head_block_time());
  uint64_t service_id = reg_service(N(provider1111), update_start_time);
  reg_service(service_id, N(provider2222), update_start_time);
  reg_service(service_id, N(provider3333), update_start_time);
  reg_service(service_id, N(provider4444), update_start_time);
  reg_service(service_id, N(provider5555), update_start_time);

  add_fee_type(service_id);
  stake_asset(service_id, N(provider1111), core_sym::from_string("10.0000"));
  stake_asset(service_id, N(provider2222), core_sym::from_string("10.0000"));
  stake_asset(service_id, N(provider3333), core_sym::from_string("10.0000"));
  stake_asset(service_id, N(provider4444), core_sym::from_string("10.0000"));
  stake_asset(service_id, N(provider5555), core_sym::from_string("10.0000"));

  subscribe_service(service_id, N(consumer1111), N(conconsumer1));
  subscribe_service(service_id, N(consumer2222), N(conconsumer2));
  subscribe_service(service_id, N(consumer3333), N(conconsumer3));
  subscribe_service(service_id, N(consumer4444), N(conconsumer4));
  subscribe_service(service_id, N(consumer5555), N(conconsumer5));

  pay_service(service_id, N(conconsumer1), core_sym::from_string("10.0000"));
  pay_service(service_id, N(conconsumer2), core_sym::from_string("10.0000"));
  pay_service(service_id, N(conconsumer3), core_sym::from_string("10.0000"));
  pay_service(service_id, N(conconsumer4), core_sym::from_string("10.0000"));
  pay_service(service_id, N(conconsumer5), core_sym::from_string("10.0000"));

  /// publish data
   {
     auto services = get_data_service(service_id);

    uint64_t update_cycle = services["update_cycle"].as<uint64_t>();
     BOOST_REQUIRE(update_cycle > 0);
    uint64_t duration = services["duration"].as<uint64_t>();
    BOOST_REQUIRE(duration > 0);
    uint64_t update_number = update_start_time.sec_since_epoch()/update_cycle;
    const string data_json = "publish test data json";
    uint64_t request_id = 0;

    auto data = publishdata(service_id, N(provider1111), update_number,
                            request_id, data_json);
    data = publishdata(service_id, N(provider2222), update_number, request_id,
                       data_json);
    data = publishdata(service_id, N(provider3333), update_number, request_id,
                       data_json);
    data = publishdata(service_id, N(provider4444), update_number, request_id,
                       data_json);
    data = publishdata(service_id, N(provider5555), update_number, request_id,
                       data_json);

   auto oracledata = get_oracle_data(service_id,update_number);

    uint64_t update_number_from_api = oracledata["update_number"].as<uint64_t>();

      BOOST_REQUIRE(update_number_from_api > 0);

    fetchdata(service_id,update_number);

   }

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( multipublish_test, bos_oracle_tester ) try {

  name account = N(alice);
  time_point_sec update_start_time = time_point_sec(control->head_block_time());
  uint64_t service_id = reg_service(account, update_start_time);

  add_fee_type(service_id);
  stake_asset(service_id, N(alice), core_sym::from_string("10.0000"));
  subscribe_service(service_id, N(bob));
  pay_service(service_id, N(dappuser.bos), core_sym::from_string("10.0000"));

  /// multipublish
  {
     name provider = N(alice);
    const string data_json = "multipush test data json";
    bool is_request = false;

    auto token = multipublish(service_id, provider, data_json, is_request);
       
     is_request = true;

     token = multipublish(service_id, provider, data_json, is_request);
   }

   

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( addfeetype_test, bos_oracle_tester ) try {
name account = N(alice);
time_point_sec update_start_time = time_point_sec( control->head_block_time() );
 uint64_t new_service_id= reg_service(account,update_start_time);

/// add fee type
{
   uint64_t service_id = new_service_id;
  std::vector<uint8_t> fee_types = {0,1};
  std::vector<asset> service_prices = {core_sym::from_string("1.0000"),core_sym::from_string("2.0000")};
  auto token = addfeetypes(service_id, fee_types, service_prices);

  int fee_type = 1;
  auto fee = get_data_service_fee(service_id,fee_types[fee_type]);
  REQUIRE_MATCHING_OBJECT(fee,mvo()
      ("service_id", service_id)
      ("fee_type", fee_types[fee_type])
      ("service_price", service_prices[fee_type])
  );
}

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( subscribe_test, bos_oracle_tester ) try {

name account = N(alice);
time_point_sec update_start_time = time_point_sec( control->head_block_time() );
 uint64_t service_id= reg_service(account,update_start_time);
add_fee_type(service_id);
stake_asset(service_id,N(alice),core_sym::from_string("10.0000"));

   // subscribe service
  {
    name contract_account = N(dappuser.bos);
    name action_name = N(receivejson);
    std::string publickey = "";
    name account = N(bob);
    asset amount = core_sym::from_string("0.0000");
    std::string memo = "";
    auto subs = subscribe(service_id, contract_account, action_name, publickey,
                          account, amount, memo);

    auto consumer = get_data_consumer(account);
    auto time = consumer["create_time"];
    //  BOOST_TEST("" == "1221ss");
    BOOST_REQUIRE(0 == consumer["status"].as<uint8_t>());
    //  BOOST_TEST("" == "11ss");
    auto subscription =
        get_data_service_subscription(service_id, contract_account);
    BOOST_TEST("" == "ss");
    BOOST_TEST_REQUIRE(amount == subscription["payment"].as<asset>());
    BOOST_TEST_REQUIRE(action_name == subscription["action_name"].as<name>());
    BOOST_TEST_REQUIRE(account == subscription["account"].as<name>());
   }


} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( requestdata_test, bos_oracle_tester ) try {
   
     name account = N(alice);
  time_point_sec update_start_time = time_point_sec(control->head_block_time());
  uint64_t service_id = reg_service(account, update_start_time);

  add_fee_type(service_id);
  stake_asset(service_id, N(alice), core_sym::from_string("10.0000"));
  subscribe_service(service_id, N(bob));
  pay_service(service_id, N(dappuser.bos), core_sym::from_string("10.0000"));

     /// request data
   {
     name contract_account = N(dappuser.bos);
     name action_name = N(alice);
     name account = N(bob);
     std::string request_content = "request once";
     auto req = requestdata(service_id, contract_account, action_name, account,
                            request_content);
   }

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( payservice_test, bos_oracle_tester ) try {
     name account = N(alice);
  time_point_sec update_start_time = time_point_sec(control->head_block_time());
  uint64_t service_id = reg_service(account, update_start_time);

  add_fee_type(service_id);
  stake_asset(service_id, N(alice), core_sym::from_string("10.0000"));
  subscribe_service(service_id, N(bob));
 
 ///pay service
   {
  name contract_account = N(dappuser.bos);
  asset amount = core_sym::from_string("10.0000");
  std::string memo = "";
  push_permission_update_auth_action(contract_account);
  auto token = payservice(service_id, contract_account, 
                          amount, memo);
}

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( claim_test, bos_oracle_tester ) try {
     name account = N(alice);
  time_point_sec update_start_time = time_point_sec(control->head_block_time());
  uint64_t service_id = reg_service(account, update_start_time);

  add_fee_type(service_id);
  stake_asset(service_id, N(alice), core_sym::from_string("10.0000"));
  subscribe_service(service_id, N(bob));
  pay_service(service_id, N(dappuser.bos), core_sym::from_string("10.0000"));
  request_data(service_id,N(bob));
  push_data(service_id,N(alice),0);
  multi_push(service_id,N(alice),false);
  multi_push(service_id,N(alice),true);
 _deposit(service_id,N(bob));
 _withdraw(service_id,N(bob));

  {
      name account = N(alice);
      name receive_account = N(alice);
      push_permission_update_auth_action(N(consumer.bos));
      auto token = claim(account, receive_account);

      BOOST_REQUIRE_EQUAL( core_sym::from_string("994.8000"), get_balance( "alice" ) );
   }
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( deposit_test, bos_oracle_tester ) try {
  name account = N(alice);
  time_point_sec update_start_time = time_point_sec(control->head_block_time());
  uint64_t service_id = reg_service(account, update_start_time);

  add_fee_type(service_id);
  stake_asset(service_id, N(alice), core_sym::from_string("10.0000"));
  subscribe_service(service_id, N(bob));
  pay_service(service_id, N(dappuser.bos), core_sym::from_string("10.0000"));
 /// deposit
   {
     name from = N(dappuser);
     name to = N(bob);
     asset quantity = core_sym::from_string("1.0000");
     std::string memo = "";
     bool is_notify = false;
     auto token = deposit(service_id, from, to, quantity, memo, is_notify);

      auto app_balance = get_riskcontrol_account(to, "4,TST");
   REQUIRE_MATCHING_OBJECT( app_balance, mvo()
      ("balance", "1.0000 TST")
   );


   }



} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( withdraw_test, bos_oracle_tester ) try {
      name account = N(alice);
  time_point_sec update_start_time = time_point_sec(control->head_block_time());
  uint64_t service_id = reg_service(account, update_start_time);

  add_fee_type(service_id);
  stake_asset(service_id, N(alice), core_sym::from_string("10.0000"));
  subscribe_service(service_id, N(bob));
  pay_service(service_id, N(dappuser.bos), core_sym::from_string("10.0000"));
  request_data(service_id,N(bob));
  push_data(service_id,N(alice),0);
  multi_push(service_id,N(alice),false);
  multi_push(service_id,N(alice),true);
 _deposit(service_id,N(bob));

  /// withdraw
   {
     name from = N(bob);
     name to = N(dappuser);
     asset quantity = core_sym::from_string("0.1000");
     std::string memo = "";
     auto token = withdraw(service_id, from, to, quantity, memo);

     auto app_balance = get_riskcontrol_account(from, "4,TST");
     REQUIRE_MATCHING_OBJECT(app_balance, mvo()("balance", "0.9000 TST"));
   }

} FC_LOG_AND_RETHROW()



BOOST_AUTO_TEST_SUITE_END()
