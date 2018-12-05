#include "../include/bos.issuance.hpp"
#include "bos.helper.cpp"
#include <stdio.h>      /* printf, scanf */
#include <time.h>       /* time_t, struct tm, time, mktime */
#include <unistd.h>
#include <eosiolib/time.hpp>
#include "bos.bidding.cpp"
#include "bos.refund.cpp"
#include "bos.dividend.cpp"
#include "bos.reward.cpp"
#include "bos.helper.cpp"
#include <math.h>
//namespace bos {
using namespace eosio;
namespace bos {
    // 12.12: 1544601600
    // 12.24: 1545638400
    // 12.25: 1545724800
    // 1.1:   1546329600
    bosio_issuance::issuestate bosio_issuance::get_default_global() {
        _issuance.set(issuestate{1545724800, 86400, 1571644800, 43200}, issuance_account);
        return _istate;
    }

    bosio_issuance::currday bosio_issuance::get_default_day() {
        _currday.set(currday{1}, issuance_account);
        return _curday;
    }

    void bosio_issuance::timechecking() {
        eosio::print("EOSHB: ", symbol_code("EOSHB").raw());
        eosio::print("ETHHB: ", symbol_code("ETHHB").raw());
        eosio::print("BTCHB: ", symbol_code("BTCHB").raw());
        uint64_t startbidtime = _istate.startbidtime;
        uint64_t oneday = _istate.oneday;
        uint64_t endbidtime = startbidtime + oneday * 300;
        uint64_t now = current_time() / 1000;
        eosio::print("now: ", now);
        eosio::print("startbidtime: ", startbidtime);
        uint64_t today = (now-startbidtime)/oneday/1000 + 1;
        print("(now-startbidtime)/oneday: ", (now-startbidtime)/oneday);
        print("today: ", today);
        if(today > _curday.today){
            print("NewDay! sum up yesterday token income !");
            foreach_token(today - 1, "calculatebid");
            _currday.set(currday{today}, issuance_account);
        }
        if(today >= 91 && today < 301){
            print("It is time to calculate dividend !");
            foreach_token(today, "calculatedividend");
        }else if(today >= 301){
            print("bos bidding has ended");
//            foreach_token(301, "reward_token");
        }
    }


    //
    void bosio_issuance::init(uint64_t startbidtime, uint64_t oneday, uint64_t refundbuffertime) {
        require_auth( _self );
        _currday.set(currday{1}, issuance_account);
        _issuance.set(issuestate{startbidtime, oneday, startbidtime+300*oneday, refundbuffertime}, issuance_account);
    }


    void bosio_issuance::foreach_token(uint64_t day, const string option) {
        eosio::print("option: ", option);
        if (option == "calculatebid") {
            eosio::print("I am here: ");
            calculatebid(symbol_code("EOSHB").raw(), day);
            calculatebid(symbol_code("BTCHB").raw(), day);
            calculatebid(symbol_code("ETHHB").raw(), day);
            processq(); //测试用
        } else if (option == "reward_token") {
            reward_token(symbol_code("EOSHB").raw());
            reward_token(symbol_code("BTCHB").raw());
            reward_token(symbol_code("ETHHB").raw());
        } else if (option == "calculatedividend") {
            caldividend(symbol_code("EOSHB").raw(), day);
            caldividend(symbol_code("BTCHB").raw(), day);
            caldividend(symbol_code("ETHHB").raw(), day);
            processq();
        }
    }

    uint64_t bosio_issuance::get_real_amt(asset masset){
//        asset masset = asset{125824233323, symbol("BOS", 4)};;
        uint64_t a = masset.amount / pow(10, masset.symbol.precision());
        return a;
    }

    name bosio_issuance::get_deploy_acct(uint64_t sycraw){
        if(sycraw == symbol_code("EOSHB").raw()){
            return pegeos_account;
        }else if(sycraw == symbol_code("ETHHB").raw()){
            return pegeth_account;
        }else if(sycraw == symbol_code("BTCHB").raw()){
            return pegbtc_account;
        }
    }

    void bosio_issuance::transfer(const name sender, const name receiver) {
//        eosio_assert(false, "Hello Transfer");
        array<char, 33> owner_pubkey_char;
        array<char, 33> active_pubkey_char;
        const auto transfer = unpack_action_data<bosio_issuance::trxferstruct>();
        if (transfer.from == _self || transfer.to != _self) {
            // this is an outgoing transfer, do nothing
            return;
        }
        print("transfer.quantity.symbol:", transfer.quantity.symbol);
        //TODO: verify_maximum_supply in pegTokend & extended_asset
        eosio_assert(transfer.quantity.symbol == symbol("BOS", 4) || transfer.quantity.symbol == symbol("EOSHB", 4) ||
                     transfer.quantity.symbol == symbol("ETHHB", 8) ||
                     transfer.quantity.symbol == symbol("BTCHB", 8),
                     "Must be BOS or EOS or ETH or BTC");
        eosio_assert(transfer.quantity.is_valid(), "Invalid token transfer");
        eosio_assert(transfer.quantity.amount > 0, "Quantity must be positive");

        uint64_t symday;
        uint64_t today = _curday.today;
        if (transfer.quantity.symbol == symbol("EOSHB", 4) ||
            transfer.quantity.symbol == symbol("ETHHB", 8) ||
            transfer.quantity.symbol == symbol("BTCHB", 8)) {
            symday = index_encode(transfer.quantity.symbol.code().raw(), today);
            eosio::print("transfer.quantity.symbol.code().raw(): ", transfer.quantity.symbol.code().raw());
            eosio::print("(symbol_code(EOSHB).raw(): ", (symbol_code("EOSHB").raw()));
            eosio::print("symbol(EOSHB, 4).code().raw(): ", symbol("EOSHB", 4).code().raw());
            eosio::print("today:", today);
            eosio::print("symday: ", symday);
            bosio_issuance::bidbos(sender, symday, transfer.quantity, today);
        }else if(transfer.quantity.symbol == symbol("BOS", 4) && validaterefundmemo(transfer.memo)){
                symday = index_encode(transfer.quantity.symbol.code().raw(), today);
                uint64_t day = static_cast<uint64_t>(std::stoi(transfer.memo.substr(0,3)));
                print("day: ", day);
                string symraw = transfer.memo.substr(4, transfer.memo.length()-4);
//                print(symraw);
//                print("symraw: ", symraw);
                refundtoken(sender, symday, transfer.quantity);
        }

    };


