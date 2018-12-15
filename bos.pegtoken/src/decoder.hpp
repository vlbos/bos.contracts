#include "sha3.h"
#include <algorithm>
#include <cstdio>
#include <eosiolib/crypto.h>
#include <eosiolib/eosio.hpp>
#include <string>

constexpr char hex_map[] = "0123456789abcdef";

uint8_t hex_to_digit(char ch)
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

std::string decode_hex(const void* data, uint64_t len)
{

    auto src = static_cast<const uint8_t*>(data);
    std::string str(len * 2, 0);
    for (int i = 0; i < len; ++i) {
        str[2 * i] = hex_map[(src[i] & 0xf0) >> 4];
        str[2 * i + 1] = hex_map[src[i] & 0x0f];
    }
    return str;
}

bool valid_ethereum_addr(std::string addr)
{
    eosio_assert(addr.size() % 2 == 0, "invalid eth addr");

    auto prefix_index = addr.find("0x");
    if (prefix_index == 0)
        addr = addr.substr(prefix_index+2);

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