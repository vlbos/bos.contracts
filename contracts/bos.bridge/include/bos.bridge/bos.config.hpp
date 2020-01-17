#pragma once
#include <eosio/eosio.hpp>
#include <eosio/crypto.hpp>
#include <eosio/asset.hpp>
#include <eosio/symbol.hpp>
#include <eosio/system.hpp>
#include <cmath>
using namespace eosio;

using bytes = std::vector<char>;


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

std::string hex_to_string(const std::string& input) {
  static const char* const lut = "0123456789abcdef";
  size_t len = input.length();
  if (len & 1) abort();
  std::string output;
  output.reserve(len / 2);
  for (size_t i = 0; i < len; i += 2) {
    char a = input[i];
    const char* p = std::lower_bound(lut, lut + 16, a);
    if (*p != a) abort();
    char b = input[i + 1];
    const char* q = std::lower_bound(lut, lut + 16, b);
    if (*q != b) abort();
    output.push_back(((p - lut) << 4) | (q - lut));
  }
  return output;
}

const char * const ALPHABET =
    "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
const char ALPHABET_MAP[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1,
    -1,  9, 10, 11, 12, 13, 14, 15, 16, -1, 17, 18, 19, 20, 21, -1,
    22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, -1, -1, -1, -1,
    -1, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, -1, 44, 45, 46,
    47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, -1, -1, -1, -1, -1
};

int base58encode(const std::string input, int len, unsigned char result[]) {
    unsigned char const* bytes = (unsigned const char*)(input.c_str());
    unsigned char digits[len * 137 / 100];
    int digitslen = 1;
    for (int i = 0; i < len; i++) {
        unsigned int carry = (unsigned int) bytes[i];
        for (int j = 0; j < digitslen; j++) {
            carry += (unsigned int) (digits[j]) << 8;
            digits[j] = (unsigned char) (carry % 58);
            carry /= 58;
        }
        while (carry > 0) {
            digits[digitslen++] = (unsigned char) (carry % 58);
            carry /= 58;
        }
    }
    int resultlen = 0;
    // leading zero bytes
    for (; resultlen < len && bytes[resultlen] == 0;)
        result[resultlen++] = '1';
    // reverse
    for (int i = 0; i < digitslen; i++)
        result[resultlen + i] = ALPHABET[digits[digitslen - 1 - i]];
    result[digitslen + resultlen] = 0;
    return digitslen + resultlen;
}

class ec{
  public:
      static void ecrecover(std::string data, const eosio::signature &sig)
      {
        std::string tmp;
        eosio::checksum256 digest= sha256(&data[0], data.size());

        char pub[34]; // public key without checksum
        public_key n = recover_key(digest, sig);

        std::string pubhex = to_hex(pub, sizeof(pub)).substr(2); // remove leading '00'
        tmp = hex_to_string(pubhex.c_str());
        strcpy(pub, tmp.c_str());

        checksum160 chksm=ripemd160(pub, 33);

        tmp = hex_to_string(pubhex + to_hex(&chksm, 20).substr(0,8)); // append checksum

        unsigned char encoded[37  * 137 / 100];
        base58encode(tmp, 37, encoded);
        tmp = "EOS" + std::string(reinterpret_cast<char*>(encoded));
        assert(tmp.length() == 53);
        print(tmp);
      }

      ///@abi action
      static void ecverify(std::string data, const signature &sig, const public_key &pk)
      {
        checksum256 digest= sha256(&data[0], data.size());

        assert_recover_key(digest, sig,  pk);
        print("VALID");
      }

};



std::string key2str(public_key pk)
      {
        std::string tmp(pk.data.data());
       
        // char pub[33]; // public key without checksum
        // memcpy(pub, pk.data.data(),pk.data.size());
        // std::string pubhex = to_hex(pub, sizeof(pub)).substr(2); // remove leading '00'
        // tmp = hex_to_string(pubhex.c_str());
        // strcpy(pub, tmp.c_str());

        // checksum160 chksm=ripemd160(pub, 33);

        // tmp = hex_to_string(pubhex + to_hex(&chksm, 20).substr(0,8)); // append checksum

        // unsigned char encoded[37  * 137 / 100];
        // base58encode(tmp, 37, encoded);
        // tmp = "EOS" + std::string(reinterpret_cast<char*>(encoded));
        // assert(tmp.length() == 53);
        return tmp;
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