#pragma once

#include <eosio/eosio.hpp>
#include "bos.bridge/bos.config.hpp"
#include "bos.bridge/interfaces/IBridgeValidators.hpp"
#include "bos.bridge/interfaces/IBurnableMintableToken.hpp"

class HomeToken : public IBurnableMintableToken {

public:
  HomeToken(name _self,const std::string& _token):self(_self),token(_token){}
  void create()
  {
    asset maximum_supply=asset(10000000000000,symbol(token,4));
    action(permission_level{self, "active"_n}, "eosio.token"_n,"create"_n, std::make_tuple(self, maximum_supply)).send();
  }

  void mint(name _to, uint64_t _amount) 
  {
   action(permission_level{self, "active"_n}, "eosio.token"_n, "issue"_n, std::make_tuple(self, _to, asset(_amount,symbol(token,4)), "")).send();
  }

  void burn(name sender,uint64_t _value) {
   asset quantity= asset(_value,symbol(token,4));
   action(permission_level{sender, "active"_n}, "eosio.token"_n, "transfer"_n,
   std::make_tuple(sender, self, quantity, "")).send();
   action(permission_level{self, "active"_n}, "eosio.token"_n, "retire"_n,
   std::make_tuple(quantity, "")).send();

  }
private:
name self;
std::string token;
};
