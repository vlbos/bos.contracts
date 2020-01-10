#pragma once

#include <eosio/eosio.hpp>
#include "bos.bridge/bos.config.hpp"
#include "bos.bridge/interfaces/IBridgeValidators.hpp"
#include "bos.bridge/interfaces/IBurnableMintableToken.hpp"

class HomeToken : public IBurnableMintableToken {

  bool mint(name _to, uint64_t _amount) {}
  void burn(uint64_t _value) {}
}
