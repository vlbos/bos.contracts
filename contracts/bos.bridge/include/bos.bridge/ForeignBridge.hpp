#pragma once

#include <eosio/eosio.hpp>
#include "bos.bridge/bos.config.hpp"
#include "bos.bridge/libraries/Message.hpp"
#include "bos.bridge/BasicBridge.hpp"

template <class TableType>
class ForeignBridge : public BasicBridge<TableType,bridge_parameters_storage> {
protected:
  TableType& table;
  name self;

public:
  ForeignBridge(name _self,TableType& _table)
      : BasicBridge<TableType,bridge_parameters_storage>(_self,_table,_table.foreign),self(_self), table(_table)
      {

      }
        /* --- CONSTRUCTOR / INITIALIZATION --- */

    void initialize(name _validatorContract, uint64_t _dailyLimit,
                        uint64_t _maxPerTx, uint64_t _minPerTx,
                        uint64_t _foreignGasPrice,
                        uint64_t _requiredBlockConfirmations) {
                    require_auth(_validatorContract);
    check(is_account(_validatorContract),
          "Validator contract address cannot be empty");
    check(_minPerTx > 0 && _maxPerTx > _minPerTx && _dailyLimit > _maxPerTx,
          "Tx limits initialization error");
    check(_foreignGasPrice > 0, "ForeignGasPrice should be greater than 0");

    table.foreign.validatorContractAddress = _validatorContract;
    table.foreign.deployedAtBlock = current_block_time(); ///block.number;
    table.foreign.dailyLimit[table.core_symbol] = _dailyLimit;
    table.foreign.maxPerTx[table.core_symbol] = _maxPerTx;
    table.foreign.minPerTx[table.core_symbol] = _minPerTx;
    table.foreign.gasPrice = _foreignGasPrice;
    table.foreign.requiredBlockConfirmations = _requiredBlockConfirmations;
  }

  /* --- EXTERNAL / PUBLIC  METHODS --- */

  void transferNativeToHome(name sender,name _recipient,uint64_t value) {
    require_auth(sender);
    check(this->withinLimit(table.core_symbol, value), "Transfer exceeds limit");
    table.foreign.totalSpentPerDay[get_checksum256(table.core_symbol,this->getCurrentDay())] += value;
    // emit TransferToHome(table.core_symbol, _recipient, msg.value);

  }

  void transferTokenToHome(name sender,std::string _token, name _recipient, uint64_t _value) {
     require_auth(sender);
    uint64_t castValue18 = _value;//castTo18Decimal(_token, _value);
    check(this->withinLimit(_token, castValue18), "Transfer exceeds limit");
    table.foreign.totalSpentPerDay[get_checksum256(_token,this->getCurrentDay())] += castValue18;

    // if (_token == USDTAddress) {
    //   // Handle USDT special case since it does not have standard erc20 token
    //   // interface =.=
    //   //   uint64_t balanceBefore = USDT(_token).balanceOf(this);
    //   //   USDT(_token).transferFrom(msg.sender, this, _value);
    //   // check transfer suceeded
    //   //   check(balanceBefore.add(_value) == USDT(_token).balanceOf(this));
    // } else {
    //   //   check(ERC20Token(_token).transferFrom(msg.sender, this, _value),
    //   //   "TransferFrom failed for ERC20 Token");
    // }
    // // emit TransferToHome(_token, _recipient, castValue18);
  }

  void transferFromHome(name sender,std::vector<signature> sig, bytes message) {
    require_auth(sender);
    Message::hasEnoughValidSignatures(message, sig, this->validatorContract());
  
    std::tuple<std::string, name, int64_t, checksum256> msg = Message::parseMessage(message);
   std::string token=std::get<0>(msg);
    name recipient=std::get<1>(msg);
    uint64_t amount=std::get<2>(msg);
    checksum256 txHash=std::get<3>(msg);
    check(!table.transfers[txHash], "Transfer already processed");
    table.transfers[txHash] = true;
    uint64_t castedAmount = amount;//castFrom18Decimal(token, amount);
    performTransfer(token, recipient, castedAmount);
    // emit TransferFromHome(token, recipient, castedAmount, txHash);
  }
private:

   /* --- INTERNAL / PRIVATE METHODS --- */

  void performTransfer(std::string tokenAddress, name recipient,
                       uint64_t amount) {
    if (tokenAddress == table.core_symbol) {
      action(permission_level{self, "active"_n}, bos_bridge::token_account, "transfer"_n,std::make_tuple(self, recipient, asset(amount,symbol(table.core_symbol,4)), "")).send();
      //recipient.transfer(amount);
      return;
    }

    // if (tokenAddress == USDTAddress) {
    //     uint64_t balanceBefore = USDT(tokenAddress).balanceOf(recipient);
    //     USDT(tokenAddress).transfer(recipient, amount);
    //     // check transfer suceeded
    //     check(balanceBefore.add(amount) ==
    //     USDT(tokenAddress).balanceOf(recipient)); return;
    // }

    // std::string token = (tokenAddress);
    // check(token.transfer(recipient, amount), "Transfer failed for ERC20 token");
    action(permission_level{self, "active"_n}, bos_bridge::token_account, "transfer"_n,std::make_tuple(self, recipient, asset(amount,symbol(table.core_symbol,4)), "")).send();

  }

  void castTo18Decimal(std::string token, uint64_t value) {
    return value*getCastScale(token, value);
  }

  uint64_t castFrom18Decimal(std::string token, uint64_t value) {
    if (token.empty()) {
      return value;
    }

    return value / (getCastScale(token, value));
  }

  uint64_t getCastScale(std::string token, uint64_t value) {
    // check((token).decimals() > 0 && (token).decimals() <= 18);

    // if ((token).decimals() == 18) {
    //   return 1;
    // }

    // uint64_t decimals = uint64_t((token).decimals()); // cast to uint64_t
    // return pow(10, (18 - decimals));
  }
};
