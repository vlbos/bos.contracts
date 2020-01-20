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

    std::string core_token="eosio.token:"+table.core_symbol+":"+std::to_string(table.precision);
    table.home.validatorContractAddress = _validatorContract;
    table.home.deployedAtBlock = current_block_time(); // block.number;
    table.home.dailyLimit[core_token] = _dailyLimit;
    table.home.maxPerTx[core_token] = _maxPerTx;
    table.home.minPerTx[core_token] = _minPerTx;
    table.home.gasPrice = _homeGasPrice;
    table.home.requiredBlockConfirmations = _requiredBlockConfirmations;
  }

  /* --- EXTERNAL / PUBLIC  METHODS --- */

  void registerToken(name sender,std::string foreignAddress, std::string homeAddress) {
    require_auth(sender);
    check(this->validatorContract()->isValidator(sender), "Signer of registerToken is not a validator ");
    check(table.foreignToHomeTokenMap.find(foreignAddress) == table.foreignToHomeTokenMap.end() &&
              table.homeToForeignTokenMap.find(homeAddress)==table.homeToForeignTokenMap.end(),
          "Token already registered");
    table.foreignToHomeTokenMap[foreignAddress] = homeAddress;
    table.homeToForeignTokenMap[homeAddress] = foreignAddress;
    symbol sym=bos_bridge::str2sym(homeAddress);
    name contract=bos_bridge::str2contract(homeAddress);
    
    if (self != contract ) {
      return;
    }

    HomeToken(self,homeAddress).create();
  }

  void transferNativeToForeign(name sender,name recipient, uint64_t value) {
    require_auth(sender);
    std::string core_token="eosio.token:"+table.core_symbol+":"+std::to_string(table.precision);
    check(this->withinLimit(core_token, value), "Transfer exceeds limit");
    table.home.totalSpentPerDay[get_checksum256(core_token,this->getCurrentDay())] += value;

    auto foreignToken = table.homeToForeignTokenMap.find(core_token);
    check(foreignToken != table.homeToForeignTokenMap.end(), "Foreign native token address is empty");

   asset quantity= asset(value,symbol(table.core_symbol,table.precision));
   std::string memo = "";
   action(permission_level{sender, "active"_n}, "eosio.token"_n, "transfer"_n,
   std::make_tuple(sender, self, quantity, memo)).send();

    // emit TransferToForeign(foreignToken, recipient, msg.value);
    action(permission_level{self, "active"_n}, self, "transfer2fe"_n,std::make_tuple(foreignToken->second,recipient,value)).send();
  }

  void transferTokenToForeign(name sender,std::string homeToken, name recipient,
                              uint64_t value) {
    require_auth(sender);
    symbol sym=bos_bridge::str2sym(homeToken);
    name contract=bos_bridge::str2contract(homeToken);
    if ("eosio.token"_n == contract && sym==symbol(table.core_symbol,table.precision)) {
      return;
    }

    check(this->withinLimit(homeToken, value), "Transfer exceeds limit");
    table.home.totalSpentPerDay[get_checksum256(homeToken,this->getCurrentDay())] += value;

    print(table.homeToForeignTokenMap.size(),"===",table.foreignToHomeTokenMap.size());
    for(auto t:table.homeToForeignTokenMap)
    {
      print(t.first,"==",t.second);
    }

    auto foreignToken = table.homeToForeignTokenMap.find(homeToken);
    check(foreignToken != table.homeToForeignTokenMap.end(), "Incorrect token address mapping no foreign token");
    auto ht = table.foreignToHomeTokenMap.find(foreignToken->second);
    check(ht != table.foreignToHomeTokenMap.end() &&  ht->second == homeToken, "Incorrect token address mapping not equal");
    
 
    asset quantity= asset(value, sym);
    std::string memo = "";
    action(permission_level{sender, "active"_n}, contract, "transfer"_n,
    std::make_tuple(sender, self, quantity, memo)).send();

    if(self == contract)
    {
    HomeToken(self,homeToken).burn(sender,value);
    }

    // emit TransferToForeign(foreignToken, recipient, value);
    action(permission_level{self, "active"_n}, self, "transfer2fe"_n,std::make_tuple(foreignToken->second,recipient,value)).send();
  }

  void transferFromForeign(name sender,std::string foreignToken, name recipient,
                           uint64_t value, checksum256 transactionHash) {
    require_auth(sender);
    check(this->validatorContract()->isValidator(sender), "Signer of message is not a validator transferFromForeign");

    auto  homeToken = table.foreignToHomeTokenMap.find(foreignToken);
    check(homeToken != table.foreignToHomeTokenMap.end() && isRegisterd(foreignToken, homeToken->second), "Token not registered");

    checksum256 hashMsg = get_checksum256(homeToken->second, recipient, value, transactionHash);
    checksum256 hashSender = get_checksum256(sender, hashMsg);
    // Duplicated transfers
    check(table.transfersSigned.find(hashSender)==table.transfersSigned.end(),
          "Transfer already signed by this validator");
    table.transfersSigned[hashSender] = true;
    uint64_t nsigned = 0;

    auto it = table.numTransfersSigned.find(hashMsg);
    // check(it == table.numTransfersSigned.end() || !isAlreadyProcessed(it->second), "Transfer already processed");
    // the check above assumes that the case when the value could be overflew
    // will not happen in the addition operation below
    if(it != table.numTransfersSigned.end())
    {
       nsigned = it->second;
       check(!isAlreadyProcessed(it->second), "Transfer already processed");
    }
    table.numTransfersSigned[hashMsg] = ++nsigned;

    // emit SignedForTransferFromForeign(msg.sender, transactionHash);

    if (nsigned >= this->requiredSignatures()) {
      // If the bridge contract does not own enough tokens to transfer
      // it will cause funds lock on the home side of the bridge
      table.numTransfersSigned[hashMsg] = markAsProcessed(nsigned);

      // Passing the mapped home token address here even when token address is
      // 0x0. This is okay because by default the address mapped to 0x0 will
      // also be 0x0
      performTransfer(homeToken->second, recipient, value);
      // emit TransferFromForeign(homeToken, recipient, value, transactionHash);
    }
    else
    {
      print(nsigned, "============else if (nsigned >= this->requiredSignatures())==",this->requiredSignatures());
    }
    
  }

  void submitSignature(name sender, public_key sender_key, signature sig,
                       bytes message) {
    require_auth(sender);
    check(this->validatorContract()->isValidator(sender), "Signer of message is not a validator submitSignature");
    // ensure that `signature` is really `message` signed by `msg.sender`
    check(Message::isMessageValid(message), "Invalid message format");
    check(sender_key == Message::recoverAddressFromSignedMessage(sig, message),
          "Sender is not signer of message");
    eosio::checksum256 hashMsg = get_checksum256(message);
    eosio::checksum256 hashSender = get_checksum256(sender, hashMsg);
    uint64_t nsigned = 0;
    auto it = table.numMessagesSigned.find(hashMsg);
    // check(it == table.numTransfersSigned.end() || !isAlreadyProcessed(it->second), "Transfer already processed");
    // the check above assumes that the case when the value could be overflew
    // will not happen in the addition operation below
    if(it != table.numMessagesSigned.end())
    {
       nsigned = it->second;
       check(!isAlreadyProcessed(it->second), "Transfer already processed");
    }

    nsigned++;
    if (nsigned > 1) {
      // Duplicated signatures
      check(table.messagesSigned.find(hashSender) == table.messagesSigned.end(),
              "Message already signed by this validator");
    } else {
      table.messages[hashMsg] = message;
    }
    table.messagesSigned[hashSender] = true;

    uint64_t nn = (nsigned - 1);
    eosio::checksum256 signIdx = get_checksum256(hashMsg, nn);
    table.signatures[signIdx] = sig;

    table.numMessagesSigned[hashMsg] = nsigned;

    // emit SignedForTransferToForeign(msg.sender, hashMsg);

    uint64_t reqSigs = this->requiredSignatures();
    if (nsigned >= reqSigs) {
      table.numMessagesSigned[hashMsg] = markAsProcessed(nsigned);
      // emit CollectedSignatures(msg.sender, hashMsg, reqSigs);
       action(permission_level{self, "active"_n}, self, "collectedsig"_n,std::make_tuple(sender,hashMsg,reqSigs)).send();
    }
    else
    {
     print(nsigned, "============else if (nsigned >= reqSigs())==",reqSigs);
    }
    
  }

private:
  /* --- INTERNAL / PRIVATE METHODS --- */
  void performTransfer(std::string token, name recipient, uint64_t value) {
    symbol sym=bos_bridge::str2sym(token);
    name contract=bos_bridge::str2contract(token);
  print("============ performTransfer ff");
    if ("eosio.token"_n == contract && sym==symbol(table.core_symbol,table.precision)) {
       print("============  == contract && sym==symbol(table.core_symbol,table.precision)) ");
      std::string memo = "";
           action(permission_level{self, "active"_n}, contract, "transfer"_n,
          std::make_tuple(self, recipient, asset(value,sym), memo)).send();
      return;
    }

    if(self==contract)
    {
    HomeToken(self,token).mint(recipient, value);
    }
  }

  bool isRegisterd(std::string foreignToken, std::string homeToken) {
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
        return _v|(uint64_t)pow(2,62);
    }

    bool isAlreadyProcessed(uint64_t _number)  {
        return (_number&(uint64_t)pow(2,62)) == (uint64_t)pow(2,62);
    }
};
