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

    void setTokenParameter(name sender,std::string _token, uint64_t _dailyLimit,
                        uint64_t _maxPerTx, uint64_t _minPerTx) {
    require_auth(sender);
    check(_minPerTx > 0 && _maxPerTx > _minPerTx && _dailyLimit > _maxPerTx,
          "Tx limits initialization error in set token");

    para.dailyLimit[_token] = _dailyLimit;
    para.maxPerTx[_token] = _maxPerTx;
    para.minPerTx[_token] = _minPerTx;

  }

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
        std::string checkmsg = "no token:"+ token;
        auto itlimit = para.dailyLimit.find(token);
        if( itlimit== para.dailyLimit.end())
        {
            print(checkmsg,"in dailyLimit");
            return false;
        } 
        
        auto itmax = para.maxPerTx.find(token);
        if(para.maxPerTx.end() == itmax){
             print(checkmsg,"in maxPerTx");
            return false;
        }
         
         auto itmin = para.minPerTx.find(token);
         if(para.minPerTx.end() == itmin){
             print(checkmsg,"in minPerTx");
            return false;
         }
        
        checksum256 next = get_checksum256(token,getCurrentDay());
        uint64_t nextLimit = 0;

        auto itTotal =  para.totalSpentPerDay.find(next);
        if(para.totalSpentPerDay.end()!=itTotal)
        {
            nextLimit = itTotal->second;
        }

        nextLimit+=_amount;
        para.totalSpentPerDay[next]=nextLimit;
        
        return itlimit->second >= nextLimit && _amount <= itmax->second && _amount >= itmin->second;
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
