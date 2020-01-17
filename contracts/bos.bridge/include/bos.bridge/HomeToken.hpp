#pragma once

#include <eosio/eosio.hpp>
#include "bos.bridge/bos.config.hpp"
#include "bos.bridge/interfaces/IBridgeValidators.hpp"
#include "bos.bridge/interfaces/IBurnableMintableToken.hpp"

class HomeToken : public IBurnableMintableToken {

public:
  HomeToken(name _self,const eosio::extended_symbol& _token):self(_self),token(_token){}
  void create()
  {
    check(self==token.contract,"token  contract is not home account");
    asset maximum_supply=asset(10000000000000,token.sym);
    action(permission_level{self, "active"_n}, token.contract,"create"_n, std::make_tuple(self, maximum_supply)).send();
  }

  void mint(name _to, uint64_t _amount) 
  {
    std::string memo = "";
   action(permission_level{self, "active"_n}, token.contract, "issue"_n, std::make_tuple(self, _to, asset(_amount,token.sym), memo)).send();
  }

  void burn(name sender,uint64_t _value) {
   asset quantity= asset(_value,token.sym);
   std::string memo = "";
   action(permission_level{sender, "active"_n}, token.contract, "transfer"_n,
   std::make_tuple(sender, self, quantity, memo)).send();
   action(permission_level{self, "active"_n}, token.contract, "retire"_n,
   std::make_tuple(quantity, memo)).send();
  }
private:
name self;
eosio::extended_symbol token;
};
