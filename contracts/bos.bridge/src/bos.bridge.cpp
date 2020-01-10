#include <eosio/eosio.hpp>
#include "bos.bridge/bos.bridge.hpp"
#include <cmath>
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/transaction.hpp>
using namespace eosio;
using std::string;



// void bos_bridge::deposit(name from, name to, asset quantity) {
//    require_auth(from);
//    action(permission_level{_self, "active"_n}, "eosio.token"_n, "transfer"_n, std::make_tuple(from, _self, quantity, "")).send();

//    if(quantity.symbol != core_symbol())
//    {
//    action(permission_level{_self, "active"_n}, "eosio.token"_n, "transfer"_n, std::make_tuple(from, _self, quantity, "")).send();
//    action(permission_level{_self, "active"_n}, "eosio.token"_n, "retire"_n, std::make_tuple(quantity, "")).send();
//    }
  

//    action(permission_level{_self, "active"_n}, _self, "transfertofe"_n, std::make_tuple(to.to_string(),quantity.to_string())).send();
// }

// void bos_bridge::regvalidator(const std::vector<name>& validators) {
//    require_auth(_self);

//    auto validators_table = validators(get_self(), get_self().value);
//       auto itr = validators_table.find(account.value);
//       checkmsg = account.to_string() + " account already added";
//       check(itr == validators_table.end(), checkmsg.c_str());

//    std::string checkmsg = "";
//    for (auto& account : validators) {
//       checkmsg = account.to_string() + " account does not exist";
//       if(!is_account(account))
//       {
//          print("\n",checkmsg);
//          continue;
//       }

//       validators_table.emplace(get_self(), [&](auto& a) {
//          a.validators.push_back(account);
//       });
//    }

// }


// void bos_bridge::transfertof(name recipient, asset amount) {
//     require_auth(_self);1qazxsw2


//   if(amount.symbol != core_symbol())   {

//    //   asset maximum_supply=asset::from_string("1000000000.0000 ETHT");
//    //   action(permission_level{_self, "active"_n}, "eosio.token"_n, "create"_n, std::make_tuple(_self, maximum_supply)).send();

//       action(permission_level{_self, "active"_n}, "eosio.token"_n, "issue"_n, std::make_tuple(_self, recipient, amount, "")).send();
//    }
//    else   {
//       action(permission_level{_self, "active"_n}, "eosio.token"_n, "transfer"_n, std::make_tuple( _self,recipient, amount, "")).send();
//    }
   
// }

// void bos_bridge::transfertofe(string recipient, string amount) {
//    require_auth(_self);
// }

// void bos_bridge::on_transfer(name from, name to, asset quantity, string memo) {

//    //  check(get_first_receiver() == "eosio.token"_n, "should be eosio.token");
//    print_f("On notify : % % % %", from, to, quantity, memo);
//    if (from == _self || to != _self || quantity.symbol != core_symbol() || memo.empty()) {
//       // print("memo is empty on trasfer");
//       return;
//    }
//    check(quantity.is_valid(), "invalid quantity");
//    check(quantity.amount > 0, "must transfer positive quantity");
//    check_data(memo, "memo");

//    //  check(quantity.amount>100, "amount could not be less than 100");
//    //
//    // check(find(to) != end, "no account's business found ");

//    std::vector<std::string> parameters = bos_util::get_parameters(memo);
//    check(parameters.size() > 0, "parse memo failed ");
//    uint64_t transfer_category = bos_util::convert_to_int(parameters[index_category]);

//    auto check_parameters_size = [&](uint64_t category) -> bool {
//       std::vector<uint8_t> index_counts = {arbitrator_count, arbitrator_count, deposit_count, appeal_count, arbitrator_count, resp_case_count, risk_guarantee_case_count};
//       std::vector<std::string> help_strings = {"stake_category,index_id",
//                                                "pay_category,index_id",
//                                                "deposit_category,from,to,notify ",
//                                                "appeal_category,service_id ,evidence,info,reason,provider",
//                                                "arbitrator_category,type ",
//                                                "resp_case_category,arbitration_id,evidence",
//                                                "risk_guarantee_category,id,duration"};


