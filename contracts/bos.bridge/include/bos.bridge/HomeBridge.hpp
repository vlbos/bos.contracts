#include <eosio/eosio.hpp>
#include "bos.bridge/bos.config.hpp"
#include "bos.bridge/libraries/Message.hpp"
#include "bos.bridge/BasicBridge.hpp"
#include "bos.bridge/interfaces/IBurnableMintableToken.hpp"

template <class TableType, typename Iterator>
class HomeBridge : public BasicBridge<TableType,Iterator> {
protected:
  TableType table;
  Iterator& it;
  name self;

public:
  HomeBridge(name _self,Iterator& itr)
      : self(_self), table(_self, _self.value),it(itr)

        /* --- CONSTRUCTOR / INITIALIZATION --- */

    void initialize(name _validatorContract, uint64_t _dailyLimit,
                        uint64_t _maxPerTx, uint64_t _minPerTx,
                        uint64_t _homeGasPrice,
                        uint64_t _requiredBlockConfirmations) {
    check(_validatorContract != name(),
          "Validator contract address cannot be 0x0");
    check(_homeGasPrice > 0, "HomeGasPrice should be greater than 0");
    check(_requiredBlockConfirmations > 0,
          "RequiredBlockConfirmations should be greater than 0");
    check(_minPerTx > 0 && _maxPerTx > _minPerTx && _dailyLimit > _maxPerTx,
          "Tx limits initialization error");

    validatorContractAddress = _validatorContract;
    deployedAtBlock = block.number;
    dailyLimit[name()] = _dailyLimit;
    maxPerTx[name()] = _maxPerTx;
    minPerTx[name()] = _minPerTx;
    gasPrice = _homeGasPrice;
    requiredBlockConfirmations = _requiredBlockConfirmations;
  }

  /* --- EXTERNAL / PUBLIC  METHODS --- */

  void registerToken(name foreignAddress, name homeAddress) {
    check(foreignToHomeTokenMap[foreignAddress] == name() &&
              homeToForeignTokenMap[homeAddress] == name(),
          "Token already registered");
    foreignToHomeTokenMap[foreignAddress] = homeAddress;
    homeToForeignTokenMap[homeAddress] = foreignAddress;
  }

  void transferNativeToForeign(name recipient, uint64_t value) {
    check(withinLimit(name(), value), "Transfer exceeds limit");
    totalSpentPerDay[name()][getCurrentDay()] += value;

    address foreignToken = homeToForeignTokenMap[name()];
    check(foreignToken != name(), "Foreign native token address is not 0x0");

    // emit TransferToForeign(foreignToken, recipient, msg.value);
  }

  void transferTokenToForeign(std::string homeToken, name recipient,
                              uint64_t value) {
    check(withinLimit(homeToken, value), "Transfer exceeds limit");
    totalSpentPerDay[homeToken][getCurrentDay()] += (value);

    std::string foreignToken = homeToForeignTokenMap[homeToken];
    check(foreignToHomeTokenMap[foreignToken] == homeToken,
          "Incorrect token address mapping");

    IBurnableMintableToken(homeToken).burn(value);
    // emit TransferToForeign(foreignToken, recipient, value);
  }

  void transferFromForeign(std::string foreignToken, name recipient,
                           uint64_t value, checksum256 transactionHash) {
    std::string homeToken = foreignToHomeTokenMap[foreignToken];
    check(isRegisterd(foreignToken, homeToken), "Token not registered");

    checksum256 hashMsg =
        get_checksum256(homeToken, recipient, value, transactionHash);
    checksum256 hashSender = get_checksum256(msg.sender, hashMsg);
    // Duplicated transfers
    check(!transfersSigned[hashSender],
          "Transfer already signed by this validator");
    transfersSigned[hashSender] = true;

    uint64_t signed = numTransfersSigned[hashMsg];
    check(!isAlreadyProcessed(signed), "Transfer already processed");
    // the check above assumes that the case when the value could be overflew
    // will not happen in the addition operation below
    signed = signed + 1;

    numTransfersSigned[hashMsg] = signed;

    // emit SignedForTransferFromForeign(msg.sender, transactionHash);

    if (signed >= requiredSignatures()) {
      // If the bridge contract does not own enough tokens to transfer
      // it will cause funds lock on the home side of the bridge
      numTransfersSigned[hashMsg] = markAsProcessed(signed);

      // Passing the mapped home token address here even when token address is
      // 0x0. This is okay because by default the address mapped to 0x0 will
      // also be 0x0
      performTransfer(homeToken, recipient, value);
      // emit TransferFromForeign(homeToken, recipient, value, transactionHash);
    }
  }

  void submitSignature(name sender, public_key sender_key, signature sig,
                       bytes message) {
    // ensure that `signature` is really `message` signed by `msg.sender`
    check(Message::isMessageValid(message), "Invalid message format");
    check(sender_key == Message::recoverAddressFromSignedMessage(sig, message),
          "Sender is not signer of message");
    eosio::checksum256 hashMsg = get_checksum256(message);
    eosio::checksum256 hashSender = get_checksum256(sender, hashMsg);

    uint64_t signed = numMessagesSigned[hashMsg];
    require(!isAlreadyProcessed(signed), "Transfer already processed");
    // the check above assumes that the case when the value could be overflew
    // will not happen in the addition operation below
    signed = signed + 1;
    if (signed > 1) {
      // Duplicated signatures
      require(!messagesSigned[hashSender],
              "Message already signed by this validator");
    } else {
      messages[hashMsg] = message;
    }
    messagesSigned[hashSender] = true;

    eosio::checksum256 signIdx = get_checksum256(hashMsg, (signed - 1));
    signatures[signIdx] = sig;

    numMessagesSigned[hashMsg] = signed;

    // emit SignedForTransferToForeign(msg.sender, hashMsg);

    uint64_t reqSigs = requiredSignatures();
    if (signed >= reqSigs) {
      numMessagesSigned[hashMsg] = markAsProcessed(signed);
      // emit CollectedSignatures(msg.sender, hashMsg, reqSigs);
    }
  }

  /* --- INTERNAL / PRIVATE METHODS --- */

  void performTransfer(std::string token, name recipient, uint64_t value) {
    if (token.empty()) {
      recipient.transfer(value);
      return;
    }

    IBurnableMintableToken(token).mint(recipient, value);
  }

  bool isRegisterd(std::string foreignToken, std::string homeToken) {
    if (foreignToken.empty() && homeToken.empty()) {
      return false;
    } else {
      return (foreignToHomeTokenMap[foreignToken] == homeToken &&
              homeToForeignTokenMap[homeToken] == foreignToken);
    }
  }

  checksum256 signature(checksum256 _hash, uint64_t _index) {
        checksum256 signIdx = get_checksum256(_hash, _index));
        return signatures[signIdx];
    }

    bytes message(checksum256 _hash)  {
        return messages[_hash];
    }

    uint64_t markAsProcessed(uint64_t _v)  {
        return _v | pow(2,255);
    }

    bool isAlreadyProcessed(uint64_t _number)  {
        return _number & pow(2,255) == pow(2,255);
    }
};
