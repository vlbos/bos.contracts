#pragma once
#include <eosio/eosio.hpp>
#include <eosio/types.h>
#include <eosio/crypto.h>
#include <eosio/asset.hpp>
#include <eosio/symbol.hpp>
#include <cmath>

using bytes = vector<uint8_t>;


static const std::string default_core_symbol =  "BOS";
static const uint8_t default_precision =  4;
static const std::string chain_token =  "eth";