//       if (category >= 0 && category < index_counts.size()) {
//          if (parameters.size() == index_counts[category]) {
//             return true;
//          }
//          std::string str = "the parameters'size does not match " + help_strings[category];
//          check(false, str.c_str());
//       }

//       return false;
//    };

//    if (!check_parameters_size(transfer_category)) {
//       print("unknown category");
//       return;
//    }

//    auto s2name = [&](uint64_t index) -> name {
//       if (index >= 0 && index < parameters.size()) {
//          return name(parameters[index]);
//       } else {
//          print("index invalid", index, parameters.size());
//       }

//       return name{};
//    };
//    auto s2int = [&](uint64_t index) -> uint64_t {
//       if (index >= 0 && index < parameters.size()) {
//          return bos_util::convert_to_int(parameters[index]);
//       } else {
//          print("index invalid", index, parameters.size());
//       }
//       return 0;
//    };

//    if (tc_deposit == transfer_category) {
//       call_deposit(s2name(static_cast<uint64_t>(index_from)), s2name(index_to), quantity, 0 != s2int(index_notify));
//       // transfer(_self, riskctrl_account, quantity, memo);
//    } else {

//    }
// }

// /**
//  * @brief
//  *
//  * @param from
//  * @param to
//  * @param quantity
//  * @param memo
//  */
// void bos_bridge::transfer(name to, asset quantity, string memo) { 






//    // bridge_transfer(from, to, quantity, memo, true); 

//    }

// void bos_bridge::bridge_transfer(name from, name to, asset quantity, string memo, bool is_deferred) {
//    check_data(memo, "memo");

//    check(from != to, "cannot transfer to self");
//    //  require_auth( from );
//    check(is_account(to), "to account does not exist");
//    //  auto sym = quantity.symbol.code();
//    //  stats statstable( _self, sym.raw() );
//    //  const auto& st = statstable.get( sym.raw() );

//    //  require_recipient( from );
//    //  require_recipient( to );

//    check(quantity.is_valid(), "invalid quantity");
//    check(quantity.amount > 0, "must transfer positive quantity");
//    // check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
//    check(memo.size() <= 256, "memo has more than 256 bytes");

//    //   token::transfer_action transfer_act{ token_account, { account,
//    //   active_permission } };
//    //          transfer_act.send( account, consumer_account, amount, memo );

//    //  auto payer = has_auth( to ) ? to : from;
//    // print("===quantity");
//    quantity.print();

//    if (!is_deferred) {
//       action(permission_level{from, "active"_n}, token_account, "transfer"_n, std::make_tuple(from, to, quantity, memo)).send();
//    } else {
//       transaction t;
//       t.actions.emplace_back(permission_level{from, active_permission}, token_account, "transfer"_n, std::make_tuple(from, to, quantity, memo));
//       t.delay_sec = 0;
//       uint128_t deferred_id = (uint128_t(to.value) << 64) | bos_bridge::current_time_point_sec().sec_since_epoch();
//       cancel_deferred(deferred_id);
//       t.send(deferred_id, _self, true);
//    }

//    // INLINE_ACTION_SENDER(eosio::token, transfer)(token_account, {{from,
//    // active_permission}, {to, active_permission}},{from, to, quantity, memo});
// }






// } // namespace bosbridge

/**
 * @brief  Sets config parameters
 *
 * @param version   parameters version  changed when paramters changes
 * @param parameters  config paramters such as core symbol,precision etc
 */
