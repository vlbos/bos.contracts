#include "bos.bridge/bos.bridge.hpp"
#include <cmath>
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/transaction.hpp>
using namespace eosio;
using std::string;
#include "bos.bridge/BridgeValidators.hpp"
#include "bos.bridge/ForeignBridge.hpp"
#include "bos.bridge/HomeBridge.hpp"
#include "bridge.token.cpp"

// BasicBridge  begin
void bos_bridge::impvalidator(uint64_t requiredSignatures,
                              std::vector<std::pair<name,public_key>> &initialValidators,
                              name owner) {
  BridgeValidators _BridgeValidators(_self, _bridge_meta_parameters);
  _BridgeValidators.initialize(requiredSignatures, initialValidators, owner);
}
// BasicBridge end

// ForeignBridge  begin
void bos_bridge::transfern2h(name sender, name recipient, uint64_t value) {
  ForeignBridge _ForeignBridge(_self, _bridge_meta_parameters);
  _ForeignBridge.transferNativeToHome(sender, recipient, value);
}

void bos_bridge::transfert2h(name sender, std::string token, name recipient,
                             uint64_t value) {
  ForeignBridge _ForeignBridge(_self, _bridge_meta_parameters);
  _ForeignBridge.transferTokenToHome(sender, token, recipient, value);
}

void bos_bridge::transfer2he(const std::string& token, name recipient, uint64_t value) {
  require_auth(_self);
     
}

void bos_bridge::transferfrom(name sender, std::vector<signature> sig, bytes message) {
  ForeignBridge _ForeignBridge(_self, _bridge_meta_parameters);
  _ForeignBridge.transferFromHome(sender, sig, message);
}

/// ForeignBridge  end

/// HomeBridge begin
void bos_bridge::regtoken(name sender, std::string foreignAddress, std::string homeAddress) {
  HomeBridge _HomeBridge(_self, _bridge_meta_parameters);
  _HomeBridge.registerToken(sender, foreignAddress, homeAddress);
}

void bos_bridge::transfern2f(name sender, name recipient, uint64_t value) {
  HomeBridge _HomeBridge(_self, _bridge_meta_parameters);
  _HomeBridge.transferNativeToForeign(sender, recipient, value);
}

void bos_bridge::transfert2f(name sender, std::string token, name recipient,
                             uint64_t value) {
  HomeBridge _HomeBridge(_self, _bridge_meta_parameters);
  _HomeBridge.transferTokenToForeign(sender, token, recipient, value);
}

void bos_bridge::transfer2fe( std::string token, name recipient,
                             uint64_t value) {
  require_auth(_self);
}

void bos_bridge::transferfrof(name sender, std::string foreignToken,
                              name recipient, uint64_t value,
                              checksum256 transactionHash) {
  HomeBridge _HomeBridge(_self, _bridge_meta_parameters);
  _HomeBridge.transferFromForeign(sender, foreignToken, recipient,  value,
                           transactionHash);
}

void bos_bridge::submitsig(name sender, public_key sender_key, signature sig,
                           bytes message) {
  HomeBridge _HomeBridge(_self, _bridge_meta_parameters);
  _HomeBridge.submitSignature(sender, sender_key, sig, message);
}

void bos_bridge::collectedsig(name sender, checksum256 messageHash, uint64_t NumberOfCollectedSignatures)
{

}
/// HomeBridge end

// void bos_bridge::deposit(name from, name to, asset quantity) {
//    require_auth(from);
//    action(permission_level{_self, "active"_n}, "eosio.token"_n, "transfer"_n,
//    std::make_tuple(from, _self, quantity, std::string(""))).send();

//    if(quantity.symbol != core_symbol())
//    {
//    action(permission_level{_self, "active"_n}, "eosio.token"_n, "transfer"_n,
//    std::make_tuple(from, _self, quantity, std::string(""))).send();
//    action(permission_level{_self, "active"_n}, "eosio.token"_n, "retire"_n,
//    std::make_tuple(quantity, std::string(""))).send();
//    }

//    action(permission_level{_self, "active"_n}, _self, "transfertofe"_n,
//    std::make_tuple(to.to_string(),quantity.to_string())).send();
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
//    //   action(permission_level{_self, "active"_n}, "eosio.token"_n,
//    "create"_n, std::make_tuple(_self, maximum_supply)).send();

//       action(permission_level{_self, "active"_n}, "eosio.token"_n, "issue"_n,
//       std::make_tuple(_self, recipient, amount, std::string(""))).send();
//    }
//    else   {
//       action(permission_level{_self, "active"_n}, "eosio.token"_n,
//       "transfer"_n, std::make_tuple( _self,recipient, amount, std::string(""))).send();
//    }

// }

// void bos_bridge::transfertofe(string recipient, string amount) {
//    require_auth(_self);
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

