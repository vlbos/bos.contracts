/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <personal.bos/personal.bos.hpp>

namespace eosio {
static void check_url( string url ){
   eosio_assert( url.size() <= 256, "url is too long");
   eosio_assert( url.find("http") == 0, "illegal url");
}

void personal_contract::sethomepage( name account , string url ){
   check_url( url );
   setpersonal( account , "homepage"_n , url );
}

void personal_contract::setpersonal( name account , name key , string value ){
   require_auth( account );
   if(key.value == "homepage"_n.value){
      check_url( value );
   }
   //not support too long value, for safety.
   eosio_assert( value.size() <= 1024 , "value should be less than 1024" );
   personal_table personal( _self , account.value );
   auto ite = personal.find( key.value );
   if( ite != personal.end() ){
	  personal.modify( ite , account , [&](auto& data){
		 data.value = value;
	  });
   }else{
	  personal.emplace( account , [&](auto& data){
		 data.key = key;
		 data.value = value;
	  });
   }
}
} 

EOSIO_DISPATCH( eosio::personal_contract, (sethomepage)(setpersonal) )
