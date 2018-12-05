#include "../include/bos.issuance.hpp"
#include "bos.helper.cpp"

using namespace eosio;
namespace bos {
    uint64_t bosio_issuance::superewardpert(uint64_t super_reward, uint64_t rank) {
        if (rank == 1) {
            return super_reward * 0.4;
        } else if (rank == 2) {
            return super_reward * 0.2;
        } else if (rank == 3) {
            return super_reward * 0.16;
        } else if (rank == 4) {
            return super_reward * 0.12;
        } else if (rank == 5) {
            return super_reward * 0.08;
        } else if (rank == 6) {
            return super_reward * 0.04;
        }
    }


// TODO: secondary_index symbol
    void bosio_issuance::reward_token(uint64_t symraw) {
        uint64_t total_token_remain, total_bos_sold;
        auto sym_index = _dailyorders.get_index<name("symbol")>();
        // for a specific token
        auto itrdorder = sym_index.find(symraw);
        std::map <uint64_t, uint64_t> dayamt_map;                     // map<day，mtoken>
        std::vector <std::pair<uint64_t, uint64_t>> sortedayamt_vec;  // super6，follow by sorted day and percentage
        std::vector <std::pair<uint64_t, uint64_t>> daypert_vec;      // for normal award: pair<day, pert>

        // sort map by value
        // iter masset.amount for specific token
        for (; itrdorder != sym_index.end(); itrdorder++) {
            uint64_t day = std::get<1>(index_decode(itrdorder->symday));
            uint64_t tokenremain_oftheday = itrdorder->masset.amount;
            dayamt_map.insert(std::pair<uint64_t, uint64_t>(day, tokenremain_oftheday));
            total_token_remain += itrdorder->masset.amount;
            total_bos_sold += itrdorder->bosasset.amount;
        }
        MapSortOfValue(sortedayamt_vec, dayamt_map);


        uint64_t super_reward = total_token_remain * 0.25;
        // TODO: for eos, btc, eth for each day, select top 6 most valuable
        std::vector < std::pair < uint64_t, uint64_t > > ::iterator
        sortedit;
        uint64_t rank = 1;
        for (sortedit = sortedayamt_vec.begin(); sortedit != sortedayamt_vec.end() && rank <= 6; sortedit++) {
            eosio::print("\n the ranked by eos \n");
            eosio::print("day: ", sortedit->first);
            eosio::print("amount: ", sortedit->second);
            // get the symday for super reward
            uint64_t symday_topsix = index_encode(symraw, sortedit->first);
            auto topsix_dailyorder = _dailyorders.get(symday_topsix);

            uint64_t bos_of_theday = topsix_dailyorder.bosasset.amount;
            uint64_t supereward_of_theday = superewardpert(super_reward, rank);    // for a specific day, how much is bos reward
            double supereward_foreach_bos =
                    supereward_of_theday / bos_of_theday;  // for one day, how many token for supereward:

            std::set <name> userset = topsix_dailyorder.userset;                    // num of user for the day the symbol
            std::set<name>::iterator setit;
            for (setit = userset.begin(); setit != userset.end(); ++setit) {
                name f = *setit;
                userorders_table _userorders(_self, f.value);
                auto one_user = _userorders.get(symday_topsix);
                asset usertokenreward = supereward_foreach_bos * one_user.bosasset;
                addtoq( _self, _self, f, usertokenreward, "Top "+std::to_string(rank)+" super reward");   // finish sum up
            }
            rank++;
        } // end of super reward (top six)
        processq();

        // per user per time
        uint64_t normal_reward = total_token_remain * 0.25;
        double price_ofreward = normal_reward / total_bos_sold;    // TODO: double handling;  weight handling
        for (auto &acct : _pioneeraccts) {   // iter pioneer table
            userorders_table _userorders(_self, acct.acct.value);
            asset totalbos_oftheuser;
            for (auto &user: _userorders) {
                totalbos_oftheuser += user.bosasset;
            }
            asset usertokenreward = totalbos_oftheuser * price_ofreward;
            addtoq( _self, _self, acct.acct, usertokenreward, "reward");
        }
        processq();
    }


}


//
//userorders_table _userorders(_self, itr->to.value);
//auto userorders_lookup = _userorders.find(symday);  // if (userorders_lookup != _userorders.end())
//_userorders.modify(userorders_lookup, _self, [&](auto &update_user_order) {
//update_user_order.reward += itr->quantity;
//});