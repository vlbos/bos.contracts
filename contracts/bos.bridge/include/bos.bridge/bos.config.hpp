#pragma once
#include <eosio/eosio.hpp>
#include <eosio/crypto.hpp>
#include <eosio/asset.hpp>
#include <eosio/symbol.hpp>
#include <eosio/system.hpp>
#include <cmath>

using bytes = std::vector<uint8_t>;


static const std::string default_core_symbol =  "BOS";
static const uint8_t default_precision =  4;
static const std::string chain_token =  "eth";
static const std::string address_zero =  "0";

static const uint8_t current_bridge_version = 1;

template<typename CharT>
static std::string to_hex(const CharT* d, uint32_t s) {
  std::string r;
  const char* to_hex="0123456789abcdef";
  uint8_t* c = (uint8_t*)d;
  for( uint32_t i = 0; i < s; ++i ) {
    (r += to_hex[(c[i] >> 4)]) += to_hex[(c[i] & 0x0f)];
  }
  return r;
}

bool operator<(const eosio::checksum256& lhs, const eosio::checksum256& rhs)
{
    return to_hex(&lhs, sizeof(lhs)) < to_hex(&rhs, sizeof(rhs));
}


std::vector<std::string>
split(const std::string& str, const std::string& delims = ":")
{
    std::vector<std::string> output;
    auto first = std::cbegin(str);
    while (first != std::cend(str))
    {
        const auto second = std::find_first_of(first, std::cend(str), 
                  std::cbegin(delims), std::cend(delims));
        if (first != second)
            output.emplace_back(first, second);
        if (second == std::cend(str))
            break;
        first = std::next(second);
    }
    return output;
}

// std::vector<std::string_view>
// splitSV(std::string_view strv, std::string_view delims = " ")
// {
//     std::vector<std::string_view> output;
//     size_t first = 0;
//     while (first < strv.size())
//     {
//         const auto second = strv.find_first_of(delims, first);
//         if (first != second)
//             output.emplace_back(strv.substr(first, second-first));
//         if (second == std::string_view::npos)
//             break;
//         first = second + 1;
//     }
//     return output;
// }


// template < class ContainerT >
// void tokenize(const std::string& str, ContainerT& tokens,
//               const std::string& delimiters = " ", bool trimEmpty = false)
// {
//    std::string::size_type pos, lastPos = 0, length = str.length();

//    using value_type = typename ContainerT::value_type;
//    using size_type  = typename ContainerT::size_type;

//    while(lastPos < length + 1)
//    {
//       pos = str.find_first_of(delimiters, lastPos);
//       if(pos == std::string::npos)
//       {
//          pos = length;
//       }

//       if(pos != lastPos || !trimEmpty)
//          tokens.push_back(value_type(str.data()+lastPos,
//                (size_type)pos-lastPos ));

//       lastPos = pos + 1;
//    }
// }

// std::string data = "hello";
// checksum256 digest;
// sha256(&data[0], data.size(), &digest);

// std::string hexstr = to_hex(&digest, sizeof(digest));