#include <redpacket.hpp>
#include <eosiolib/transaction.hpp>
#include "utils.hpp"
#include "types.hpp"
#include "murmurhash.hpp"

#define IS_DEBUG true

#if IS_DEBUG
    #define debug(args...) print(" | ", ##args)
#else
    #define debug(args...)
#endif

static constexpr uint32_t MAX_RECEIVER = 100;
static constexpr uint64_t MIN_AMOUNT = 10;
static constexpr uint64_t SEND_MIN_AMOUNT = 1000;
static constexpr uint32_t EXPIRE_TIME = 3600 * 24;

void redpacket::get( name receiver, uint64_t id, signature sig )
{
    debug("get");
    _require_caller_auth();

    _ping();

    asset amount = _get(receiver, id, sig, false);
    eosio_assert(amount.amount > 0, "asset too small");
    _transfer(_self, receiver, amount, _get_memo_for_redpacket(id));
}

void redpacket::create( name account, public_key owner_key, public_key active_key, uint64_t id, signature sig )
{
    debug("create");
    _require_caller_auth();

    _ping();

    asset amount = _get(account, id, sig, true);
    eosio_assert(amount.amount > 0, "asset too small");
    _create_account(account, owner_key, active_key, amount);
}

/**
 * 1. 创建账号
 *    act^new_account_name^owner_key^active_key
 * 2. 创建红包
 *    hb^hb_type^hb_id^hb_count^hb_pubkey^sender^greetings
 *
 *    hb_type:
 *    1. 普通
 *    2. 随机
 *    3. 建账号
 **/
void redpacket::transfer(name from, name to, asset quantity, string memo)
{
    debug("transfer");

    if (from == _self || to != _self || memo.length() == 0) {
        return;
    }
    bool is_bos = (_code == BOS_CONTRACT) && (quantity.symbol == BOS_SYMBOL);
    bool is_eos = (_code == EOS_CONTRACT) && (quantity.symbol == EOS_SYMBOL);
    eosio_assert(is_bos || is_eos, "unsupported symbol");
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must transfer positive quantity");

    vector<string> params = split(memo, "^");
    eosio_assert(params.size() > 0, "invalid memo");
    auto cmd = params[0];
    debug("cmd: " + cmd);

    if (cmd == "act") {
        eosio_assert(is_bos, "only BOS can be used to create account");
        eosio_assert(params.size() == 4, "invalid memo");
        auto account = decode_name(params[1]);
        auto owner_key = decode_pubkey(params[2]);
        auto active_key = decode_pubkey(params[3]);

        _create_account(account, owner_key, active_key, quantity);

    } else if (cmd == "hb") {
        eosio_assert(params.size() == 7, "invalid memo");
        auto type = static_cast<RedpacketType>(std::stoi(params[1]));
        eosio_assert(type >= HT_NORMAL && type <= HT_ACCOUNT, "unknown redpacke type");
        if (type == HT_ACCOUNT) {
            eosio_assert(is_bos, "only BOS can be used to create account");
        }

        auto id = std::stoull(params[2]);
        auto count = std::stoull(params[3]);
        eosio_assert(count > 0 && count <= MAX_RECEIVER, "illegal receiver count");
        uint64_t send_min_amount = SEND_MIN_AMOUNT;
        uint64_t min_amount = MIN_AMOUNT;
        eosio_assert(send_min_amount <= quantity.amount, "asset too small");
        eosio_assert(min_amount * count <= quantity.amount, "asset too small");

        auto pubkey = decode_pubkey(params[4]);
        auto sender = decode_name(params[5]);
        auto greetings = params[6];

        redpacket_table redpacket{_self, _self.value};
        redpacket.emplace(_self, [&](auto& h) {
            h.id = id;
            h.type = type;
            h.count = count;
            h.amount = quantity;
            h.sender = sender;
            h.pubkey = pubkey;
            h.greetings = greetings;
            h.expire = now() + EXPIRE_TIME;
        });

        stats_table stats{_self, _self.value};
        auto it = stats.find(quantity.symbol.code().raw());
        if (it == stats.end()) {
            stats.emplace(_self, [&](auto& s) {
                s.total_num = 1;
                s.total_value = quantity;
            });
        } else {
            stats.modify(it, same_payer, [&](auto& s) {
                s.total_num += 1;
                s.total_value += quantity;
            });
        }
    } else {
        eosio_assert(false, "unknown command");
    }
}

