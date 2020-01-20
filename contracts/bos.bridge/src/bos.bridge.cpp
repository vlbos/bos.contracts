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
 require_auth(_self);
}
/// HomeBridge end


  void bos_bridge::settokenpara(name sender,bool ishome ,std::string token,uint64_t minPerTx,
                        uint64_t maxPerTx,uint64_t dailyLimit) {
    if (ishome) {
      HomeBridge _HomeBridge(_self, _bridge_meta_parameters);
      _HomeBridge.setTokenParameter(sender, token,minPerTx, maxPerTx, dailyLimit);
      return;
    }

    ForeignBridge _ForeignBridge(_self, _bridge_meta_parameters);

    _ForeignBridge.setTokenParameter(sender, token, minPerTx, maxPerTx, dailyLimit);
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
