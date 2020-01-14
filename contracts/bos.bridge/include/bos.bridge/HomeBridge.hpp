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

    table.home.validatorContractAddress = _validatorContract;
    table.home.deployedAtBlock = current_block_time(); // block.number;
    table.home.dailyLimit[table.core_symbol] = _dailyLimit;
    table.home.maxPerTx[table.core_symbol] = _maxPerTx;
    table.home.minPerTx[table.core_symbol] = _minPerTx;
    table.home.gasPrice = _homeGasPrice;
    table.home.requiredBlockConfirmations = _requiredBlockConfirmations;
  }

  /* --- EXTERNAL / PUBLIC  METHODS --- */

  void registerToken(name sender,std::string foreignAddress, std::string homeAddress) {
    require_auth(sender);
    check(table.foreignToHomeTokenMap[foreignAddress].empty() &&
              table.homeToForeignTokenMap[homeAddress].empty(),
          "Token already registered");
    table.foreignToHomeTokenMap[foreignAddress] = homeAddress;
    table.homeToForeignTokenMap[homeAddress] = foreignAddress;
    HomeToken(self,foreignAddress).create();
  }

  void transferNativeToForeign(name sender,name recipient, uint64_t value) {
    require_auth(sender);
    check(this->withinLimit(table.core_symbol, value), "Transfer exceeds limit");
    table.home.totalSpentPerDay[get_checksum256(table.core_symbol,this->getCurrentDay())] += value;

    std::string foreignToken = table.homeToForeignTokenMap[table.core_symbol];
    check(!foreignToken.empty(), "Foreign native token address is empty");

    // emit TransferToForeign(foreignToken, recipient, msg.value);
       action(permission_level{self, "active"_n}, self, "transfer2fe"_n,std::make_tuple(foreignToken,recipient,value)).send();
  }

  void transferTokenToForeign(name sender,std::string homeToken, name recipient,
                              uint64_t value) {
                                require_auth(sender);
    check(this->withinLimit(homeToken, value), "Transfer exceeds limit");
    table.home.totalSpentPerDay[get_checksum256(homeToken,this->getCurrentDay())] += value;

    std::string foreignToken = table.homeToForeignTokenMap[homeToken];
    check(table.foreignToHomeTokenMap[foreignToken] == homeToken,
          "Incorrect token address mapping");

    HomeToken(self,homeToken).burn(sender,value);
    // emit TransferToForeign(foreignToken, recipient, value);
     action(permission_level{self, "active"_n}, self, "transfer2fe"_n,std::make_tuple(foreignToken,recipient,value)).send();
  }

  void transferFromForeign(name sender,std::string foreignToken, name recipient,
                           uint64_t value, checksum256 transactionHash) {
                             require_auth(sender);
    std::string homeToken = table.foreignToHomeTokenMap[foreignToken];
    check(isRegisterd(foreignToken, homeToken), "Token not registered");

    checksum256 hashMsg =
        get_checksum256(homeToken, recipient, value, transactionHash);
    checksum256 hashSender = get_checksum256(sender, hashMsg);
    // Duplicated transfers
    check(!table.transfersSigned[hashSender],
          "Transfer already signed by this validator");
    table.transfersSigned[hashSender] = true;

    uint64_t nsigned = table.numTransfersSigned[hashMsg];
    check(!isAlreadyProcessed(nsigned), "Transfer already processed");
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

    uint64_t nsigned = table.numMessagesSigned[hashMsg];
    check(!isAlreadyProcessed(nsigned), "Transfer already processed");
    // the check above assumes that the case when the value could be overflew
    // will not happen in the addition operation below
    nsigned = nsigned + 1;
    if (nsigned > 1) {
      // Duplicated signatures
      check(!table.messagesSigned[hashSender],
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

  void performTransfer(std::string token, name recipient, uint64_t value) {
    if (token==table.core_symbol) {
      action(
          permission_level{self, "active"_n}, "eosio.token"_n, "transfer"_n,
          std::make_tuple(self, recipient, asset(value, symbol(table.core_symbol,4)), ""))
          .send();
      return;
    }

    HomeToken(self,token).mint(recipient, value);
  }

  bool isRegisterd(std::string foreignToken, std::string homeToken) {
    if (foreignToken.empty() && homeToken.empty()) {
      return false;
    } else {
      return (table.foreignToHomeTokenMap[foreignToken] == homeToken &&
              table.homeToForeignTokenMap[homeToken] == foreignToken);
    }
  }

  checksum256 signature(checksum256 _hash, uint64_t _index) {
        checksum256 signIdx = get_checksum256(_hash, _index);
        return table.signatures[signIdx];
    }

    bytes message(checksum256 _hash)  {
        return table.messages[_hash];
    }

    uint64_t markAsProcessed(uint64_t _v)  {
        return _v|(uint64_t)pow(2,255);
    }

    bool isAlreadyProcessed(uint64_t _number)  {
        return (_number&(uint64_t)pow(2,255)) == (uint64_t)pow(2,255);
    }
};
