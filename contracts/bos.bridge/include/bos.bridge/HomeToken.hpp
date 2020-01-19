#pragma once

#include <eosio/eosio.hpp>
#include "bos.bridge/bos.config.hpp"
#include "bos.bridge/interfaces/IBridgeValidators.hpp"
#include "bos.bridge/interfaces/IBurnableMintableToken.hpp"
#include "bos.bridge/bos.bridge.hpp"

class HomeToken : public IBurnableMintableToken {

public:
  HomeToken(name _self,const std::string& _token):self(_self),token(bos_bridge::str2sym(_token),bos_bridge::str2contract(_token)){}
  void create()
  {
    // check(self==token.get_contract(),"token  contract is not home account");
    asset maximum_supply=asset(10000000000000,token.get_symbol());
    action(permission_level{self, "active"_n}, token.get_contract(),"create"_n, std::make_tuple(self, maximum_supply)).send();
  }

  void mint(name _to, uint64_t _amount) 
  {
    std::string memo = "";
   action(permission_level{self, "active"_n}, token.get_contract(), "issue"_n, std::make_tuple(self, _to, asset(_amount,token.get_symbol()), memo)).send();
  }

  void burn(name sender,uint64_t _value) {
   asset quantity= asset(_value,token.get_symbol());
   std::string memo = "";
   action(permission_level{sender, "active"_n}, token.get_contract(), "transfer"_n,
   std::make_tuple(sender, self, quantity, memo)).send();
   action(permission_level{self, "active"_n}, token.get_contract(), "retire"_n,
   std::make_tuple(quantity, memo)).send();
  }
private:
name self;
eosio::extended_symbol token;
};
