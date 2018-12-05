#pragma once
#include "../include/bos.issuance.hpp"
#include <tuple>
#include <utility>
#include <map>
#include <cmath>
#include <iomanip>
#include <sstream>
//#include <boost/lexical_cast.hpp>
#include <stdlib.h>
//#include <boost/property_tree/ptree.hpp>
//#include <boost/property_tree/json_parser.hpp>

using namespace eosio;
namespace bos {
    //map数据按照值来排序
    void bosio_issuance::MapSortOfValue(std::vector<std::pair<uint64_t,uint64_t> >& vec,std::map<uint64_t,uint64_t>& m)
    {
        for (map<uint64_t, uint64_t>::iterator mapit = m.begin(); mapit != m.end(); mapit++)
            vec.push_back(make_pair(mapit->second, mapit->first));
        sort(vec.begin(), vec.end());
    }

    uint64_t bosio_issuance::index_encode(uint64_t sym, uint64_t day) {
        uint64_t res = 0;
        for (int i = 0; sym & 0xFF && i < 7; i++) {
            res |= (sym & 0x1F) << (5 * i);
            sym >>= 8;
        }
        res <<= 16;
        res |= day & 0xFFFF;
        return res;
    }

    std::tuple<uint64_t, uint64_t> bosio_issuance::index_decode(uint64_t symday) {
        uint64_t symbol = 0, day = 0;
        day = symday & 0xFFFF;
        symday >>= 16;
        symbol = 0;
        for (int i = 0; symday & 0x1F && i < 7; i++) {
            symbol |= (symday & 0x1F) << (8 * i);
            symday >>= 5;
        }
        return {symbol, day};
    }


    std::string bosio_issuance::uint_to_string(const uint64_t name) const
    {
        static const char *charmap = ".12345abcdefghijklmnopqrstuvwxyz";

        std::string str(13, '.');

        uint64_t tmp = name;
        for (uint32_t i = 0; i <= 12; ++i)
        {
            char c = charmap[tmp & (i == 0 ? 0x0f : 0x1f)];
            str[12 - i] = c;
            tmp >>= (i == 0 ? 4 : 5);
        }

        const auto last = str.find_last_not_of('.');
        if (last != std::string::npos)
            str = str.substr(0, last + 1);
        return str;
    }

    double bosio_issuance::roundAny(double r,int precision){
        std::stringstream buffer;
        buffer << std::fixed << setprecision(precision) << r;
        return stod(buffer.str());
    }
}