// void bos_bridge::bridge_transfer(name from, name to, asset quantity, string
// memo, bool is_deferred) {
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
//    // check(quantity.symbol == st.supply.symbol, "symbol precision
//    mismatch"); check(memo.size() <= 256, "memo has more than 256 bytes");

//    //   token::transfer_action transfer_act{ token_account, { account,
//    //   active_permission } };
//    //          transfer_act.send( account, consumer_account, amount, memo );

//    //  auto payer = has_auth( to ) ? to : from;
//    // print("===quantity");
//    quantity.print();

//    if (!is_deferred) {
//       action(permission_level{from, "active"_n}, token_account, "transfer"_n,
//       std::make_tuple(from, to, quantity, memo)).send();
//    } else {
//       transaction t;
//       t.actions.emplace_back(permission_level{from, active_permission},
//       token_account, "transfer"_n, std::make_tuple(from, to, quantity,
//       memo)); t.delay_sec = 0; uint128_t deferred_id = (uint128_t(to.value)
//       << 64) | bos_bridge::current_time_point_sec().sec_since_epoch();
//       cancel_deferred(deferred_id);
//       t.send(deferred_id, _self, true);
//    }

//    // INLINE_ACTION_SENDER(eosio::token, transfer)(token_account, {{from,
//    // active_permission}, {to, active_permission}},{from, to, quantity,
//    memo});
// }

// } // namespace bosbridge

  void bos_bridge::settokenpara(name sender,bool is_home ,std::string token, uint64_t dailyLimit,
                        uint64_t maxPerTx, uint64_t minPerTx) {
    if (is_home) {
      HomeBridge _HomeBridge(_self, _bridge_meta_parameters);
      _HomeBridge.setTokenParameter(sender, token, dailyLimit, maxPerTx,
                                    minPerTx);
      return;
    }

    ForeignBridge _ForeignBridge(_self, _bridge_meta_parameters);

    _ForeignBridge.setTokenParameter(sender, token, dailyLimit, maxPerTx,
                                     minPerTx);
  }

/**
 * @brief  Sets config parameters
 *
 * @param version   parameters version  changed when paramters changes
 * @param parameters  config paramters such as core symbol,precision etc
 */
void bos_bridge::setparameter(ignore<uint8_t> version,
                              ignore<std::string> core_symbol,
                              ignore<uint8_t> precision,
                              ignore<bridge_parameters> foreign,
                              ignore<bridge_parameters> home) {
  require_auth(_self);
  uint8_t _version;
 std::string _core_symbol;
                              uint8_t _precision;
                              bridge_parameters _foreign;
                              bridge_parameters _home;
  _ds >> _version >> _core_symbol >> _precision >> _foreign >> _home;

  std::string checkmsg =
      "unsupported version for setparameter action,current_bridge_version =" +
      std::to_string(current_bridge_version);
  check(_version == current_bridge_version, checkmsg.c_str());
  _bridge_meta_parameters.version=_version;
  _bridge_meta_parameters.core_symbol = _core_symbol;
  _bridge_meta_parameters.precision = _precision;
  ForeignBridge _ForeignBridge(_self, _bridge_meta_parameters);

  _ForeignBridge.initialize(
      _foreign.validatorContractAddress, _foreign.dailyLimit, _foreign.maxPerTx,
      _foreign.minPerTx, _foreign.gasPrice, _foreign.requiredBlockConfirmations);
       HomeBridge _HomeBridge(_self, _bridge_meta_parameters);
  _HomeBridge.initialize(_home.validatorContractAddress, _home.dailyLimit,
                         _home.maxPerTx, _home.minPerTx, _home.gasPrice,
                         _home.requiredBlockConfirmations);
}

void bos_bridge::test(name sender, public_key sender_key, signature sig, bytes message)
{
message_data md = unpack<message_data>(message);
print("token=",md.token);
print("recipient=",md.recipient);
print("amount=",md.amount);
print("hash=",hash2str(md.txhash));
}

eosio::time_point_sec bos_bridge::current_time_point_sec() {
  const static eosio::time_point_sec cts{current_time_point()};
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
          bos_bridge, (impvalidator)(transfern2h)(transfert2h)(transfer2he)(
                          transferfrom)(regtoken)(transfern2f)(transfert2f)(
                          transfer2fe)(transferfrof)(submitsig)(setparameter)(settokenpara)(test)(create)(issue)(transfer)(open)(close)(retire)(addblacklist)(rmblacklist) )
    }
  }
  if (code == "eosio.token"_n.value && action == "transfer"_n.value) {

    datastream<const char *> ds = datastream<const char *>(nullptr, 0);

    bos_bridge thiscontract(name(receiver), name(code), ds);

    const transferx &t = unpack_action_data<transferx>();
    // thiscontract.on_transfer(t.from, t.to, t.quantity, t.memo);
  }
}
