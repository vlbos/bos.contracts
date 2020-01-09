
#include "bos.bridge/interfaces/IBridgeValidators.hpp";

class BasicBridge {

    // /* --- EVENTS --- */

    // event GasPriceChanged(uint64_t gasPrice);
    // event RequiredBlockConfirmationChanged(uint64_t requiredBlockConfirmations);
    // event DailyLimit(address token, uint64_t newLimit);

    // /* --- MODIFIERs --- */

    void onlyValidator(name sender) {
        check(validatorContract().isValidator(sender), "Sender is not a validator");
    }

    void onlyOwner(name sender) {
        check(validatorContract().owner() == sender, "Sender is not owner");
    }

    /* --- FIELDS --- */

    // /* Beginning of V1 storage variables */
    // name  validatorContractAddress;
    // uint64_t  gasPrice; // Used by bridge client to determine proper gas price for corresponding chain
    // uint64_t  requiredBlockConfirmations; // Used by bridge client to determine proper number of blocks to wait before validating transfer
    // uint64_t  deployedAtBlock; // Used by bridge client to determine initial block number to start listening for transfers
    // mapping(address => uint64_t)  minPerTx;
    // mapping(address => uint64_t)  maxPerTx; // Set to 0 to disable
    // mapping(address => uint64_t)  dailyLimit; // Set to 0 to disable
    // mapping(address => mapping(uint64_t => uint64_t)) totalSpentPerDay;
    // /* End of V1 storage variables */

    /* --- EXTERNAL / PUBLIC  METHODS --- */
public:
    void setMaxPerTx(symbol token, uint64_t _maxPerTx)   {
        check(_maxPerTx < dailyLimit[token], "Error setting maxPerTx");
        maxPerTx[token] = _maxPerTx;
    }

    void setMinPerTx(symbol token, uint64_t _minPerTx){
        check(_minPerTx < dailyLimit[token] && _minPerTx < maxPerTx[token], "Error setting minPerTx");
        minPerTx[token] = _minPerTx;
    }

    void setGasPrice(uint64_t _gasPrice){
        check(_gasPrice > 0, "Error setting gasPrice");
        gasPrice = _gasPrice;
        // emit GasPriceChanged(_gasPrice);
    }

    void setRequiredBlockConfirmations(uint64_t _blockConfirmations) {
        check(_blockConfirmations > 0, "Error setting blockConfirmations");
        requiredBlockConfirmations = _blockConfirmations;
        // emit RequiredBlockConfirmationChanged(_blockConfirmations);
    }

    void setDailyLimit(symbol token, uint64_t _dailyLimit){
        dailyLimit[token] = _dailyLimit;
        // emit DailyLimit(token, _dailyLimit);
    }

    bool withinLimit(symbol token, uint64_t _amount)  {
        uint64_t nextLimit = totalSpentPerDay[token][getCurrentDay()].add(_amount);
        return dailyLimit[token] >= nextLimit && _amount <= maxPerTx[token] && _amount >= minPerTx[token];
    }

    uint64_t requiredSignatures()  {
        return validatorContract().requiredSignatures();
    }

    IBridgeValidators validatorContract()  {
        return IBridgeValidators(validatorContractAddress);
    }

    uint64_t getCurrentDay()  {
        return bos_oracle::current_time_point_sec().sec_since_epoch() / eosio::days(1).to_seconds();
    }
}