    void bosio_issuance::printhello() {
        eosio::print("singleton_example");
    }

    void bosio_issuance::notify(name user, std::string msg) {
//        require_auth(get_self());
        require_recipient(user);
    }


    void send_summary(name user, std::string message) {
//        action(
//                permission_level{_self(),"active"_n},
//                _self(),
//                "notify"_n,
//                std::make_tuple(user, name{user}.to_string() + message)
//        ).send();
    };


    void bosio_issuance::processq() {
        pendxfer_table pendxfer(_self, _self.value);

        bool empty = false;
        auto itr = pendxfer.begin();

        uint64_t payload = itr->xfer_id;
        eosio::transaction out{};
        deferfuncarg
                a = {.payload = payload};
        print("payload: ", payload);
        out.actions.emplace_back(permission_level{_self, "active"_n}, _self, "processxfer"_n, a);
        out.delay_sec = 1;
        uint128_t sender_id = now() + itr->xfer_id;
        out.send(sender_id, _self);
    }

    void bosio_issuance::processxfer(uint64_t payload) {
        pendxfer_table pendxfer(_self, _self.value);
        auto itr = pendxfer.find(payload);
        eosio_assert(itr != pendxfer.end(), "Transfer ID is not found.");

        print("---------- Transfer -----------\n");
        print("Transfer ID:      ", itr->xfer_id, "\n");
        print("Token Contract:   ", name{itr->token_contract}, "\n");
        print("From:             ", name{itr->from}, "\n");
        print("To:               ", name{itr->to}, "\n");
        print("Amount:           ", itr->quantity, "\n");
        print("Memo:             ", itr->memo, "\n");
        print("---------- End Transfer -------\n");

        // 发送transfer action
        action(
                permission_level{itr->from, "active"_n},
                itr->token_contract, "transfer"_n,
                std::make_tuple(itr->from, itr->to, itr->quantity, itr->memo))
                .send();




        // 计入compxfer table
        compxfer_table compxfer(_self, _self.value);
        compxfer.emplace(_self, [&](auto &t) {
            t.xfer_id = compxfer.available_primary_key();
            t.token_contract = itr->token_contract;
            t.from = itr->from;
            t.to = itr->to;
            t.quantity = itr->quantity;
            t.memo = itr->memo;
            t.created = itr->created;
            t.completed = now();
        });

        itr++;
        if (itr != pendxfer.end()) {
            uint64_t payload = itr->xfer_id;
            eosio::transaction out{};
            print("---------- Deferring Transaction -----------\n");
            print("Transfer ID:      ", itr->xfer_id, "\n");
            print("Token Contract:   ", name{itr->token_contract}, "\n");
            print("From:             ", name{itr->from}, "\n");
            print("To:               ", name{itr->to}, "\n");
            print("Amount:           ", itr->quantity, "\n");
            print("Memo:             ", itr->memo, "\n");
            print("---------- End Transfer ------------------\n");
            deferfuncarg
                    a = {.payload = payload};
            out.actions.emplace_back(permission_level{_self, "active"_n}, _self, "processxfer"_n, a);
            out.delay_sec = 1;
            uint128_t sender_id = now() + itr->xfer_id;
            out.send(sender_id, _self);
        }
        itr--;
        itr = pendxfer.erase(itr);

    }


    void bosio_issuance::addtoq(const name _token_contract,
                                const name _from,
                                const name _to,
                                const asset _asset,
                                const string _memo) {

        pendxfer_table pendxfer(_self, _self.value);
        pendxfer.emplace(_self, [&](auto &t) {
            t.xfer_id = pendxfer.available_primary_key();
            t.token_contract = _token_contract;
            t.from = _from;
            t.to = _to;
            t.quantity = _asset;
            t.memo = _memo;
            t.created = now();
        });
    }


#define EOSIO_DISPATCH_EX(TYPE, MEMBERS)                                            \
  extern "C" {                                                                 \
  void apply(uint64_t receiver, uint64_t code, uint64_t action) {              \
    if (action == "onerror"_n.value) {                                                \
      /* onerror is only valid if it is for the "eosio" code account and       \
       * authorized by "eosio"'s "active permission */                         \
      eosio_assert(code == "eosio"_n.value, "onerror action's are only valid from "   \
                                     "the \"eosio\" system account");          \
    }                                                                          \
    auto self = receiver;                                                      \
    if (code == receiver || code == "eosio.token"_n.value || code == "bos.btc"_n.value  || code == "bos.eos"_n.value  || code == "bos.eth"_n.value || action == "onerror"_n.value  ) {      \
      switch (action) { EOSIO_DISPATCH_HELPER(TYPE, MEMBERS) }                             \
      /* does not allow destructor of thiscontract to run: eosio_exit(0); */   \
    }                                                                          \
  }                                                                            \
  }
    EOSIO_DISPATCH_EX(bosio_issuance, (transfer)(init)(timechecking)(addtoq)(processq)(processxfer)(caldividend))
}