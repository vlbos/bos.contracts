#pragma once
#include <eosio/eosio.hpp>

class IBridgeValidators {
public:
  virtual bool isValidator(name _validator) = 0;
  virtual bool isValidator(public_key _validator) = 0;
  virtual uint64_t requiredSignatures() = 0;
  virtual name owner() = 0;
};