void bos_bridge::setparameter(ignore<uint8_t> version, ignore<bridge_parameters> parameters) {
   require_auth(_self);
   uint8_t _version;
   bridge_parameters _parameters;
   _ds >> _version >> _parameters;
   // //  print(_version, _parameters.core_symbol, _parameters.precision);
   // check(!_parameters.core_symbol.empty(), "core_symbol could not be empty");
   // check(_parameters.precision > 0, "precision must be greater than 0");
   // check(_parameters.min_service_stake_limit > 0, "min_service_stake_limit must be greater than 0");
   // check(_parameters.min_appeal_stake_limit > 0, "min_appeal_stake_limit must be greater than 0");
   // check(_parameters.min_reg_arbitrator_stake_limit > 0, "min_reg_arbitrator_stake_limit must be greater than 0");
   // check(_parameters.arbitration_correct_rate > 0, "arbitration_correct_rate must be greater than 0");
   // check(_parameters.round_limit > 0, "round_limit must be greater than 0");
   // check(_parameters.arbi_timeout_value > 0, "arbi_timeout_value must be greater than 0");
   // check(_parameters.arbi_freeze_stake_duration > 0, "arbi_freeze_stake_duration must be greater than 0");
   // check(_parameters.time_deadline > 0, "time_deadline must be greater than 0");
   // check(_parameters.clear_data_time_length > 0, "clear_data_time_length must be greater than 0");
   // check(_parameters.max_data_size > 0, "max_data_size must be greater than 0");
   // check(_parameters.min_provider_limit > 0, "min_provider_limit must be greater than 0");
   // check(_parameters.max_provider_limit > 0, "max_provider_limit must be greater than 0");
   // check(_parameters.min_update_cycle > 0, "min_update_cycle must be greater than 0");
   // check(_parameters.max_update_cycle > 0, "max_update_cycle must be greater than 0");
   // check(_parameters.min_duration > 0, "min_duration must be greater than 0");
   // check(_parameters.max_duration > 0, "max_duration must be greater than 0");
   // check(_parameters.min_acceptance > 0, "min_acceptance must be greater than 0");
   // check(_parameters.max_acceptance > 0, "max_acceptance must be greater than 0");

   // std::string checkmsg = "unsupported version for setparameter action,current_bridge_version=" + std::to_string(current_bridge_version);
   // check(_version == current_bridge_version, checkmsg.c_str());
   // _bridge_meta_parameters.version = _version;
   // _bridge_meta_parameters.parameters_data = pack(_parameters);
}



time_point_sec bos_oracle::current_time_point_sec() {
  const static time_point_sec cts{current_time_point()};
  return cts;
}


// EOSIO_DISPATCH(bos_bridge,
//  (regservice)(unregservice)(execaction)(unstakeasset)(pushdata)(bridgepush)(
//               addfeetypes)(claim)(subscribe)(requestdata)(starttimer)(cleardata)
//               (uploadeviden)(uploadresult)(acceptarbi)(unstakearbi)(claimarbi)(timertimeout)
//               (deposit)(withdraw)
//               (importwps)(setstatus)
//               (setparameter)
// )




struct transferx {
  name from;
  name to;
  asset quantity;
  string memo;

  EOSLIB_SERIALIZE(transferx, (from)(to)(quantity)(memo))
};

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action) {
  if (action == "onerror"_n.value) {
    /* onerror is only valid if it is for the "eosio" code account and
     * authorized by "eosio"'s "active permission */
    check(code == "eosio"_n.value,
          "onerror action's are only valid from the \"eosio\" system account");
  }

  if (code == receiver || action == "onerror"_n.value) {
    switch (action) {
      // NB: Add custom method in bracets after () to use them as
      // endpoints
      EOSIO_DISPATCH_HELPER(
          bos_bridge,(setparameter)
                       )
    }
  }
  if (code == "eosio.token"_n.value && action == "transfer"_n.value) {

    datastream<const char *> ds = datastream<const char *>(nullptr, 0);

    bos_bridge thiscontract(name(receiver), name(code), ds);

    const transferx &t = unpack_action_data<transferx>();
    thiscontract.on_transfer(t.from, t.to, t.quantity, t.memo);
  }
}
