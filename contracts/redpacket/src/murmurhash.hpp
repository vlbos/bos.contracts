#pragma once

uint32_t murmur_hash2(const char *data, size_t len)
{
    uint32_t h, k;

    h = 0 ^ len;

    while (len >= 4) {
        k = data[0] & 0xff;
        k |= (data[1] & 0xff) << 8;
        k |= (data[2] & 0xff) << 16;
        k |= (data[3] & 0xff) << 24;

        k *= 0x5bd1e995;
        k ^= k >> 24;
        k *= 0x5bd1e995;

        h *= 0x5bd1e995;
        h ^= k;

        data += 4;
        len -= 4;
    }

    switch (len) {
    case 3:
        h ^= (data[2] & 0xff) << 16;
        /* fall through */
    case 2:
        h ^= (data[1] & 0xff) << 8;
        /* fall through */
    case 1:
        h ^= data[0] & 0xff;
        h *= 0x5bd1e995;
    }

    h ^= h >> 13;
    h *= 0x5bd1e995;
    h ^= h >> 15;

    return h;
}

