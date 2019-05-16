#pragma once
#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include "bos.oracle/bos.constants.hpp"
#include "bos.oracle/bos.functions.hpp"
#include "bos.oracle/bos.types.hpp"

using namespace eosio;
// using std::string;


   struct [[eosio::table("fee"), eosio::contract("bos.oracle")]] bos_oracle_fee {
      bos_oracle_fee(){}

      uint64_t          total_fee = 0;
    

      EOSLIB_SERIALIZE( bos_oracle_fee, (total_fee))
   };

typedef eosio::singleton< "fee"_n, bos_oracle_fee > oracle_fee_singleton;

// //   ///bos.oraclize end
// };

// } // namespace bosoracle