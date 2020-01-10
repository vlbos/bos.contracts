#include <eosio/eosio.hpp>

class IBridgeValidators {
public:
  virtual bool isValidator(name _validator) = 0;
  virtual uint64_t requiredSignatures() = 0;
  virtual name owner() = 0;
  IBridgeValidators(name _owner){owner = _owner; }

protected:
  name owner;
};
