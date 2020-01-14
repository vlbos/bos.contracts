#pragma once
#include <eosio/eosio.hpp>
#include "bos.bridge/bos.config.hpp"
#include "bos.bridge/interfaces/IBridgeValidators.hpp"

template <class TableType> 
class BridgeValidators : public IBridgeValidators {
protected:
  TableType &table;
  name self;

public:
  BridgeValidators(name _self, TableType &_table)
      : self(_self), table(_table){}

void initialize(uint64_t _requiredSignatures,
                                         std::vector<name> & _initialValidators,
                                         name _owner) {
    require_auth(_owner);
    setOwner(_owner);
    check(_requiredSignatures != 0,
          "RequiredSignatures should be greater than 0");
    check(_initialValidators.size() >= _requiredSignatures,
          "Number of proposed validators should be greater than "
          "requiredSignatures");
    for (uint64_t i = 0; i < _initialValidators.size(); i++) {
      check(is_account(_initialValidators[i]),
            "Validator address should not be name()");
      check(!isValidator(_initialValidators[i]),"is validator false");
      table.validatorCount++;
      table.validators[_initialValidators[i]] = true;
      // emit ValidatorAdded(_initialValidators[i]);
    }
    check(table.validatorCount >= _requiredSignatures,
          "Number of confirmed validators should be greater than "
          "requiredSignatures");
    table.requiredSignatures = _requiredSignatures;
  }

  /* --- EXTERNAL / PUBLIC  METHODS --- */
  void addValidator(name _validator) {
    check(is_account(_validator), "Validator address should not be name()");
    check(!isValidator(_validator),
          "New validator should be an existing validator");
    table.validatorCount++;
    table.validators[_validator] = true;
    // emit ValidatorAdded(_validator);
  }

  name removeValidator(name _validator) {
    check(table.validatorCount > table.requiredSignatures,
          "Removing validator should not make validator count be < "
          "requiredSignatures");
    check(isValidator(_validator),
          "Cannot remove address that is not a validator");
    table.validators[_validator] = false;
    table.validatorCount--;
    // emit ValidatorRemoved(_validator);
  }

  void setRequiredSignatures(uint64_t _requiredSignatures) {
    check(table.validatorCount >= _requiredSignatures,
          "New requiredSignatures should be greater than num of validators");
    check(_requiredSignatures != 0,
          "New requiredSignatures should be > than 0");
    table.requiredSignatures = _requiredSignatures;
    // emit RequiredSignaturesChanged(_requiredSignatures);
  }

  bool isValidator(name _validator) {
    return table.validators.find(_validator)!= table.validators.end() && table.validators[_validator];
  }

 bool isValidator(public_key _validator) {
    return true;//table.validators.find(_validator) && table.validators[_validator];
  }


  /* --- INTERNAL / PRIVATE METHODS --- */

  void setOwner(name _owner) {
    check(is_account(_owner), "New owner cannot be name()");
    table.owner = _owner;
  }

   uint64_t requiredSignatures() {
return table.requiredSignatures;
   }

   name owner() {
       return table.owner;
   }

};
