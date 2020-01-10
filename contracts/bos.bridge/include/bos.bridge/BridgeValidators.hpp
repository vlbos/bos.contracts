#include <eosio/eosio.hpp>
#include "bos.bridge/bos.config.hpp"
#include "bos.bridge/interfaces/IBridgeValidators.hpp"

template <class TableType, typename Iterator>
class BridgeValidators :public IBridgeValidators {
    protected:
    TableType table;
    name self;
 public:
 BridgeValidators(name _self):self(_self),table(_self,_self.value)

   void initialize(uint64_t _requiredSignatures, std::vector<name>& _initialValidators, name _owner)
    {
        setOwner(_owner);
        check(_requiredSignatures != 0, "RequiredSignatures should be greater than 0");
        check(_initialValidators.length >= _requiredSignatures, "Number of proposed validators should be greater than requiredSignatures");
        for (uint64_t i = 0; i < _initialValidators.length; i++) {
            check(_initialValidators[i] != name(), "Validator address should not be 0x0");
            check(!isValidator(_initialValidators[i]));
            validatorCount = validatorCount+(1);
            validators[_initialValidators[i]] = true;
            // emit ValidatorAdded(_initialValidators[i]);
        }
        check(validatorCount >= _requiredSignatures, "Number of confirmed validators should be greater than requiredSignatures");
        requiredSignatures = _requiredSignatures;
    }

    /* --- EXTERNAL / PUBLIC  METHODS --- */
    void addValidator(name _validator) {
        check(_validator != address(0), "Validator address should not be 0x0");
        check(!isValidator(_validator), "New validator should be an existing validator");
        validatorCount = validatorCount+(1);
        validators[_validator] = true;
        // emit ValidatorAdded(_validator);
    }

    name removeValidator(name _validator) {
        check(validatorCount > requiredSignatures, "Removing validator should not make validator count be < requiredSignatures");
        check(isValidator(_validator), "Cannot remove address that is not a validator");
        validators[_validator] = false;
        validatorCount = validatorCount.sub(1);
        // emit ValidatorRemoved(_validator);
    }

    void setRequiredSignatures(uint64_t _requiredSignatures)  {
        check(validatorCount >= _requiredSignatures, "New requiredSignatures should be greater than num of validators");
        check(_requiredSignatures != 0, "New requiredSignatures should be > than 0");
        requiredSignatures = _requiredSignatures;
        // emit RequiredSignaturesChanged(_requiredSignatures);
    }

    bool isValidator(name _validator)  {
        return validators.find(_validator) && validators[_validator];
    }

    /* --- INTERNAL / PRIVATE METHODS --- */

    void setOwner(name _owner)  {
        check(_owner != name(), "New owner cannot be 0x0");
        owner = _owner;
    }
};
