#include <boost/test/unit_test.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/testing/tester.hpp>
#include "eosio.system_tester.hpp"
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

class bos_burn_tester : public  tester  {
 public:
   bos_burn_tester() {
      produce_blocks(2);
      create_accounts({N(eosio.token), N(eosio.ram), N(eosio.ramfee), N(eosio.stake), N(eosio.bpay), N(eosio.vpay), N(eosio.saving), N(eosio.names), N(eosio.rex)});
      create_accounts({ N(alice),N(bob), N(carol), N(dapp), N(dappuser), N(burnbos4unac),N(burn.bos), N(hole.bos), N(dappuser.bos), N(dapp.bos), N(provider.bos), N(consumer.bos), N(arbitrat.bos), N(riskctrl.bos)});
      produce_blocks(2);

      set_code(N(burn.bos), contracts::burn_wasm());
      set_abi(N(burn.bos), contracts::burn_abi().data());

      produce_blocks();

      const auto& accnt = control->db().get<account_object, by_name>(N(burn.bos));
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

      // set_code(config::system_account_name, contracts::system_wasm());
      // set_abi(config::system_account_name, contracts::system_abi().data());
      // base_tester::push_action(config::system_account_name, N(init), config::system_account_name, mutable_variant_object()("version", 0)("core", CORE_SYM_STR));
      deploy_contract();
      produce_blocks();

      set_code(N(dapp.bos), contracts::token_wasm());
      set_abi(N(dapp.bos), contracts::token_abi().data());

      create_currency(N(dapp.bos), N(dappuser.bos), core_sym::from_string("10000000000.0000"));
      issuex(N(dapp.bos), N(dappuser.bos), core_sym::from_string("1000000000.0000"), N(dappuser.bos));
      produce_blocks();

      create_account_with_resources(N(alice1111111), N(eosio), core_sym::from_string("4000.0000"), false, core_sym::from_string("0.1000"), core_sym::from_string("0.1000"));
      create_account_with_resources(N(bob111111111), N(eosio), core_sym::from_string("4000.4500"), false, core_sym::from_string("0.2000"), core_sym::from_string("0.2000"));
      create_account_with_resources(N(carol1111111), N(eosio), core_sym::from_string("4000.0000"), false);
      create_account_with_resources(N(burnbos2msig), N(eosio), core_sym::from_string("1000.4500"), false, core_sym::from_string("10000.0000"), core_sym::from_string("10000.0000"));
      create_account_with_resources(N(burnbos4msig), N(eosio), core_sym::from_string("1000.4500"), false, core_sym::from_string("10000.0000"), core_sym::from_string("10000.0000"));

      transfer("eosio", "hole.bos", ("100.0001"), "eosio");
      transfer("eosio", "alice1111111", ("0.5000"), "eosio");
      transfer("eosio", "bob111111111", ("0.4000"), "eosio");
      transfer("eosio", "carol1111111", ("3000.0000"), "eosio");
      transfer("eosio", "alice", ("3000.0000"), "eosio");
      transfer("eosio", "bob", ("3000.0000"), "eosio");
      transfer("eosio", "carol", ("3000.0000"), "eosio");
      transfer("eosio", "dappuser.bos", ("30000.0000"), "eosio");
      transfer("eosio", "dappuser", ("3000.0000"), "eosio");
      transfer("eosio", "dapp", ("3000.0000"), "eosio");
      transfer("eosio", "burnbos4msig", ("30000.0000"), "eosio");
      transfer("eosio", "burnbos2msig", ("30000.0000"), "eosio");
      transfer("eosio", "burn.bos", ("30000.0000"), "eosio");
      // stake(N(alice1111111), core_sym::from_string("0.1000"),core_sym::from_string("0.1000"));
      // unstake("eosio", N(alice1111111), core_sym::from_string("10.0000"),core_sym::from_string("10.0000"));
      // stake(N(bob111111111), core_sym::from_string("0.2000"),core_sym::from_string("0.2000"));
      // unstake("eosio", N(bob111111111), core_sym::from_string("10.0000"),core_sym::from_string("10.0000"));
   //       BOOST_REQUIRE_EQUAL( success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("10000000.0000") ) );
   // BOOST_REQUIRE_EQUAL( core_sym::from_string("90000998.0049"), get_balance( "alice1111111" ) );

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
   transaction_trace_ptr create_account_with_resources( account_name a, account_name creator, uint32_t ram_bytes = 8000 ) {
      signed_transaction trx;
      set_transaction_headers(trx);

      authority owner_auth;
      owner_auth =  authority( get_public_key( a, "owner" ) );

      trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                newaccount{
                                   .creator  = creator,
                                   .name     = a,
                                   .owner    = owner_auth,
                                   .active   = authority( get_public_key( a, "active" ) )
                                });

      trx.actions.emplace_back( get_action( config::system_account_name, N(buyrambytes), vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("payer", creator)
                                            ("receiver", a)
                                            ("bytes", ram_bytes) )
                              );
      trx.actions.emplace_back( get_action( config::system_account_name, N(delegatebw), vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("from", creator)
                                            ("receiver", a)
                                            ("stake_net_quantity", core_sym::from_string("10.0000") )
                                            ("stake_cpu_quantity", core_sym::from_string("10.0000") )
                                            ("transfer", 0 )
                                          )
                                );

      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );
      return push_transaction( trx );
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
                                          mvo()("from", creator)("receiver", a)("stake_net_quantity", net)("stake_cpu_quantity", cpu)("transfer", 1)));

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
   void transfer( name from, name to, const asset& amount, name manager = config::system_account_name) {
      base_tester::push_action( N(eosio.token), N(transfer), manager, mutable_variant_object()
                                ("from",    from)
                                ("to",      to )
                                ("quantity", amount)
                                ("memo", "")
                                );
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

   void deploy_contract(bool call_init = true) {
      set_code(config::system_account_name, contracts::system_wasm());
      set_abi(config::system_account_name, contracts::system_abi().data());
      if (call_init) {
         base_tester::push_action(config::system_account_name, N(init), config::system_account_name, mutable_variant_object()("version", 0)("core", CORE_SYM_STR));
      }

      {
         const auto& accnt = control->db().get<account_object, by_name>(config::system_account_name);
         abi_def abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
         system_abi_ser.set_abi(abi, abi_serializer_max_time);
      }
   }

   fc::variant get_total_stake(const account_name& act) {
      vector<char> data = get_row_by_account(config::system_account_name, act, N(userres), act);
      return data.empty() ? fc::variant() : system_abi_ser.binary_to_variant("user_resources", data, abi_serializer_max_time);
   }
 
   abi_serializer initialize_multisig() {
      abi_serializer msig_abi_ser;
      {
         create_account_with_resources( N(eosio.msig), config::system_account_name );
         BOOST_REQUIRE_EQUAL( success(), buyram( "eosio", "eosio.msig", core_sym::from_string("5000.0000") ) );
         produce_block();

         auto trace = base_tester::push_action(config::system_account_name, N(setpriv),
                                               config::system_account_name,  mutable_variant_object()
                                               ("account", "eosio.msig")
                                               ("is_priv", 1)
         );

         set_code( N(eosio.msig), contracts::msig_wasm() );
         set_abi( N(eosio.msig), contracts::msig_abi().data() );

         produce_blocks();
         const auto& accnt = control->db().get<account_object,by_name>( N(eosio.msig) );
         abi_def msig_abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, msig_abi), true);
         msig_abi_ser.set_abi(msig_abi, abi_serializer_max_time);
      }
      return msig_abi_ser;
   }


   transaction_trace_ptr setup_producer_accounts( const std::vector<account_name>& accounts,
                                                  asset ram = core_sym::from_string("1.0000"),
                                                  asset cpu = core_sym::from_string("80.0000"),
                                                  asset net = core_sym::from_string("80.0000")
                                                )
   {
      account_name creator(config::system_account_name);
      signed_transaction trx;
      set_transaction_headers(trx);

      for (const auto& a: accounts) {
         authority owner_auth( get_public_key( a, "owner" ) );
         trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                   newaccount{
                                         .creator  = creator,
                                         .name     = a,
                                         .owner    = owner_auth,
                                         .active   = authority( get_public_key( a, "active" ) )
                                         });

         trx.actions.emplace_back( get_action( config::system_account_name, N(buyram), vector<permission_level>{ {creator, config::active_name} },
                                               mvo()
                                               ("payer", creator)
                                               ("receiver", a)
                                               ("quant", ram) )
                                   );

         trx.actions.emplace_back( get_action( config::system_account_name, N(delegatebw), vector<permission_level>{ {creator, config::active_name} },
                                               mvo()
                                               ("from", creator)
                                               ("receiver", a)
                                               ("stake_net_quantity", net)
                                               ("stake_cpu_quantity", cpu )
                                               ("transfer", 0 )
                                               )
                                   );
      }

      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );
      return push_transaction( trx );
   }
   action_result regproducer( const account_name& acnt, int params_fixture = 1 ) {
      action_result r = push_actions( acnt, N(regproducer), mvo()
                          ("producer",  acnt )
                          ("producer_key", get_public_key( acnt, "active" ) )
                          ("url", "" )
                          ("location", 0 )
      );
      BOOST_REQUIRE_EQUAL( success(), r);
      return r;
   }

