#pragma once
#include <eosiolib/eosio.hpp>
#include <eosiolib/crypto.h>
#include "murmurhash.hpp"


checksum256 get_uu_hash(const uint64_t& service_id, const uint8_t& fee_type)
{
  checksum256 result;
  size_t service_id_len =  sizeof(uint64_t);
  size_t fee_type_len =  sizeof(uint8_t);
  char *buffer = (char *)malloc( service_id_len+fee_type_len);
  memcpy(buffer, &service_id, service_id_len);
  memcpy(buffer + service_id_len, &fee_type, service_id_len);
  result = sha256(buffer, service_id_len + fee_type_len);
  return result;
}

checksum256 get_uuu_hash(const uint64_t& service_id, const name& contract_name, const name& action_name)
{
  checksum256 result;
  size_t len =  sizeof(uint64_t);
  
  char *buffer = (char *)malloc( len*3);
  memcpy(buffer, &service_id, len);
  memcpy(buffer + len, &contract_name, len);
  memcpy(buffer + len*2, &action_name, len);
  result = sha256(buffer, len*3);
  return result;
}

checksum256 get_nnn_hash(const name& account, const name& contract_name, const name& action_name)
{
  checksum256 result;
  size_t len =  sizeof(uint64_t);
  size_t total_len = len*3;
  char *buffer = (char *)malloc( total_len);
  memcpy(buffer, &account, len);
  memcpy(buffer + len, &contract_name, len);
  memcpy(buffer + len*2, &action_name, len);
  result = sha256(buffer, total_len);
  return result;
}


checksum256 get_nn_hash( const name& contract_name, const name& action_name)
{
  checksum256 result;
  size_t len =  sizeof(uint64_t);
  size_t total_len = len*2;
  char *buffer = (char *)malloc( total_len);
  memcpy(buffer , &contract_name, len);
  memcpy(buffer + len, &action_name, len);
  result = sha256(buffer, total_len);
  return result;
}

checksum256 get_sn_hash(const string& task, const name& contract)
{
  checksum256 result;
  size_t tasklen = strlen(task.c_str());
  char *buffer = (char *)malloc(tasklen + 8);
  memcpy(buffer, &contract, 8);
  memcpy(buffer + 8, task.data(), tasklen);
  result = sha256(buffer, tasklen + 8);
  return result;
}

checksum256 get_ssn_hash(const string &task, const string &memo, const name &contract)
{
  checksum256 result;
  size_t tasklen = strlen(task.c_str());
  size_t memolen = strlen(memo.c_str());
  char *buffer = (char *)malloc(tasklen + memolen + 8);
  memcpy(buffer, &contract, 8);
  memcpy(buffer + 8, task.data(), tasklen);
  memcpy(buffer + tasklen + 8, memo.data(), memolen);
  result = sha256(buffer, tasklen + 8);
  return result;
}

uint64_t get_hash_key(checksum256 hash)
{
  const uint64_t *p64 = reinterpret_cast<const uint64_t *>(&hash);
  return p64[0] ^ p64[1] ^ p64[2] ^ p64[3];
}
uint32_t random(void* seed, size_t len)
{
    checksum256 rand256;
    rand256 = sha256(static_cast<const char*>(seed), len);
    auto res = murmur_hash2(reinterpret_cast<const char*>(&rand256), sizeof(rand256) / sizeof(char));
    return res;
}
