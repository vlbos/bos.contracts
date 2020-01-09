#include <eosio/eosio.hpp>

class IBurnableMintableToken {
    public:
    bool mint(name _to, uint64_t _amount) ;
    void burn(uint64_t _value) ;
};