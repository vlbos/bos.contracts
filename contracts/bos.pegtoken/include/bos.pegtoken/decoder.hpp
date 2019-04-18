#pragma once

#include "sha3.h"
#include <algorithm>
#include <cstdio>
#include <eosiolib/crypto.h>
#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.h>
#include <string>
#include <vector>

namespace eosio {

using std::string;

constexpr char hex_map[] = "0123456789abcdef";

inline std::vector<string> split_string(string str, string delimiter)
{
    std::vector<string> res;
    if (str.size() == 0) {
        return res;
    }

    size_t pos = 0;
    while ((pos = str.find(delimiter)) != std::string::npos) {
        auto tmp = str.substr(0, pos);
        if (tmp.size() > 0) {
            res.push_back(tmp);
        }
        str.erase(0, pos + delimiter.length());
    }
    if (str.size() > 0) {
        res.push_back(str);
    }
    return res;
}

inline uint8_t hex_to_digit(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'a' && ch <= 'z')
        return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'Z')
        return ch - 'A' + 10;
    eosio_assert(false, "invalid hex type");
    return 0xff;
}

string decode_hex(const void* data, uint64_t len)
{
    auto src = static_cast<const uint8_t*>(data);
    string str(len * 2, 0);
    for (int i = 0; i < len; ++i) {
        str[2 * i] = hex_map[(src[i] & 0xf0) >> 4];
        str[2 * i + 1] = hex_map[src[i] & 0x0f];
    }
    return str;
}

bool unbase58(const char* s, unsigned char* out)
{
    static const char* tmpl = "123456789"
                              "ABCDEFGHJKLMNPQRSTUVWXYZ"
                              "abcdefghijkmnopqrstuvwxyz";
    int i, j, c;
    const char* p;

    memset(out, 0, 25);
    for (i = 0; s[i]; i++) {
        if (!(p = std::strchr(tmpl, s[i])))
            return false;

        c = p - tmpl;
        for (j = 25; j--;) {
            c += 58 * out[j];
            out[j] = c % 256;
            c /= 256;
        }

        // address too long
        if (c)
            return false;
    }

    return true;
}

// see http://rosettacode.org/wiki/Bitcoin/address_validation#C
bool valid_bitcoin_addr(string addr)
{
    if (addr.size() < 26 || addr.size() > 35)
        return false;

#ifdef TESTNET
    if (addr[0] != '2' && addr[0] != '9' && addr[0] != 'm' && addr[0] != 'n')
        return false;
#else
    string bc1("bc1");
    if (addr[0] != '1' && addr[0] != '3'
        && std::mismatch(bc1.begin(), bc1.end(), addr.begin()).first != bc1.end())
        return false;
#endif

    unsigned char dec[32];
    if (!unbase58(addr.c_str(), dec))
        return false;

    capi_checksum256 tmp, res;
    sha256(reinterpret_cast<const char*>(dec), 21, &tmp);
    sha256(reinterpret_cast<const char*>(tmp.hash), 32, &res);
    return std::memcmp(dec + 21, res.hash, 4) ? false : true;
}

bool valid_ethereum_addr(string addr)
{
    auto prefix_index = addr.find("0x");
    if (prefix_index == 0)
        addr = addr.substr(prefix_index + 2);

    eosio_assert(addr.size() == 40, "invalid eth adr len, expected: 40");

    for (auto&& r : addr) {
        if (!(r >= '0' && r <= '9')
            && !(r >= 'a' && r <= 'z')
            && !(r >= 'A' && r <= 'Z')) {
            return false;
        }
    }
    return true;
}

bool valid_ethereum_addr_strict(string addr)
{
    auto prefix_index = addr.find("0x");
    if (prefix_index == 0)
        addr = addr.substr(prefix_index + 2);

    eosio_assert(addr.size() == 40, "invalid eth adr len, expected: 40");

    auto origin_addr = addr;

    std::transform(addr.begin(), addr.end(), addr.begin(), ::tolower);

    uint8_t buf[addr.length()];
    int i = 0;
    for (auto& r : addr)
        buf[i++] = r;
    uint8_t out[32];

    keccak_256(out, sizeof(out), buf, addr.length());

    auto mask = decode_hex(out, sizeof(out));
    for (int i = 0; i < addr.size(); ++i) {
        addr[i] = (hex_to_digit(mask[i]) > 7) ? ::toupper(addr[i]) : ::tolower(addr[i]);
    }

    return addr == origin_addr;
}

bool valid_usdt_addr(string addr)
{
    auto addrs_pairs = split_string(addr, "|");

    switch (addrs_pairs.size()) {
    case 0: {
        return false;
    } break;
    case 1: {
        auto splitted_addr = split_string(addrs_pairs[0], ":");
        if (splitted_addr.size() != 2) {
            return false;
        }
        if (splitted_addr[0] == "BTC") {
            return valid_bitcoin_addr(splitted_addr[1]);
        } else if (splitted_addr[0] == "ETH") {
            return valid_ethereum_addr(splitted_addr[1]);
        } else {
            return false;
        }
    } break;
    case 2: {
        bool btc_checked = false, eth_checked = false;
        for (auto&& s : addrs_pairs) {
            auto splitted_addr = split_string(s, ":");
            if (splitted_addr.size() != 2) {
                return false;
            }
            if (splitted_addr[0] == "BTC") {
                if (btc_checked || !valid_bitcoin_addr(splitted_addr[1])) {
                    return false;
                } else {
                    btc_checked = true;
                }
            } else if (splitted_addr[0] == "ETH") {
                if (eth_checked || !valid_ethereum_addr(splitted_addr[1])) {
                    return false;
                } else {
                    eth_checked = true;
                }
            } else {
                return false;
            }
        }
        return true;
    } break;
    default:
        return false;
    }
    return false;
}

capi_checksum256 get_trx_id()
{
    capi_checksum256 trx_id;
    std::vector<char> trx_bytes;
    size_t trx_size = transaction_size();
    trx_bytes.resize(trx_size);
    read_transaction(trx_bytes.data(), trx_size);
    sha256(trx_bytes.data(), trx_size, &trx_id);
    return trx_id;
}

string checksum256_to_string(capi_checksum256 src)
{
    return decode_hex(src.hash, 32);
}

uint64_t hash64(const string s)
{
    capi_checksum256 hash256;
    uint64_t res = 0;
    sha256(s.c_str(), s.length(), &hash256);
    std::memcpy(&res, hash256.hash, 8);
    return res;
}

} // namespace eosio