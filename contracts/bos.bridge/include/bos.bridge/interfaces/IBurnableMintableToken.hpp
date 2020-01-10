#pragma once
#include <eosio/eosio.hpp>
#include "bos.bridge/bos.config.hpp"
class IBurnableMintableToken {
    public:
    virtual bool mint(name _to, uint64_t _amount) = 0 ;
    virtual void burn(uint64_t _value)  = 0;
    IBurnableMintableToken(std::string _token){token = _token; }

protected:
  std::string token;
};