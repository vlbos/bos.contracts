/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/eosio.hpp>

#include <string>

namespace eosio {

   using std::string;

   class [[eosio::contract("personal.bos")]] personal_contract : public contract {
      public:
         using contract::contract;

         [[eosio::action]]
         void setpersonal( name account, name key, string value);

         [[eosio::action]]
         void sethomepage( name account, string url);

         string get_personal( name account, name key ){
            personal_table personal( _self , account.value );
            auto ite = personal.find( key.value );
            if(ite != personal.end()){
               return ite->value;
            }else{
               return "";
            }
         }

      private:
		 /*For accounts to store their personal specific key-value data.
		 key needs to be string and follow rules: only contain a-z,0-5,not longer than 12.
		 for the key shorter than 12,will add 0 at higher bits.
		 by doing this we can use key as uint64_t,to save space and easy use.
		 usage: ex. dapp owner can call sethomepage to set its homepage,guiding users to their dapp*/
		 struct [[eosio::table]] personal{
			name key;
			string value;
			EOSLIB_SERIALIZE(personal,(key)(value))
				
			uint64_t primary_key()const { return key.value; }
		 };
		 typedef eosio::multi_index< "personaldata"_n, personal > personal_table;
   };

} /// namespace eosio
