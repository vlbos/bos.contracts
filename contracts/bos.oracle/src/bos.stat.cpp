#pragma once

#include "bos.oracle/bos.oracle.hpp"
#include <cmath>
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
using namespace eosio;
using std::string;

// } // namespace bosoracle

  time_point_sec bos_oracle::current_time_point_sec() {
      const static time_point_sec cts{ current_time_point() };
      return cts;
   }