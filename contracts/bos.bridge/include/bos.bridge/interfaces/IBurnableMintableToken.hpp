#pragma once
#include <eosio/eosio.hpp>
#include "bos.bridge/bos.config.hpp"
class IBurnableMintableToken {
    public:
    virtual void mint(name _to, uint64_t _amount) = 0 ;
    virtual void burn(name sender,uint64_t _value)  = 0;
};