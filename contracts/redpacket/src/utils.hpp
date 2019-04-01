#pragma once

#include <algorithm>
#include <eosiolib/eosio.hpp>
#include <eosiolib/crypto.hpp>
#include <string>
#include <vector>
#include "murmurhash.hpp"

// base58转换表
static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

// base58转换表
static const int8_t mapBase58[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1,
    -1, 9, 10, 11, 12, 13, 14, 15, 16, -1, 17, 18, 19, 20, 21, -1,
    22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, -1, -1, -1, -1,
    -1, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, -1, 44, 45, 46,
    47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

// base58解码
bool DecodeBase58(const char* psz, std::vector<unsigned char>& vch)
{
    // Skip leading spaces.
    while (*psz && isspace(*psz))
        psz++;
    // Skip and count leading '1's.
    int zeroes = 0;
    int length = 0;
    while (*psz == '1') {
        zeroes++;
        psz++;
    }
    // Allocate enough space in big-endian base256 representation.
    int size = strlen(psz) * 733 / 1000 + 1; // log(58) / log(256), rounded up.
    std::vector<unsigned char> b256(size);
    // Process the characters.
    static_assert(sizeof(mapBase58) / sizeof(mapBase58[0]) == 256, "mapBase58.size() should be 256"); // guarantee not out of range
    while (*psz && !isspace(*psz)) {
        // Decode base58 character
        int carry = mapBase58[(uint8_t)*psz];
        if (carry == -1) // Invalid b58 character
            return false;
        int i = 0;
        for (std::vector<unsigned char>::reverse_iterator it = b256.rbegin(); (carry != 0 || i < length) && (it != b256.rend()); ++it, ++i) {
            carry += 58 * (*it);
            *it = carry % 256;
            carry /= 256;
        }
        assert(carry == 0);
        length = i;
        psz++;
    }
    // Skip trailing spaces.
    while (isspace(*psz))
        psz++;
    if (*psz != 0)
        return false;
    // Skip leading zeroes in b256.
    std::vector<unsigned char>::iterator it = b256.begin() + (size - length);
    while (it != b256.end() && *it == 0)
        it++;
    // Copy result into output vector.
    vch.reserve(zeroes + (b256.end() - it));
    vch.assign(zeroes, 0x00);
    while (it != b256.end())
        vch.push_back(*(it++));
    return true;
}

bool decode_base58(const std::string& str, std::vector<unsigned char>& vch)
{
    return DecodeBase58(str.c_str(), vch);
}


eosio::public_key decode_pubkey(const std::string& public_key_str)
{
    std::string pubkey_prefix("EOS");
    auto result = mismatch(pubkey_prefix.begin(), pubkey_prefix.end(), public_key_str.begin());
    eosio_assert(result.first == pubkey_prefix.end(), "Public key should be prefix with EOS");
    auto base58substr = public_key_str.substr(pubkey_prefix.length());
    std::vector<unsigned char> vch;
    eosio_assert(decode_base58(base58substr, vch), "Decode pubkey failed");
    eosio_assert(vch.size() == 37, "Invalid public key");

    eosio::public_key pubkey;
    pubkey.type = 0;
    for (auto i = 0; i < 33; ++i)
        pubkey.data[i] = vch[i];

    capi_checksum160 checksum;
    ripemd160(reinterpret_cast<char *>(pubkey.data.data()), 33, &checksum);
    eosio_assert(memcmp(&checksum.hash, &vch.end()[-4], 4) == 0, "Invalid public key");

    return pubkey;
}

// 解析用户名
eosio::name decode_name(const std::string& name_str)
{
    auto len = name_str.size();
    for (size_t i = 0; i < len && i < 12; ++i) {
        if (!((name_str[i] >= 'a' && name_str[i] <= 'z') ||
                    (name_str[i] >= '1' && name_str[i] <= '5') ||
                    name_str[i] == '.')) {
            eosio_assert(false, "only a-z1-5. can be used in first 12 ch of name.");
        }
    }
    eosio_assert(len != 13 || (name_str.back() >= 'a' && name_str.back() <= 'p'), "only a-p. can be used in the 13th ch of name.");

    return eosio::name{name_str};
}

std::vector<std::string> split(std::string str, std::string delimeter)
{
    std::vector<std::string> vec;
    size_t begin = 0;
    size_t end = 0;
    while ((end = str.find(delimeter, begin)) != string::npos) {
        vec.push_back(str.substr(begin, end - begin));
        begin = end + delimeter.size();
    }
    vec.push_back(str.substr(begin));

    return vec;
}

uint32_t random(void* seed, size_t len)
{
    eosio::checksum256 checksum = eosio::sha256(static_cast<const char *>(seed), len);
    return murmur_hash2(reinterpret_cast<const char*>(checksum.data()), checksum.size());
}

bool has_suffix(std::string str, std::string suffix)
{
    if (str.length() >= suffix.length()) {
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    }
    return false;
}
