#include "../include/bos.issuance.hpp"
#include "bos.helper.cpp"

using namespace eosio;
namespace bos {
    // 这里不更新dailybids或则dailyorders，仍保持原样，用ava
    void bosio_issuance::caldividend(uint64_t divdsymraw, uint64_t today) {
        uint64_t first_divday; // 一级分红
        uint64_t second_divday = 0; // 二级分红
        asset dividend;
        if (91 <= today <= 300) {
            first_divday = today - 90;
            if(today >= 270){
                second_divday = today - 180;
            }
        } else if (today > 300) {
            first_divday = today - 30;
            second_divday = today - 120;
        }
        print("first_divday", first_divday);
        print("second_divday", second_divday);
        print("index_encode(divdsymraw, first_divday))", index_encode(divdsymraw, first_divday));
        auto itr1 = _dailybids.find(index_encode(divdsymraw, first_divday));
        if(itr1 != _dailybids.end()){
            print("firstday.masset: ", itr1->masset);
            print("一级 dividend on day: " + uint_to_string(first_divday), itr1->masset / 4);
            if((itr1->masset / 4).amount > 0){
                addtoq(get_deploy_acct(itr1->masset.symbol.code().raw()), issuance_account, dividend_account, itr1->masset / 4,
                       "send dividend token " + uint_to_string(dividend.symbol.code().raw()) + " on day: " +
                       to_string(first_divday));
            }
        }

        auto itr2 = _dailybids.find(index_encode(divdsymraw, second_divday));
        if(itr2 != _dailybids.end()){
            print("second_divday.masset: ", itr2->masset);
            print("二级 dividend on day: "  + uint_to_string(second_divday), itr2->masset / 8);
            if((itr2->masset / 8).amount > 0){
                addtoq(get_deploy_acct(itr2->masset.symbol.code().raw()), issuance_account, dividend_account, itr2->masset / 8,
                       "send dividend token " + uint_to_string(dividend.symbol.code().raw()) + " on day: " +
                       to_string(second_divday));
            }
        }


//        auto firstday = _dailybids.get(index_encode(divdsymraw, first_divday));
//        dividend = firstday.masset * 0.5 * 0.5;
//
//        if(second_divday > 0){
//            auto second_divday = _dailybids.get(index_encode(divdsymraw, second_divday));
//            dividend += second_divday.masset * 0.25 * 0.5;
//        }
//
//        // addtoq
//        if(dividend.amount > 0) {
//            addtoq(get_deploy_acct(dividend.symbol.code().raw()), issuance_account, dividend_account, dividend,
//                   "send dividend token " + uint_to_string(dividend.symbol.code().raw()) + " on day: " +
//                   uint_to_string(today));
//        }
//        INLINE_ACTION_SENDER(eosio::token, transfer)(
//                bostoken_account, {{_self, active_permission}},
//                {_self, dividend_account, dividend, std::string("send dividend BOS on day x")}

    }
}