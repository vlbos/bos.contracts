#include "../include/bos.issuance.hpp"
#include "bos.helper.cpp"
#include <stdio.h>      /* printf, scanf */
#include <time.h>       /* time_t, struct tm, time, mktime */
#include <unistd.h>
#include <eosiolib/time.hpp>

//namespace bos {
using namespace eosio;
namespace bos {
    void bosio_issuance::bidbos(const name owner, uint64_t symday, asset mappedasset, uint64_t today) {
        require_auth(owner);
        userbids_table _userbids(_self, owner.value);
        auto userbid_lookup = _userbids.find(symday);
        if (userbid_lookup == _userbids.end()) {
            _userbids.emplace(_self, [&](auto &new_userbid) {
                new_userbid.symday = symday;
                new_userbid.masset = mappedasset;
            });
        } else {
            _userbids.modify(userbid_lookup, _self, [&](auto &update_user_bid) {
                update_user_bid.masset = mappedasset + update_user_bid.masset;
            });
        }

        auto pioneeracct_lookup = _pioneeraccts.find(owner.value);
        if(pioneeracct_lookup == _pioneeraccts.end()){
            _pioneeraccts.emplace(_self, [&](auto &new_pioneeracct) { // payer for owner is the contract
                new_pioneeracct.acct = owner;
            });
        }

        auto dailybid_lookup = _dailybids.find(symday);
        if (dailybid_lookup == _dailybids.end()) {
            _dailybids.emplace(_self, [&](auto &new_dailybid) {
                new_dailybid.symday = symday;
                new_dailybid.masset = mappedasset;
                new_dailybid.day = today;
                std::set <name> newset;
                newset.insert(owner);
                new_dailybid.biddedusers = newset;
            });
        } else {
            _dailybids.modify(dailybid_lookup, _self, [&](auto &update_daily_bid) {
                update_daily_bid.masset = mappedasset + update_daily_bid.masset;
                set <name> existing_set = update_daily_bid.biddedusers;
                existing_set.insert(owner);
                update_daily_bid.biddedusers = existing_set;
            });
        }
    }

    uint64_t bosio_issuance::bos_token_amt(uint64_t bidsymraw){
        if(bidsymraw == symbol_code("EOSHB").raw()){
            return 1500000;
        }else if(bidsymraw == symbol_code("ETHHB").raw()){
            return 1000000;
        }else if(bidsymraw == symbol_code("BTCHB").raw()){
            return 500000;
        }
    }

    // 对于一个特定天和，计算应分发的bos数目
    void bosio_issuance::calculatebid(uint64_t bidsymraw, uint64_t yesterday) {
        // "eos"_n.value6138406292105986048day: 246eosday: 246
        uint64_t sym_day = index_encode(bidsymraw, yesterday);
        auto dailybid_lookup = _dailybids.find(sym_day);
        int64_t total_bos_out_amt = 0;
        if (dailybid_lookup != _dailybids.end()) {
            auto totaltoken = dailybid_lookup->masset.amount; //int64_t
            eosio::print("dailybid_lookup->masset.amount: ", dailybid_lookup->masset.amount);
            eosio::print("totaltoken: ", totaltoken);
            eosio::print("代币总数, get_real_amt: ", get_real_amt(dailybid_lookup->masset));
            //TODO: asset.amount得出错误
            double price = bos_token_amt(bidsymraw) / get_real_amt(dailybid_lookup->masset) ; //  一个token能换多少个BOS
            eosio::print("(一个token能换多少个BOS: ", price);
            std::set <name> userset = dailybid_lookup->biddedusers;
            std::set<name>::iterator setit;
            for (setit = userset.begin(); setit != userset.end(); ++setit) {
                name f = *setit; // Note the "*" here
                userbids_table _userbids(_self, f.value); // 建立user scope
                auto userbid_lookup = _userbids.find(sym_day);
                if (userbid_lookup != _userbids.end()) {
                    userorders_table _userorders(_self, f.value);
                    _userorders.emplace(_self, [&](auto &new_userorder) { // contract账户为user account付款
                        new_userorder.symday = sym_day;
                        new_userorder.masset = userbid_lookup->masset;
                        new_userorder.symbol = userbid_lookup->masset.symbol.code().raw();
                        int64_t bosamt = get_real_amt(userbid_lookup->masset) * price;
                        //TODO:
                        total_bos_out_amt += bosamt;
//                        eosio::print("bos数目:", bosamt);
                        asset bosasset = asset{bosamt, symbol("BOS", 4)};
                        if(bosasset != asset{0, symbol("BOS", 4)}) {
                            addtoq("eosio.token"_n, issuance_account, f, bosasset,
                                   "memo"); //发送BOS，则和bos.eos，bos.eth，bos.btc无关 get_deploy_acct(bidsymraw)
                        }else{
                            print("bosasset == asset{0, symbol(\"BOS\", 4)}: ", bosamt);
                        }
                        new_userorder.bosasset = bosasset;
                    });
                } else {
                    eosio::print("no user bids");
                }
            }
            _dailyorders.emplace(_self, [&](auto &new_dailyorder) {
                new_dailyorder.symday = sym_day;
                new_dailyorder.masset = dailybid_lookup->masset;
                new_dailyorder.symbol = dailybid_lookup->masset.symbol.code().raw();
                // asset没有+=的选项
                new_dailyorder.bosasset = asset{static_cast<int64_t>(total_bos_out_amt), symbol("BOS", 4)};
//                new_dailyorder.bosasset = asset{static_cast<int64_t>(15000000000), symbol("BOS", 4)};
                new_dailyorder.userset = dailybid_lookup->biddedusers;
            });
            // 找到eos income的所有用户
        } else {
            eosio::print("no "+ to_string(bidsymraw) +" income for day: ", yesterday);
        }

    }
}

//                    transaction t;
//                    t.actions.emplace_back(permission_level{_self, active_permission},
//                                           _self, "sendbos"_n,
//                                           std::make_tuple(f, dailybid_lookup->masset, price, sym_day)
//                    );
//                    t.delay_sec = 1;
//                    uint128_t deferred_id = _self.value + now();
//                    t.send(deferred_id, _self, true);

//    void bosio_issuance::sendbos(name receiver, asset masset, double price, uint64_t symday, uint64_t day) {
//        // 发送并入table
//        INLINE_ACTION_SENDER(eosio::token, transfer)(
//                bostoken_account, {{_self, active_permission}},
//                {_self, receiver, masset * price, std::string("distribute BOS on day "+ day)}
//        );
//        userorders_table _userorders(_self, receiver.value);
//        _userorders.emplace(_self, [&](auto &new_userorder) { // contract账户为user account付款
//            new_userorder.symday = symday;
//            new_userorder.masset = masset;
//            int64_t bosamt = masset.amount * price * 10000;
//            eosio::print("!!!!bosamt: !!!", bosamt);
//            asset bosasset = asset{bosamt, symbol("BOS", 4)};
//            new_userorder.bosasset = bosasset;
//        });
//    }

