bos.pegtoken
-----------

This contract allows users to create, issue, and manage peg tokens from other blockchain on
eosio based blockchains.


功能
-------------
1. 表中记录用户提现交易id，和原链交易id
2. 用户提出分配地址，才会给他分配，需要提供分配地址的账户，这些账户有权限给用户添加标志。
3. 提供命令，实现锁定合约，锁定后任何用户不能再执行交易
4. 锁定合约的情况下，支持使用sudo权限改变任意用户的token余额。
5. 大额转入需要二次认证才能给用户发行



actions
-----------
```
 [[eosio::action]]
 void init( symbol sym_base, string repeatable);
 调用权限: _self
 功能: 1. 设置基础符号，例如BTC、ETH等，各个承兑商设置的锚定币符号必须以基础符号开头；
      2. 设置不同承兑商的充值地址是否可以重复，BTC和ETH的充值地址是不可以重复的，而EOS的充值地址是在一个账号的基础上的一个memo编号，
         这些编号是可以重复的。
 参数: 
    1. sym_base 基础符号
    2. repeatable 不同承兑商为用户分配的充值地址是否可以重复。
 
 [[eosio::action]]
 void create( name    issuer,
              asset   maximum_supply,
              string  organization,
              string  website,
              string  miner_fee,
              string  service_fee,
              string  unified_recharge_address,
              string  state);
 调用权限: _self
 功能: 1. 创建承兑商
 参数: 
    1. issuer           锚定币发行账号
    2. maximum_supply   最大发行量，注意符号需要以sym_base开头
    3. organization     组织名字（支持中文）
    4. website          组织官网
    5. miner_fee        矿工费，本处只是文字描述，合约中并没有相关业务逻辑。
    6. service_fee      服务费，本处只是文字描述，合约中并没有相关业务逻辑。
    7. unified_recharge_address  统一充值地址，用于类似EOS的交易所充值方式，
                        所有用户使用同一个账号充值，使用memo中的编号区分不同用户，此处记录统一的充值地址。
    8. state            状态描述、暂停/活跃
 
 [[eosio::action]]
 void setmaxsupply( asset maximum_supply );
 调用权限: _self
 功能: 1. 设置某个承兑商可以发行的最大资产总量。
 参数: maximum_supply 最大发行量
 
 
 [[eosio::action]]
 void update( symbol_code sym_code,
              string  parameter,
              string  value );
 调用权限: issuer
 功能: 1. 设置某个承兑商相关信息。
 参数: 
    1. sym_code     承兑商发行的锚定币符号 例如 "BTCA"
    2. parameter    参数名
    3. value        参数值
 
 
 [[eosio::action]]
 void assignaddr( symbol_code  sym_code,
                  name         to,
                  string       address );
 调用权限: issuer
 功能: 1. 承兑商为用户分配充值地址。
 参数: 
    1. sym_code     承兑商发行的锚定币符号 例如 "BTCA"
    2. to           bos用户名
    3. address      锚定币充值地址                 
                  

 [[eosio::action]]
 void issue( name to, asset quantity, string memo );
   
 [[eosio::action]]
 void retire( asset quantity, string memo );

 [[eosio::action]]
 void transfer( name    from,
                name    to,
                asset   quantity,
                string  memo );

 [[eosio::action]]
 void withdraw( name    from,
                string  to,
                asset   quantity,
                string  memo );
 调用权限: from
 功能: 1. 用户将锚定币提现到原链。
 参数: 
    1. from         提币用户名
    2. to           锚定币原链的账户地址
    3. quantity     锚定币数量   
    4. memo         附加信息
    
 [[eosio::action]]
 void feedback( symbol_code  sym_code,
                uint64_t     id,
                uint8_t      state,
                string       memo );
 调用权限: issuer
 功能: 1. 承兑商反馈用户提币信息，是否成功等。
 参数: 
    1. sym_code     承兑商发行的锚定币符号 例如 "BTCA"
    2. id           用户提币记录的id
    3. state        状态，// TODO 规定状态值
    4. memo         附加信息


 [[eosio::action]]
 void rollback( symbol_code  sym_code,
                uint64_t     id,
                string       memo );
 调用权限: issuer
 功能: 1. 当用户的提币失败后（例如提币地址错误等），承兑商回滚用户提币交易。
 参数: 
    1. sym_code     承兑商发行的锚定币符号 例如 "BTCA"
    2. id           用户提币记录的id
    3. memo         附加信息

 [[eosio::action]]
 void open( name owner, const symbol& symbol, name ram_payer );

 [[eosio::action]]
 void close( name owner, const symbol& symbol );
```

tables
----------

```
 struct [[eosio::table("global")]] global_ts {
    symbol sym_base;
    bool   repeatable;   // recharge addresses can be used repeatably between different organizations.
 };

 struct [[eosio::table]] symbol_code_ts {
    symbol_code  sym_code;

    uint64_t primary_key()const { return sym_code.raw(); }
 };

 struct [[eosio::table]] recharge_address_ts {
    name           owner;
    string         address;
    time_point_sec create_time;
    time_point_sec last_update;

    uint64_t primary_key()const { return owner.value; }
    uint64_t by_address()const { return hash64( address ); }
 };

 struct [[eosio::table]] withdraw_ts {
    uint64_t       id;
    name           from;
    string         to;
    asset          quantity;
    time_point_sec create_time;
    time_point_sec feedback_time;
    string         feedback_msg;
    uint8_t        state;

    uint64_t primary_key()const { return id; }
    uint64_t by_time()const { return static_cast<uint64_t>(create_time.sec_since_epoch()); }
 };

 struct [[eosio::table]] account {
    asset    balance;

    uint64_t primary_key()const { return balance.symbol.code().raw(); }
 };

 struct [[eosio::table]] currency_stats {
    asset   supply;
    asset   max_supply;
    name    issuer;
    string  organization;
    string  website;
    string  miner_fee;
    string  service_fee;
    string  unified_recharge_address;
    string  state;

    uint64_t primary_key()const { return supply.symbol.code().raw(); }
 };

```



