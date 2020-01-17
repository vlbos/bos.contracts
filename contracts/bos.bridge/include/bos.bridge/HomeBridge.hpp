#pragma once

#include <eosio/eosio.hpp>
#include "bos.bridge/bos.config.hpp"
#include "bos.bridge/libraries/Message.hpp"
#include "bos.bridge/BasicBridge.hpp"
#include "bos.bridge/interfaces/IBurnableMintableToken.hpp"
#include "bos.bridge/HomeToken.hpp"

template <class TableType>
class HomeBridge : public BasicBridge<TableType,bridge_parameters_storage> {
protected:
  TableType& table;
  name self;

public:
  HomeBridge(name _self,TableType& _table)
      : BasicBridge<TableType,bridge_parameters_storage>(_self,_table,_table.home),self(_self), table(_table){}

        /* --- CONSTRUCTOR / INITIALIZATION --- */

    void initialize(name _validatorContract, uint64_t _dailyLimit,
                        uint64_t _maxPerTx, uint64_t _minPerTx,
                        uint64_t _homeGasPrice,
                        uint64_t _requiredBlockConfirmations) {
    require_auth(_validatorContract);
    check(is_account(_validatorContract),
          "Validator contract address cannot be 0x0");
    check(_homeGasPrice > 0, "HomeGasPrice should be greater than 0");
    check(_requiredBlockConfirmations > 0,
          "RequiredBlockConfirmations should be greater than 0");
    check(_minPerTx > 0 && _maxPerTx > _minPerTx && _dailyLimit > _maxPerTx,
          "Tx limits initialization error");

    eosio::extended_symbol core_token{symbol(table.core_symbol,table.precision),self};
    table.home.validatorContractAddress = _validatorContract;
    table.home.deployedAtBlock = current_block_time(); // block.number;
    table.home.dailyLimit[core_token] = _dailyLimit;
    table.home.maxPerTx[core_token] = _maxPerTx;
    table.home.minPerTx[core_token] = _minPerTx;
    table.home.gasPrice = _homeGasPrice;
    table.home.requiredBlockConfirmations = _requiredBlockConfirmations;
  }

  /* --- EXTERNAL / PUBLIC  METHODS --- */

  void registerToken(name sender,std::string foreignAddress, eosio::extended_symbol homeAddress) {
    require_auth(sender);
    check(table.foreignToHomeTokenMap.find(foreignAddress) == table.foreignToHomeTokenMap.end() &&
              table.homeToForeignTokenMap.find(homeAddress)==table.homeToForeignTokenMap.end(),
          "Token already registered");
    table.foreignToHomeTokenMap[foreignAddress] = homeAddress;
    table.homeToForeignTokenMap[homeAddress] = foreignAddress;
    HomeToken(self,foreignAddress).create();
  }

  void transferNativeToForeign(name sender,name recipient, uint64_t value) {
    require_auth(sender);
    eosio::extended_symbol core_token{symbol(table.core_symbol,table.precision),self};
    check(this->withinLimit(core_token, value), "Transfer exceeds limit");
    table.home.totalSpentPerDay[get_checksum256(core_token,this->getCurrentDay())] += value;

    auto foreignToken = table.homeToForeignTokenMap.find(core_token);
    check(foreignToken != table.homeToForeignTokenMap.end(), "Foreign native token address is empty");

    // emit TransferToForeign(foreignToken, recipient, msg.value);
    action(permission_level{self, "active"_n}, self, "transfer2fe"_n,std::make_tuple(foreignToken->second,recipient,value)).send();
  }

  void transferTokenToForeign(name sender,eosio::extended_symbol homeToken, name recipient,
                              uint64_t value) {
    require_auth(sender);
    check(this->withinLimit(homeToken, value), "Transfer exceeds limit");
    table.home.totalSpentPerDay[get_checksum256(homeToken,this->getCurrentDay())] += value;

    auto foreignToken = table.homeToForeignTokenMap.find(homeToken);
    check(foreignToken != table.homeToForeignTokenMap.end(), "Incorrect token address mapping no foreign token");
    auto ht = table.foreignToHomeTokenMap.find(foreignToken->second);
    check(ht != table.foreignToHomeTokenMap.end() &&  ht->second == homeToken, "Incorrect token address mapping not equal");

    HomeToken(self,homeToken).burn(sender,value);
    // emit TransferToForeign(foreignToken, recipient, value);
    action(permission_level{self, "active"_n}, self, "transfer2fe"_n,std::make_tuple(foreignToken,recipient,value)).send();
  }

