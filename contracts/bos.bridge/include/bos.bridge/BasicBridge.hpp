#pragma once

#include <eosio/eosio.hpp>
#include "bos.bridge/bos.config.hpp"
#include "bos.bridge/bos.bridge.hpp"
#include "bos.bridge/interfaces/IBridgeValidators.hpp"
#include "bos.bridge/BridgeValidators.hpp"

template <class TableType,class ParaType>
class BasicBridge {
protected:
    TableType& table;
    ParaType& para;
    name self;
    BridgeValidators<TableType> _BridgeValidators;
    void onlyValidator(name sender) {
        check(validatorContract().isValidator(sender), "Sender is not a validator");
    }

    void onlyOwner(name sender) {
        check(validatorContract().owner() == sender, "Sender is not owner");
    }


    /* --- EXTERNAL / PUBLIC  METHODS --- */
public:
    BasicBridge(name _self,TableType& _table,ParaType& _para):self(_self),table(_table),para(_para),_BridgeValidators(_self,_table){}
    void setMaxPerTx(const std::string& token, uint64_t _maxPerTx)   {
        check(_maxPerTx < para.dailyLimit[token], "Error setting maxPerTx");
        para.maxPerTx[token] = _maxPerTx;
    }

    void setMinPerTx(const std::string& token, uint64_t _minPerTx){
        check(_minPerTx < para.dailyLimit[token] && _minPerTx < para.maxPerTx[token], "Error setting minPerTx");
        para.minPerTx[token] = _minPerTx;
    }

    void setGasPrice(uint64_t _gasPrice){
        check(_gasPrice > 0, "Error setting gasPrice");
        para.gasPrice = _gasPrice;
        // emit GasPriceChanged(_gasPrice);
    }

    void setRequiredBlockConfirmations(uint64_t _blockConfirmations) {
        check(_blockConfirmations > 0, "Error setting blockConfirmations");
        para.requiredBlockConfirmations = _blockConfirmations;
        // emit RequiredBlockConfirmationChanged(_blockConfirmations);
    }

    void setDailyLimit(const std::string& token, uint64_t _dailyLimit){
        para.dailyLimit[token] = _dailyLimit;
        // emit DailyLimit(token, _dailyLimit);
    }

    bool withinLimit(const std::string& token, uint64_t _amount)  {
        para.totalSpentPerDay[token][getCurrentDay()]+=_amount;
        uint64_t nextLimit = para.totalSpentPerDay[token][getCurrentDay()];
        return para.dailyLimit[token] >= nextLimit && _amount <= para.maxPerTx[token] && _amount >= para.minPerTx[token];
    }

    uint64_t requiredSignatures()  {
        return validatorContract()->requiredSignatures();
    }

   IBridgeValidators* validatorContract()  {
        return &_BridgeValidators;
    }

    uint64_t getCurrentDay()  {
        return bos_bridge::current_time_point_sec().sec_since_epoch() / eosio::days(1).to_seconds();
    }
};