void redpacket::_create_account(name account, public_key owner_key, public_key active_key, asset quantity)
{
    const auto ram = eosio::buyrambytes(1024 * 3);
    const auto cpu = eosio::asset(1500, BOS_SYMBOL);
    const auto net = eosio::asset(500, BOS_SYMBOL);
    const auto total = ram + cpu + net;
    const auto remain = quantity - total;
    eosio_assert(remain.amount >= 0, "not enough amount");

    key_weight owner_key_weight {
        .key = owner_key,
        .weight = 1
    };
    key_weight active_key_weight {
        .key = active_key,
        .weight = 1
    };

    authority owner = authority {
        .threshold = 1,
        .keys = { owner_key_weight },
        .accounts = {},
        .waits = {}
    };
    authority active = authority {
        .threshold = 1,
        .keys = { active_key_weight },
        .accounts = {},
        .waits = {}
    };

    newaccount new_account {
        .creator = _self.value,
        .name = account.value,
        .owner = owner,
        .active = active
    };

    action {
        permission_level{_self, name{"active"}},
        name{"eosio"},
        name{"newaccount"},
        new_account
    }.send();

    action {
        permission_level{_self, name{"active"}},
        name{"eosio"},
        name{"buyram"},
        std::make_tuple(_self.value, account, ram)
    }.send();

    action {
        permission_level{_self, name{"active"}},
        name{"eosio"},
        name{"delegatebw"},
        std::make_tuple(_self.value, account, net, cpu, true)
    }.send();

    if (remain.amount > 0) {
        _transfer(_self, account, remain, _self.to_string() + ": remain amount for creating account");
    }
}

void redpacket::_transfer(name from, name to, asset quantity, string memo)
{
    name real_to = to;
    string real_memo = memo;

    // compatible with uid
    std::string uid = ".uid";
    if (has_suffix(to.to_string(), uid)) {
        real_to = name{"uid"};
        real_memo = "tf^" + to.to_string() + "^" + memo;
    }
    bool is_eos = (quantity.symbol == EOS_SYMBOL);
    name contract = is_eos ? EOS_CONTRACT : BOS_CONTRACT;
    action {
        permission_level{ from, name{"active"} },
        contract,
        name{"transfer"},
        std::make_tuple(from, real_to, quantity, real_memo)
    }.send();
}

void redpacket::_require_caller_auth()
{
    global_table global(_self, _self.value);
    auto it = global.begin();
    eosio_assert(it != global.end(), "caller not set");

    require_auth(permission_level{it->caller, name{"redpacket"}});
}