  void transferFromForeign(name sender,std::string foreignToken, name recipient,
                           uint64_t value, checksum256 transactionHash) {
    require_auth(sender);
    auto  homeToken = table.foreignToHomeTokenMap.find(foreignToken);
    check(homeToken != table.foreignToHomeTokenMap.end() && isRegisterd(foreignToken, homeToken->second), "Token not registered");

    checksum256 hashMsg =
        get_checksum256(homeToken, recipient, value, transactionHash);
    checksum256 hashSender = get_checksum256(sender, hashMsg);
    // Duplicated transfers
    check(table.transfersSigned.find(hashSender)!=table.transfersSigned.end(),
          "Transfer already signed by this validator");
    table.transfersSigned[hashSender] = true;

    auto nsigned = table.numTransfersSigned.find(hashMsg);
    check(nsigned != table.numTransfersSigned && !isAlreadyProcessed(nsigned->second), "Transfer already processed");
    // the check above assumes that the case when the value could be overflew
    // will not happen in the addition operation below

    table.numTransfersSigned[hashMsg] = ++nsigned;

    // emit SignedForTransferFromForeign(msg.sender, transactionHash);

    if (nsigned >= this->requiredSignatures()) {
      // If the bridge contract does not own enough tokens to transfer
      // it will cause funds lock on the home side of the bridge
      table.numTransfersSigned[hashMsg] = markAsProcessed(nsigned);

      // Passing the mapped home token address here even when token address is
      // 0x0. This is okay because by default the address mapped to 0x0 will
      // also be 0x0
      performTransfer(homeToken, recipient, value);
      // emit TransferFromForeign(homeToken, recipient, value, transactionHash);
    }
  }

  void submitSignature(name sender, public_key sender_key, signature sig,
                       bytes message) {
    require_auth(sender);
    // ensure that `signature` is really `message` signed by `msg.sender`
    check(Message::isMessageValid(message), "Invalid message format");
    check(sender_key == Message::recoverAddressFromSignedMessage(sig, message),
          "Sender is not signer of message");
    eosio::checksum256 hashMsg = get_checksum256(message);
    eosio::checksum256 hashSender = get_checksum256(sender, hashMsg);

    auto it = table.numTransfersSigned.find(hashMsg);
    check(it != table.numTransfersSigned && !isAlreadyProcessed(it->second), "Transfer already processed");
    // the check above assumes that the case when the value could be overflew
    // will not happen in the addition operation below
    uint64_t nsigned = it->second + 1;
    if (nsigned > 1) {
      // Duplicated signatures
      check(table.messagesSigned.find(hashSender) != table.messagesSigned.end(),
              "Message already signed by this validator");
    } else {
      table.messages[hashMsg] = message;
    }
    table.messagesSigned[hashSender] = true;

    eosio::checksum256 signIdx = get_checksum256(hashMsg, (nsigned - 1));
    table.signatures[signIdx] = sig;

    table.numMessagesSigned[hashMsg] = nsigned;

    // emit SignedForTransferToForeign(msg.sender, hashMsg);

    uint64_t reqSigs = this->requiredSignatures();
    if (nsigned >= reqSigs) {
      table.numMessagesSigned[hashMsg] = markAsProcessed(nsigned);
      // emit CollectedSignatures(msg.sender, hashMsg, reqSigs);
    }
  }

private:
  /* --- INTERNAL / PRIVATE METHODS --- */

  void performTransfer(eosio::extended_symbol token, name recipient, uint64_t value) {
    if (token.sym==symbol(table.core_symbol,table.precision)) {
      action(
          permission_level{self, "active"_n}, token.contract, "transfer"_n,
          std::make_tuple(self, recipient, asset(value, token.sym), ""))
          .send();
      return;
    }

    HomeToken(self,token).mint(recipient, value);
  }

  bool isRegisterd(eosio::extended_symbol foreignToken, eosio::extended_symbol homeToken) {
    if (table.foreignToHomeTokenMap.find(foreignToken)== table.foreignToHomeTokenMap.end() || table.homeToForeignTokenMap.find(homeToken)==table.homeToForeignTokenMap.end()) {
      return false;
    } else {
      return (table.foreignToHomeTokenMap[foreignToken] == homeToken &&
              table.homeToForeignTokenMap[homeToken] == foreignToken);
    }
  }

  checksum256 signature(checksum256 _hash, uint64_t _index) {
        checksum256 signIdx = get_checksum256(_hash, _index);
        auto it = table.signatures.find(signIdx);
        check(it != table.signatures.end(),"no found signindex");
        return it->second;
    }

    bytes message(checksum256 _hash)  {
       auto it  = table.messages[_hash];
       check(it != table.messages.end(),"no found hash");
        return it->second;
    }

    uint64_t markAsProcessed(uint64_t _v)  {
        return _v|(uint64_t)pow(2,255);
    }

    bool isAlreadyProcessed(uint64_t _number)  {
        return (_number&(uint64_t)pow(2,255)) == (uint64_t)pow(2,255);
    }
};
