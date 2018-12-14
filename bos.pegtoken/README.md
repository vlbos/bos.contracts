bos.pegtoken
-----------

This contract allows users to create, issue, and manage peg tokens from other blockchain on
eosio based blockchains.


TODO
-------------
1. 完成函数 checksum256_to_string
2. 添加以太坊地址判断
3. 定制策略，删除 withdraw_table 表中的陈旧数据
4. 给本合约添加测试用例
5. 完善并将README.md翻译为英文

功能
-------------
1. 表中记录用户提现交易id，和原链交易id
2. 用户提出分配地址，才会给他分配，需要提供分配地址的账户，这些账户有权限给用户添加标志。
3. 提供命令，实现锁定合约，锁定后任何用户不能再执行交易
4. 大额转入需要二次认证才能给用户发行


actions
-----------
```
 void create( name    issuer,
              name    auditor,
              asset   maximum_supply,
              asset   large_asset,
              asset   min_withdraw,
              name    address_style,
              string  organization,
              string  website,
              string  miner_fee,
              string  service_fee,
              string  unified_recharge_address,
              bool    active );
 调用权限: _self
 功能: 1. 创建承兑商
 参数: 
    1. issuer           锚定币发行账号
    2. auditor          承兑商审计账户，（当发行额度大于等于large_asset值时，需要审计账户确认，才可以完成发行动作）
    3. maximum_supply   最大发行量，此处要特备注意小数点后0的个数，此个数为token的精度，一旦设定无法修改
                        例如为比特币设定8位精度，符号为BTC，发行量为1千万，此处应该填写 "10000000.00000000 BTC"
    4. large_asset      大额资产阈值，大于或等于此值的资产算为大额资产
    5. min_withdraw     最小withdraw限额
    6. address_style    被映射代币原链的地址类型，此值必须为bitcoin、ethereum、eosio、other四个中的一种
                        比特币使用bitcoin，以太币和以太坊上的其他代币使用ethereum，eos和eos上的其他代币使用eosio
                        其他代币使用other，当使用other时，不进行地址有效性检验。
    7. organization     组织名字（支持中文）
    8. website          组织官网
    9. miner_fee        矿工费，本处只是文字描述，合约中并没有相关业务逻辑。
    10. service_fee      服务费，本处只是文字描述，合约中并没有相关业务逻辑。
    11. unified_recharge_address  统一充值地址，用于类似EOS的交易所充值方式，
                        所有用户使用同一个账号充值，使用memo中的编号区分不同用户，此处记录统一的充值地址。
    12. active          承兑商处于活跃/非活跃状态

 
 void setwithdraw( asset min_withdraw );
 调用权限: auditor
 功能: 1. 设置用户每次withdraw最低额度
 参数: min_withdraw  withdraw最低额度


 void setmaxsupply( asset maximum_supply );
 调用权限: _self
 功能: 1. 设置某个承兑商可以发行的最大资产总量。
 参数: maximum_supply 最大发行量，例如 "10000000.00000000 BTC"
 
 
 void setlargeast( asset large_asset );
 调用权限: _self
 功能: 1. 设置大额资产阈值。
 参数: large_asset 大额资产阈值 ，例如 "10.00000000 BTC"
 
 
 void lockall( symbol_code sym_code );
 调用权限: _self
 功能: 1. 冻结合约，合约冻结后不能再执行issue、transfer、approve、unapprove、retire、withdraw、rollback。
 参数: sym_code 代币符号名，例如 "BTC"
 
 
 void unlockall( symbol_code sym_code );
 调用权限: _self
 功能: 1. 解冻合约
 参数: sym_code 代币符号名，例如 "BTC"
 
 
 void update( symbol_code sym_code,
              string  parameter,
              string  value );
 调用权限: issuer
 功能: 1. 设置某个承兑商相关信息。
 参数: 
    1. sym_code     承兑商发行的锚定币符号 例如 "BTC"
    2. parameter    参数名，例如 "organization"
    3. value        参数值，例如 "某某承兑商"
 
 
 void applicant( symbol_code   sym_code,
                 name          action,
                 name          applicant );
 调用权限: issuer
 功能: 1. 设置充值地址申请人
 参数: 
    1. sym_code     承兑商发行的锚定币符号 例如 "BTC"
    2. action       "add" 或 "remove"
    3. applicant    申请人账户
    
    
 void applyaddr( name          applicant,
                 name          to,
                 symbol_code   sym_code );
 调用权限: applicant
 功能: 1. 为用户申请地址
 参数: 
    1. applicant    申请人
    2. to           用户账户
    3. sym_code     承兑商发行的锚定币符号 例如 "BTC"
 
 
 void assignaddr( symbol_code  sym_code,
                  name         to,
                  string       address );
 调用权限: issuer
 功能: 1. 承兑商为用户分配充值地址。
 参数: 
    1. sym_code     承兑商发行的锚定币符号 例如 "BTC"
    2. to           bos用户名
    3. address      锚定币充值地址                 


 void issue( name to, asset quantity, string memo );
 调用权限: issuer
 功能: 1. 承兑商给用户发行资产，需要注意的是，承兑商必须调用此接口为用户充值，而不应该调用transfer
         因为transfer中不会检查大额资产阈值，并且为了承兑商内部核账方便，也应该使用issue而不是transfer
 参数: 
    1. to           账户名
    2. quantity     资产，例如 "1.00000000 BTC"
    3. memo         附加信息   
    

 void approve( symbol_code  sym_code ,
               uint64_t     issue_seq_num );
 调用权限: auditor
 功能: 1. 审计员认证发行
 参数: 
    1. sym_code         承兑商发行的锚定币符号 例如 "BTC"
    2. issue_seq_num    发行编号


 void unapprove( symbol_code  sym_code ,
                 uint64_t     issue_seq_num );
 调用权限: auditor
 功能: 1. 审计员否认发行
 参数: 
    1. sym_code         承兑商发行的锚定币符号 例如 "BTC"
    2. issue_seq_num    发行编号  


 void retire( asset quantity, string memo );
 调用权限: issuer
 功能: 1. 回收发行的代币，降低总发行量
 参数: 
    1. quantity     回收资产额度，例如 "1.00000000 BTC"
    2. memo         附加信息

 void transfer( name    from,
                name    to,
                asset   quantity,
                string  memo );


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
    

 void feedback( symbol_code  sym_code,
                uint64_t     id,
                uint8_t      state,
                string       memo );
 调用权限: issuer
 功能: 1. 承兑商反馈用户提币信息，是否成功等。
 参数: 
    1. sym_code     承兑商发行的锚定币符号 例如 "BTCA"
    2. id           用户提币记录的id
    3. state        状态， // 1 表示开始处理， 2 表示处理完成。如果此值大于等于10，表示未处理，是其初始值和此条记录的主键id相同。
    4. memo         附加信息


 void rollback( symbol_code  sym_code,
                uint64_t     id,
                string       memo );
 调用权限: issuer
 功能: 1. 当用户的提币失败后（例如提币地址错误等），承兑商回滚用户提币交易。
 参数: 
    1. sym_code     承兑商发行的锚定币符号 例如 "BTCA"
    2. id           用户提币记录的id
    3. memo         附加信息


 void open( name owner, const symbol& symbol, name ram_payer );


 void close( name owner, const symbol& symbol );


```
tables
-----------
``` 
 struct [[eosio::table]] symbol_ts {
    symbol  sym;

    uint64_t primary_key()const { return sym.code().raw(); }
 };

 struct [[eosio::table]] applicant_ts {
    name  applicant;

    uint64_t primary_key()const { return applicant.value; }
 };

 struct [[eosio::table]] recharge_address_ts {
    name           owner;
    string         address;
    time_point_sec assign_time;
    uint64_t       state;   状态， // 提供地址后设置为0。如果此值为非零，是其初始值，此值和此条记录的主键（owner.value）相同。

    uint64_t primary_key()const { return owner.value; }
    uint64_t by_address()const { return hash64( address ); }
    uint64_t by_state()const { return state; }
 };

 struct [[eosio::table]] issue_ts {
    uint64_t seq_num;
    name     to;
    asset    quantity;
    string   memo;

    uint64_t primary_key()const { return seq_num; }
 };

 struct [[eosio::table]] withdraw_ts {
    uint64_t             id;
    transaction_id_type  trx_id;
    name                 from;
    string               to;
    asset                quantity;
    uint64_t             state;  // 1 表示开始处理， 2 表示处理完成。如果此值大于等于10，表示未处理，是其初始值和此条记录的主键id相同。
    string               feedback_trx_id;
    string               feedback_msg;
    time_point_sec       feedback_time;

    uint64_t  primary_key()const { return id; }
    fixed_bytes<32> by_trxid()const { return fixed_bytes<32>(trx_id.hash); }
    uint64_t by_state()const { return state; }

    withdraw_ts(): feedback_time(current_time()){}
 };

 struct [[eosio::table]] account {
    asset    balance;

    uint64_t primary_key()const { return balance.symbol.code().raw(); }
 };

 struct [[eosio::table]] currency_stats {
    asset    supply;
    asset    max_supply;
    asset    large_asset;
    asset    min_withdraw;
    name     issuer;
    name     auditor;
    name     address_style;
    string   organization;
    string   website;
    string   miner_fee;
    string   service_fee;
    string   unified_recharge_address;
    bool     active;
    uint64_t issue_seq_num;

    uint64_t primary_key()const { return supply.symbol.code().raw(); }
 };
 

```


