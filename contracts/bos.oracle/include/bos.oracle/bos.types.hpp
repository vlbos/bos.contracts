


/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <string>
#include <eosio/varint.hpp>
#include <eosio/privileged.hpp>
#include <eosio/eosio.hpp>
using std::vector;
typedef std::vector<char> bytes;

namespace eosio {

   using std::string;

   typedef checksum256   digest_type;
   typedef checksum256   block_id_type;
   typedef checksum256   chain_id_type;
   typedef checksum256   transaction_id_type;
   typedef signature     signature_type;

   template<typename T>
   void push(T&){}

   template<typename Stream, typename T, typename ... Types>
   void push(Stream &s, T arg, Types ... args){
      s << arg;
      push(s, args...);
   }

   template<class ... Types> checksum256 get_checksum256(const Types & ... args ){
      datastream <size_t> ps;
      push(ps, args...);
      size_t size = ps.tellp();

      std::vector<char> result;
      result.resize(size);

      datastream<char *> ds(result.data(), result.size());
      push(ds, args...);
      return sha256(result.data(), result.size());
   }

   inline bool is_equal_capi_checksum256( checksum256 a, checksum256 b ){
      return std::memcmp( (void*)&a, (void*)&b, 32 ) == 0;
   }
}
