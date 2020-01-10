#include <eosio/eosio.hpp>

class IBurnableMintableToken {
    public:
    virtual bool mint(name _to, uint64_t _amount) = 0 ;
    virtual void burn(uint64_t _value)  = 0;
};