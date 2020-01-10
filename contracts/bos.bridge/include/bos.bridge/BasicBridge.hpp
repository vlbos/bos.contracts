#include <eosio/eosio.hpp>
#include "bos.bridge/bos.config.hpp"
#include "bos.bridge/interfaces/IBridgeValidators.hpp"

template <class TableType, typename Iterator>
class BasicBridge {
protected:
    TableType table;
    name self;
    void onlyValidator(name sender) {
        check(validatorContract().isValidator(sender), "Sender is not a validator");
    }

    void onlyOwner(name sender) {
        check(validatorContract().owner() == sender, "Sender is not owner");
    }


    /* --- EXTERNAL / PUBLIC  METHODS --- */
public:
    BasicBridge(name _self):self(_self),table(_self,_self.value)
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

    bool withinLimit(std::string token, uint64_t _amount)  {
        totalSpentPerDay[token][getCurrentDay()]+=_amount;
        uint64_t nextLimit = totalSpentPerDay[token][getCurrentDay()];
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