vector<name> active_and_vote_producers() {
      //stake more than 15% of total EOS supply to activate chain
      transfer( "eosio", "alice1111111", core_sym::from_string("650000000.0000"), "eosio" );
      BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "alice1111111", core_sym::from_string("300000000.0000"), core_sym::from_string("300000000.0000") ) );

      // create accounts {defproducera, defproducerb, ..., defproducerz} and register as producers
      std::vector<account_name> producer_names;
      {
         producer_names.reserve('z' - 'a' + 1);
         const std::string root("defproducer");
         for ( char c = 'a'; c < 'a'+21; ++c ) {
            producer_names.emplace_back(root + std::string(1, c));
         }
         setup_producer_accounts(producer_names);
         for (const auto& p: producer_names) {

            BOOST_REQUIRE_EQUAL( success(), regproducer(p) );
         }
      }
      produce_blocks( 250);

      auto trace_auth = tester::push_action(config::system_account_name, updateauth::get_name(), config::system_account_name, mvo()
                                            ("account", name(config::system_account_name).to_string())
                                            ("permission", name(config::active_name).to_string())
                                            ("parent", name(config::owner_name).to_string())
                                            ("auth",  authority(1, {key_weight{get_public_key( config::system_account_name, "active" ), 1}}, {
                                                  permission_level_weight{{config::system_account_name, config::eosio_code_name}, 1},
                                                     permission_level_weight{{config::producers_account_name,  config::active_name}, 1}
                                               }
                                            ))
      );
      BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace_auth->receipt->status);

      //vote for producers
      {
         transfer( config::system_account_name, "alice1111111", core_sym::from_string("100000000.0000"), config::system_account_name );
         BOOST_REQUIRE_EQUAL(success(), stake( "alice1111111", core_sym::from_string("30000000.0000"), core_sym::from_string("30000000.0000") ) );
         BOOST_REQUIRE_EQUAL(success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("30000000.0000") ) );
         BOOST_REQUIRE_EQUAL(success(), push_actions(N(alice1111111), N(voteproducer), mvo()
                                                    ("voter",  "alice1111111")
                                                    ("proxy", name(0).to_string())
                                                    ("producers", vector<account_name>(producer_names.begin(), producer_names.begin()+21))
                             )
         );
      }
      produce_blocks( 250 );

      auto producer_keys = control->head_block_state()->active_schedule.producers;
      BOOST_REQUIRE_EQUAL( 21, producer_keys.size() );
      BOOST_REQUIRE_EQUAL( name("defproducera"), producer_keys[0].producer_name );

      return producer_names;
   }

   action_result buyram( const account_name& payer, account_name receiver, const asset& eosin ) {
      return push_actions( payer, N(buyram), mvo()( "payer",payer)("receiver",receiver)("quant",eosin) );
   }
   action_result buyrambytes( const account_name& payer, account_name receiver, uint32_t numbytes ) {
      return push_actions( payer, N(buyrambytes), mvo()( "payer",payer)("receiver",receiver)("bytes",numbytes) );
   }

   action_result sellram( const account_name& account, uint64_t numbytes ) {
      return push_actions( account, N(sellram), mvo()( "account", account)("bytes",numbytes) );
   }

   action_result push_actions(const account_name& signer, const action_name& name, const variant_object& data, bool auth = true) {
      string action_type_name = system_abi_ser.get_action_type(name);

      action act;
      act.account = config::system_account_name;
      act.name = name;
      act.data = system_abi_ser.variant_to_binary(action_type_name, data, abi_serializer_max_time);

      return base_tester::push_action(std::move(act), auth ? uint64_t(signer) : signer == N(bob111111111) ? N(alice1111111) : N(bob111111111));
   }
   action_result stake(const account_name& from, const account_name& to, const asset& net, const asset& cpu) {
      return push_actions(name(from), N(delegatebw), mvo()("from", from)("receiver", to)("stake_net_quantity", net)("stake_cpu_quantity", cpu)("transfer", 0));
   }

   action_result stake(const account_name& acnt, const asset& net, const asset& cpu) { return stake(acnt, acnt, net, cpu); }

   action_result stake_with_transfer(const account_name& from, const account_name& to, const asset& net, const asset& cpu) {
      return push_actions(name(from), N(delegatebw), mvo()("from", from)("receiver", to)("stake_net_quantity", net)("stake_cpu_quantity", cpu)("transfer", true));
   }

   action_result stake_with_transfer(const account_name& acnt, const asset& net, const asset& cpu) { return stake_with_transfer(acnt, acnt, net, cpu); }

   action_result unstake(const account_name& from, const account_name& to, const asset& net, const asset& cpu) {
      return push_actions(name(from), N(undelegatebw), mvo()("from", from)("receiver", to)("unstake_net_quantity", net)("unstake_cpu_quantity", cpu));
   }

   action_result push_action(const account_name& signer, const action_name& name, const variant_object& data) {

      string action_type_name = abi_ser.get_action_type(name);

      action act;
      act.account = N(burn.bos);
      act.name = name;
      act.data = abi_ser.variant_to_binary(action_type_name, data, abi_serializer_max_time);

      return base_tester::push_action(std::move(act), uint64_t(signer));
   }

   auto push_permission_update_auth_action(const account_name& signer) {
      auto auth = authority(eosio::testing::base_tester::get_public_key(signer, "active"));
      auth.accounts.push_back(permission_level_weight{{N(burn.bos), config::eosio_code_name}, 1});

      return base_tester::push_action(N(eosio), N(updateauth), signer, mvo()("account", signer)("permission", "active")("parent", "owner")("auth", auth));
   }

   fc::variant get_account(const name& account) {
      vector<char> data = get_row_by_account(N(burn.bos), account, N(accounts), account);
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("unactivated_airdrop_account", data, abi_serializer_max_time);
   }

   action_result importacnts(std::vector<std::pair<name, asset>> unactivated_airdrop_accounts) {
      // return push_action(N(burn.bos), N(importacnts), mvo()("unactivated_airdrop_accounts", unactivated_airdrop_accounts));
          auto trace = base_tester::push_action(N(burn.bos), N(importacnts),
                                               {N(burn.bos),N(burnbos4unac)},  mutable_variant_object()
                                              ("unactivated_airdrop_accounts", unactivated_airdrop_accounts));

                                              return "";
   }

   action_result clear(std::vector<name> clear_accounts) { return push_action(N(burnbos4unac), N(clear), mvo()("clear_accounts", clear_accounts)); }

   action_result burn(const asset& quantity) { 
      return push_action(N(burn.bos), N(burn), mvo()("quantity", quantity)); 
   //   auto trace = base_tester::push_action(N(burn.bos), N(burn),
   //                                             {N(burn.bos),N(burnbos4unac)},  mutable_variant_object()
   //                                            ("quantity", quantity));

   //                                            return "";
   }

   action_result transferair(const name& account) { return push_action(N(burnbos4unac), N(transferair), mvo()("account", account)); }

   abi_serializer abi_ser;
   abi_serializer system_abi_ser;
};