asset redpacket::_get(name receiver, uint64_t id, signature sig, bool is_create)
{
    redpacket_table redpacket(_self, _self.value);
    auto it = redpacket.find(id);
    eosio_assert(it != redpacket.end(), "redpacket not found");
    eosio_assert(!it->out_of_date(), "out of date");
    eosio_assert(!it->has_claimed(receiver), "cannot get twice");
    eosio_assert(it->type >= HT_NORMAL && it->type <= HT_ACCOUNT, "unknown redpacket type");
    if (is_create) {
        eosio_assert(it->amount.symbol == BOS_SYMBOL, "only BOS can be used to create account");
    }

    uint32_t hash = murmur_hash2((const char *)sig.data.data(), sizeof(sig.data));
    eosio_assert(!it->sig_already_used(hash), "sig_already_used");

    auto msg = std::to_string(id);
    assert_recover_key(sha256(msg.c_str(), msg.size()), sig, it->pubkey);

    if (it->type == HT_ACCOUNT) {
        eosio_assert(is_create, "this redpacket can only be used to create account");
    }

    asset amount{0, it->amount.symbol};
    auto user_remain = it->count - it->claims.size();
    eosio_assert(user_remain > 0, "redpacket is over");

    if (user_remain == 1) {
        amount = it->get_remain();

        // defer remove after 12 hours
        transaction out;
        out.actions.emplace_back(permission_level{_self, name{"active"}}, _self, name{"remove"}, it->id);
        out.delay_sec = 3600 * 12;
        uint128_t sender_id = uint128_t(it->id) << 64 | 1;
        cancel_deferred(sender_id);
        out.send(sender_id, _self, true);

    } else {
        if (it->type == HT_RANDOM) {
            // [0 ~ 剩余金额随机]
            auto amount_remain = it->get_remain();
            auto tmp = tapos_block_prefix();
            auto amount_num = static_cast<int64_t>(random((void *)&tmp, sizeof(tmp)));
            amount_num %= amount_remain.amount;

            // 期望设置为平均，随机金额为0～2倍平均
            amount_num *= (2.0 / user_remain);

            uint64_t min_amount =  MIN_AMOUNT;
            uint64_t min_remain_needed = (user_remain - 1) * min_amount;
            // 如果剩余不够分,期望设置为可取的最大值
            if (amount_num > amount_remain.amount - min_remain_needed) {
                amount_num = amount_remain.amount - min_remain_needed;
            }
            // 期望值不能小于最小值
            if (amount_num < min_amount) {
                amount_num = min_amount;
            }

            amount.amount = amount_num;

        } else if (it->type == HT_NORMAL || it->type == HT_ACCOUNT) {
            // 平均红包
            amount = asset(it->amount.amount / it->count, it->amount.symbol);
        } else {
            eosio_assert(false, "unknown redpacke type");
        }
    }

    eosio_assert(amount.amount > 0, "asset too small");
    // 领取记录
    redpacket.modify(it, same_payer, [&](auto& h) {
        h.claims.emplace_back(receiver, amount, hash, is_create);
    });

    return amount;
}

string redpacket::_get_memo_for_redpacket(uint64_t id)
{
    redpacket_table redpacket(_self, _self.value);
    auto it = redpacket.find(id);
    eosio_assert(it != redpacket.end(), "redpacket not found");

    return "redpacket " + std::to_string(id) + " from " + it->sender.to_string() + ":" + it->greetings;
}

void redpacket::_ping()
{
    debug("ping");
    redpacket_table redpacket(_self, _self.value);
    auto idx = redpacket.get_index<name{"expire"}>();
    auto it = idx.begin();
    if (it == idx.end() || !it->out_of_date()) {
        return;
    }

    debug("delete: ", it->id);
    uint64_t id = it->id;
    bool is_eos = (it->amount.symbol == EOS_SYMBOL);
    name contract = is_eos ? EOS_CONTRACT : BOS_CONTRACT;
    string memo = _self.to_string() + ": remain balance for redpacket " + std::to_string(id);
    transaction out;
    out.actions.emplace_back(permission_level{_self, name{"active"}},
            contract,
            name{"transfer"},
            std::make_tuple(_self, it->sender, it->get_remain(), memo));
    out.delay_sec = 0;
    uint128_t sender_id = uint128_t(id) << 64 | 2;
    cancel_deferred(sender_id);
    out.send(sender_id, _self, true);

    idx.erase(it);
}

#define EOSIO_DISPATCH_CUSTOM( TYPE, MEMBERS ) \
extern "C" { \
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
        if ( (code == BOS_CONTRACT.value && action == name{"transfer"}.value) || \
                (code == EOS_CONTRACT.value && action == name{"transfer"}.value) ) { \
            execute_action(name(receiver), name(code), &redpacket::transfer); \
        } else if ( code == receiver ) { \
            switch( action ) { \
                EOSIO_DISPATCH_HELPER( TYPE, MEMBERS ) \
            } \
            /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
        } \
    } \
} \

EOSIO_DISPATCH_CUSTOM(redpacket, (clear)(setcaller)(ping)(remove)(get)(create))

