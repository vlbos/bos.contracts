#pragma once
#include <eosiolib/eosio.hpp>
using std::vector;

typedef std::vector<char> bytes;


checksum256 get_hash(const string &task, const name &contract)
{
  checksum256 result;
  size_t tasklen = strlen(task.c_str());
  char *buffer = (char *)malloc(tasklen + 8);
  memcpy(buffer, &contract, 8);
  memcpy(buffer + 8, task.data(), tasklen);
  result = sha256(buffer, tasklen + 8);
  return result;
}

checksum256 get_full_hash(const string &task, const string &memo, const name &contract)
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
