#include <eosio/eosio.hpp>

#include "bos.bridge/libraries/Message.hpp"
#include "bos.bridge/BasicBridge.hpp"

class ForeignBridge :public BasicBridge {

    /* --- EVENTS --- */

    // // Triggered when relay of transfer from HomeBridge is complete
    // event TransferFromHome(address indexed token, address recipient, uint value, bytes32 indexed transactionHash);
    // // Event created on transfer to home.
    // event TransferToHome(address indexed token, address recipient, uint64_t value);

    /* --- FIELDS --- */

    /* Beginning of V1 storage variables */
    // mapping between the transfer transaction hash from the HomeBridge to whether the transfer has been processed
    // mapping(bytes32 => bool) public transfers;
    /* End of V1 storage variables */

    /* End of V2 storage variables */

    /* --- CONSTRUCTOR / INITIALIZATION --- */

    void initialize(
      name _validatorContract,
      uint64_t _dailyLimit,
      uint64_t _maxPerTx,
      uint64_t _minPerTx,
      uint64_t _foreignGasPrice,
      uint64_t _requiredBlockConfirmations
    ) 
    {
        check(_validatorContract != address(0), "Validator contract address cannot be 0x0");
        check(_minPerTx > 0 && _maxPerTx > _minPerTx && _dailyLimit > _maxPerTx, "Tx limits initialization error");
        check(_foreignGasPrice > 0, "ForeignGasPrice should be greater than 0");

        validatorContractAddress = _validatorContract;
        deployedAtBlock = block.number;
        dailyLimit[address(0)] = _dailyLimit;
        maxPerTx[address(0)] = _maxPerTx;
        minPerTx[address(0)] = _minPerTx;
        gasPrice = _foreignGasPrice;
        requiredBlockConfirmations = _requiredBlockConfirmations;
    }

    /* --- EXTERNAL / PUBLIC  METHODS --- */

    void transferNativeToHome(name _recipient){
        check(withinLimit(name(), msg.value), "Transfer exceeds limit");
        totalSpentPerDay[name()][getCurrentDay()] = totalSpentPerDay[name()][getCurrentDay()]=(msg.value);
        // emit TransferToHome(address(0), _recipient, msg.value);
    }

    void transferTokenToHome(symbol _token, name _recipient, uint64_t _value)  {
        uint64_t castValue18 = castTo18Decimal(_token, _value);
        check(withinLimit(_token, castValue18), "Transfer exceeds limit");
        totalSpentPerDay[_token][getCurrentDay()] = totalSpentPerDay[_token][getCurrentDay()].add(castValue18);

        if (_token == USDTAddress) {
          // Handle USDT special case since it does not have standard erc20 token interface =.=
        //   uint64_t balanceBefore = USDT(_token).balanceOf(this);
        //   USDT(_token).transferFrom(msg.sender, this, _value);
          // check transfer suceeded
        //   check(balanceBefore.add(_value) == USDT(_token).balanceOf(this));
        } else {
        //   check(ERC20Token(_token).transferFrom(msg.sender, this, _value), "TransferFrom failed for ERC20 Token");
        }
        // emit TransferToHome(_token, _recipient, castValue18);
    }

    void transferFromHome(signature sig, bytes message)  {
        Message::hasEnoughValidSignatures(message, sig,validatorContract());
        // symbol token;
        // name recipient;
        // uint64_t amount;
        // bytes txHash;
        std::tuple<name,name,int64_t,bytes> msg = Message::parseMessage(message);
        check(!transfers[std::get<2>(msg)], "Transfer already processed");
        transfers[txHash] = true;

        uint64_t castedAmount = castFrom18Decimal(token, amount);
        performTransfer(token, recipient, castedAmount);
        // emit TransferFromHome(token, recipient, castedAmount, txHash);

    }

    void setUSDTAddress(name _addr)  {
        USDTAddress = _addr;
    }

    /* --- INTERNAL / PRIVATE METHODS --- */

    void performTransfer(address tokenAddress, address recipient, uint64_t amount)  {
        if (tokenAddress == address(0)) {
            recipient.transfer(amount);
            return;
        }

        // if (tokenAddress == USDTAddress) {
        //     uint64_t balanceBefore = USDT(tokenAddress).balanceOf(recipient);
        //     USDT(tokenAddress).transfer(recipient, amount);
        //     // check transfer suceeded
        //     check(balanceBefore.add(amount) == USDT(tokenAddress).balanceOf(recipient));
        //     return;
        // }

        ERC20Token token = ERC20Token(tokenAddress);
        check(token.transfer(recipient, amount), "Transfer failed for ERC20 token");
    }

    void castTo18Decimal(symbol token, uint64_t value) {
        return value.mul(getCastScale(token, value));
    }

    uint64_t castFrom18Decimal(symbol token, uint64_t value) {
        if (token == address()) {
            return value;
        }

        return value.div(getCastScale(token, value));
    }

    uint64_t getCastScale(symbol token, uint64_t value)  {
      check(ERC20Token(token).decimals() > 0 && ERC20Token(token).decimals() <= 18);

      if (ERC20Token(token).decimals() == 18) {
          return 1;
      }

      uint64_t decimals = uint64_t(ERC20Token(token).decimals()); // cast to uint64_t
      return 10**(18 - decimals);
    }
};