BOOST_AUTO_TEST_SUITE(bos_burn_tests)

BOOST_FIXTURE_TEST_CASE(burn_test, bos_burn_tester)
try {

   push_permission_update_auth_action(N(burnbos4unac));

   produce_blocks(1);
   /// imports
   {
      name account = N(alice1111111);

      std::vector<std::pair<name, asset>> account_quantity = {std::make_pair(account, core_sym::from_string("0.5000")), std::make_pair(N(bob111111111), core_sym::from_string("0.8000")),
                                                              std::make_pair(N(carol1111111), core_sym::from_string("1.8000"))};
      auto result = importacnts(account_quantity);

      produce_blocks(1);

      auto acc = get_account(account);
      REQUIRE_MATCHING_OBJECT(acc, mvo()("account", account)("quantity", "0.5000 BOS")("is_burned", 0));

      produce_blocks(1);
   }

   produce_blocks(1);

   /// burns
   {
      name account = N(alice1111111);
      BOOST_TEST(core_sym::from_string("0.5000") == get_balance(account));
      auto total = get_total_stake(account);
      BOOST_TEST(core_sym::from_string("0.1000") == total["net_weight"].as<asset>());
      BOOST_TEST(core_sym::from_string("0.1000") == total["cpu_weight"].as<asset>());
      auto result = transferair(account);
      BOOST_TEST(core_sym::from_string("0.0000") == get_balance(account));
      total = get_total_stake(account);
      BOOST_TEST(core_sym::from_string("0.0000") == total["net_weight"].as<asset>());
      BOOST_TEST(core_sym::from_string("0.0000") == total["cpu_weight"].as<asset>());
      produce_blocks(1);
   }

   /// burn
   {
      name account = N(bob111111111);
      BOOST_TEST(core_sym::from_string("0.4000") == get_balance(account));
      auto total = get_total_stake(account);
      BOOST_TEST(core_sym::from_string("0.2000") == total["net_weight"].as<asset>());
      BOOST_TEST(core_sym::from_string("0.2000") == total["cpu_weight"].as<asset>());
      auto result = transferair(account);
      BOOST_TEST(core_sym::from_string("0.0000") == get_balance(account));
      total = get_total_stake(account);
      BOOST_TEST(core_sym::from_string("0.0000") == total["net_weight"].as<asset>());
      BOOST_TEST(core_sym::from_string("0.0000") == total["cpu_weight"].as<asset>());
      produce_blocks(1);
   }

   /// burn
   {
      name account = N(carol1111111);
      BOOST_TEST(core_sym::from_string("3000.0000") == get_balance(account));
      auto total = get_total_stake(account);
      BOOST_TEST(core_sym::from_string("10.0000") == total["net_weight"].as<asset>());
      BOOST_TEST(core_sym::from_string("10.0000") == total["cpu_weight"].as<asset>());
      auto result = transferair(account);
      BOOST_TEST(core_sym::from_string("2998.0000") == get_balance(account));
      total = get_total_stake(account);
      BOOST_TEST(core_sym::from_string("10.0000") == total["net_weight"].as<asset>());
      BOOST_TEST(core_sym::from_string("10.0000") == total["cpu_weight"].as<asset>());
      produce_blocks(1);
   }

   /// burnhole
   {
      name account = N(hole.bos);
      BOOST_TEST(core_sym::from_string("103.5001") == get_balance(account));
      auto result = burn(core_sym::from_string("1.0000"));
      BOOST_TEST(core_sym::from_string("102.5001") == get_balance(account));
      produce_blocks(1);
   }

   /// clear
   {

      name account = N(alice1111111);
      std::vector<name> accounts = {account, N(bob111111111)};
      auto result = clear(accounts);

      produce_blocks(1);
   }
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(transferair_test, bos_burn_tester)
try {

   push_permission_update_auth_action(N(burnbos4unac));

   produce_blocks(1);
   /// imports
   {
      name account = N(alice1111111);

      std::vector<std::pair<name, asset>> account_quantity = {std::make_pair(account, core_sym::from_string("0.5000")), std::make_pair(N(bob111111111), core_sym::from_string("0.8000")),
                                                              std::make_pair(N(carol1111111), core_sym::from_string("1.8000"))};
      auto result = importacnts(account_quantity);

      produce_blocks(1);

      auto acc = get_account(account);
      REQUIRE_MATCHING_OBJECT(acc, mvo()("account", account)("quantity", "0.5000 BOS")("is_burned", 0));

      produce_blocks(1);
   }

   produce_blocks(1);

   auto trace_auth = tester::push_action(
       config::system_account_name, updateauth::get_name(), N(burn.bos),
       mvo()("account", name(N(burn.bos)).to_string())("permission", name(config::active_name).to_string())("parent", name(config::owner_name).to_string())(
           "auth", authority(1, {key_weight{get_public_key(config::system_account_name, "active"), 1}}, { permission_level_weight{{N(burn.bos), config::eosio_code_name}, 1},permission_level_weight{{config::system_account_name, config::active_name}, 1}})));
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace_auth->receipt->status);

   /// transferair
   {
      name account = N(carol1111111);
      BOOST_TEST(core_sym::from_string("3000.0000") == get_balance(account));
      auto total = get_total_stake(account);
      BOOST_TEST(core_sym::from_string("10.0000") == total["net_weight"].as<asset>());
      BOOST_TEST(core_sym::from_string("10.0000") == total["cpu_weight"].as<asset>());
      auto result = transferair(account);
      BOOST_TEST(core_sym::from_string("2998.0000") == get_balance(account));
      total = get_total_stake(account);
      BOOST_TEST(core_sym::from_string("10.0000") == total["net_weight"].as<asset>());
      BOOST_TEST(core_sym::from_string("10.0000") == total["cpu_weight"].as<asset>());
      produce_blocks(1);
   }

}
FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE(msig_burn_test, bos_burn_tester)
try
{

   push_permission_update_auth_action(N(burnbos4unac));

   produce_blocks(1);

   auto trace_auth = tester::push_action(
       config::system_account_name, updateauth::get_name(), N(burn.bos),
       mvo()("account", name(N(burn.bos)).to_string())("permission", name(config::active_name).to_string())("parent", name(config::owner_name).to_string())(
           "auth", authority(1, {key_weight{get_public_key(config::system_account_name, "active"), 1}}, {permission_level_weight{{N(burn.bos), config::eosio_code_name}, 1},permission_level_weight{{config::system_account_name, config::active_name}, 1}})));
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace_auth->receipt->status);

   produce_blocks(250);

   //install multisig contract
   abi_serializer msig_abi_ser = initialize_multisig();
   auto producer_names = active_and_vote_producers();

   //helper function
   auto push_action_msig = [&](const account_name &signer, const action_name &name, const variant_object &data, bool auth = true) -> action_result {
      string action_type_name = msig_abi_ser.get_action_type(name);

      action act;
      act.account = N(eosio.msig);
      act.name = name;
      act.data = msig_abi_ser.variant_to_binary(action_type_name, data, abi_serializer_max_time);

      return base_tester::push_action(std::move(act), auth ? uint64_t(signer) : signer == N(burnbos2msig) ? N(burnbos4msig) : N(burnbos2msig));
   };



   // test begins
   vector<permission_level> prod_perms;
   for (auto &x : producer_names)
   {
      prod_perms.push_back({name(x), config::active_name});
   }

   name contract_account = N(burn.bos);

   transaction trx;
   {
         variant pretty_trx = fc::mutable_variant_object()
         ("expiration", "2020-01-01T00:30")
         ("ref_block_num", 2)
         ("ref_block_prefix", 3)
         ("net_usage_words", 0)
         ("max_cpu_usage_ms", 0)
         ("delay_sec", 0)
         ("actions", fc::variants({
               fc::mutable_variant_object()
                  ("account", name(contract_account))
                  ("name", "burn")
                  ("authorization", vector<permission_level>{ { config::system_account_name, config::active_name },{ N(burn.bos), config::active_name } })
                  ("data", fc::mutable_variant_object()
                     ("quantity", core_sym::from_string("0.0001"))
                  )
                  })
         );
      abi_serializer::from_variant(pretty_trx, trx, get_resolver(), abi_serializer_max_time);
   }

     BOOST_REQUIRE_EQUAL(success(), push_action_msig( N(burnbos4msig), N(propose), mvo()
                                                    ("proposer",      "burnbos4msig")
                                                    ("proposal_name", "setparameter")
                                                    ("trx",           trx)
                                                    ("requested", prod_perms)
                       )
   );

   // get 15 approvals
   for (size_t i = 0; i < 15; ++i){
      BOOST_REQUIRE_EQUAL(success(), push_action_msig( name(producer_names[i]), N(approve), mvo()
                                                       ("proposer",      "burnbos4msig")
                                                       ("proposal_name", "setparameter")
                                                       ("level",         permission_level{ name(producer_names[i]), config::active_name })
                          )
      );
   }

   transaction_trace_ptr trace;
   control->applied_transaction.connect([&](const transaction_trace_ptr& t) {
      if (t->scheduled) {
         trace = t;
      }
   });

   BOOST_REQUIRE_EQUAL(success(), push_action_msig( N(burnbos4msig), N(exec), mvo()
                                                    ("proposer",      "burnbos4msig")
                                                    ("proposal_name", "setparameter")
                                                    ("executer",      "burnbos4msig")
                       )
   );

   BOOST_REQUIRE(bool(trace));
   BOOST_REQUIRE_EQUAL(1, trace->action_traces.size());
   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);


}
FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
