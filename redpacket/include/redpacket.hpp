#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <vector>
#include <algorithm>

using namespace eosio;
using std::string;
using std::vector;

CONTRACT redpacket: public contract {
    public:
        using contract::contract;

        // for debug
        ACTION clear()
        {
            require_auth(_self);

            stats_table stats{_self, _self.value};
            auto it_s = stats.begin();
            while (it_s != stats.end()) {
                it_s = stats.erase(it_s);
            }

            redpacket_table redpacket{_self, _self.value};
            auto it_r = redpacket.begin();
            while (it_r != redpacket.end()) {
                it_r = redpacket.erase(it_r);
            }
        }

        ACTION setcaller( name caller )
        {
            require_auth(_self);

            global_table global{_self, _self.value};
            if (global.begin() == global.end()) {
                global.emplace(_self, [&](auto& g) {
                    g.caller = caller;
                });
            } else {
                global.modify(global.begin(), same_payer, [&](auto& g) {
                    g.caller = caller;
                });
            }
        }

        ACTION ping()
        {
            _require_caller_auth();
            _ping();
        }

        // for defer remove
        ACTION remove(uint64_t id)
        {
            require_auth(_self);

            redpacket_table redpacket(_self, _self.value);
            auto it = redpacket.find(id);
            if (it != redpacket.end()) {
                redpacket.erase(it);
            }
        }

        ACTION get( name receiver, uint64_t id, signature sig );

        ACTION create( name account, public_key owner_key, public_key active_key, uint64_t id, signature sig );

        void transfer(name from, name to, asset quantity, string memo);

    private:
        // 红包类型
        enum RedpacketType: uint8_t {
            HT_UNSET = 0,
            HT_NORMAL = 1,
            HT_RANDOM,
            HT_ACCOUNT,

            HT_UNKNOWN = 255
        };

        // 全局设置
        TABLE global {
            uint64_t id = 0;
            name caller;

            uint64_t primary_key() const { return id; }

            EOSLIB_SERIALIZE(global, (id)(caller))
        };

        // 统计信息
        TABLE stats {
            uint64_t total_num;
            asset total_value;

            uint64_t primary_key() const { return total_value.symbol.code().raw(); }

            EOSLIB_SERIALIZE(stats, (total_num)(total_value))
        };

        // 领取记录
        struct claim_info {
            name user;
            asset amount;
            uint32_t sig_hash;
            bool is_create;

            claim_info() = default;
            claim_info(name u, asset a, uint32_t h, bool c)
                : user(u), amount(a), sig_hash(h), is_create(c)
            {}

            EOSLIB_SERIALIZE(claim_info, (user)(amount)(sig_hash)(is_create))
        };

        // 红包信息
        TABLE redpacket_info {
            uint64_t id;
            uint8_t type;
            uint32_t  count;
            asset amount;
            name sender;
            public_key pubkey;
            string greetings;
            uint32_t expire;
            vector<claim_info> claims;

            uint64_t primary_key() const { return id; }
            uint64_t by_sender() const { return sender.value; }
            uint64_t by_expire() const { return static_cast<uint64_t>(expire); }

            EOSLIB_SERIALIZE(redpacket_info, (id)(type)(count)(amount)(sender)(pubkey)(greetings)(expire)(claims))

            bool out_of_date() const { return now() > expire;}

            bool has_claimed(name user) const {
                auto it = std::find_if(claims.begin(), claims.end(), [&](auto& c) {
                        return c.user.value == user.value;
                        });
                return it != claims.end();
            }

            bool sig_already_used(uint32_t hash) const {
                auto it = std::find_if(claims.begin(), claims.end(), [&](auto& c) {
                        return c.sig_hash == hash;
                        });
                return it != claims.end();
            }

            asset get_remain() const {
                auto remain = amount;
                for (auto& c : claims) {
                    remain -= c.amount;
                }
                return remain;
            }
        };

        using global_table = multi_index<"global"_n, global>;
        using stats_table = multi_index<"stats"_n, stats>;
        using redpacket_table = multi_index<"redpacket"_n, redpacket_info,
              indexed_by<"sender"_n, const_mem_fun<redpacket_info, uint64_t, &redpacket_info::by_sender>>,
              indexed_by<"expire"_n, const_mem_fun<redpacket_info, uint64_t, &redpacket_info::by_expire>>>;

    private:
        void _create_account(name account, public_key owner_key, public_key active_key, asset quantity);

        void _transfer(name from, name to, asset quantity, string memo);

        void _require_caller_auth();

        asset _get(name receiver, uint64_t id, signature sig, bool is_create);

        string _get_memo_for_redpacket(uint64_t id);

        void _ping();
};

