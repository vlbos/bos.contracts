# bos.isssuance

## 1. 发行 issuance.cpp

##### 1.1. timechecking() 手动时间检查

每5分钟或5分钟 call 一次手动轮询，更新 singleton `_issuance.set(issuestate{xxx}, "bos.issuance"_n);`

***调试***: 可设置oneday参数为60 (一分钟)，300 (五分钟），或3600 (一小时）亦或 86400 (一天)

```
foreach_token
```

1-300期，在T+1天刚到时，UTC +8 00:00 am，call `calculatebid` 统计昨日竞买

当301期触发是，call `reward_token `开奖



##### 1.2 transfer()  自动代币检查 

自动检测 进来的币为BOS， ETH，EOS，BTC，否则无法转账

若为BOS则执行`refundtoken`，否则则执行`bidbos`操作



##### 1.3 MACRO EOSIO_DISPATCH_EX() apply 校验

检测code，和transfer()配额和

> 参考 [慢雾 apply校验](https://github.com/slowmist/eos-smart-contract-security-best-practices#apply-%E6%A0%A1%E9%AA%8C)



#####  1.4 其他函数

` get_default_global() ` 设置_issuance为 1

`foreach_token(day, option)` 

从`time_checking`中call，根据option，对三种币进行bidding统计，reward统计排序

`ACTION notify(name user, std::string msg)` 和`private void send_summary` 用于require_recipient

>  参考：[developer docs中的 INLINE Actions ](https://developers.eos.io/eosio-home/docs/inline-actions)



##### 1.5 setparam

可运行发售前设置参数



## 2. 竞买bosio.bidding

由transfer接收到用户正确的EOS, BTC, ETH之后触发

##### `bidbos` 

创建并更新

- 若无该用户记录，创建scope下 userbids table，emplace或modify `userbids`记录
- Emplace或modify `dailybids`，并添加`set<name> biddedusers`
- 如果客户不存在于_pioneeraccts, 则添加该客户进入

##### `calculatebid`

- 由timechecking 发现新的一期开始之后，通过foreach_token触发，统计前一期中的竞买情况
- 计算bid量，按照每0.5秒一个transaction的方式发送defertrx
- 对`set<name>biddeduser`里面触发sendbos()

##### `sendbos` 

- 发送bos
- emplace `userorders`和`dailyorders` table

  - dailyorders 



## 3. 回购 bos.refund

##### 3.1 `refundtoken` 

- 用户打入bos的量，返给用户相应的token，

- 返给用户的token的数目根据`avarefundpert`的返回值和`userorders`中masset/boasset的比率决定

- 同时modify `userorders` 和 `dailyorders` 两个table 




##### 3.2 `get_bought_period(day)` 和 `avarefundpert(uint64_t symday)`

- 对于一个特定天，计算available的refund的百分比



##### 3.3 validaterefundmemo

用于检查refund的memo是否正确



## 4. 分红 bos.dividends


每一期的第二回购期和第三回购期 返还给项目方`bos.foundation`

例如：在1-100期内投资

- 在T+91到T+180天内分红：【回售价格为0.5】

  - 减去在**第一个回购期已经回购的后**其中一半可分红：0.5*0.5=0.25

- 在T+181到T+270天内回售：【回售价格为0.25】

  - 减去在**第一和第二个回购期已经回购的后**其中一半可分红：0.25*0.5=0.125



```
calculatedividend(today, divdsymraw);
```

注意第270期到第300期的分红部分会在



## 5. 开奖 bos.reward


  360+30天后所得BOS.BTC， BOS.ETH, BOS.EOS
  - 25% 奖励给最贵一期客户，对BTC,ETH,EOS上链table做分别的统计
  - 25% 奖励平均分给每期客户
    奖励给大家
  #### 痛点：
  - 预估有几万个客户参与
  - 可能参与的客户的数量级为10000-100000级

#### 逻辑

supereward能确定



##### reward_token(symraw)

由timechecking

##### sendreward(receiver, token, symday)

发送奖励，并更改userorder table

##### superewardpert(supereward,rank)

获取该期superreward的百分比





## 6. TABLE和结构 bos.issuance.hpp

#### 6.1 singleton 用于激励天

`issuance_state_singleton _issuance`

用于记录当前时间为第几期

使用时，直接使用 `issuestate _istate `，使用模式模仿eosio.system里面的global_state_singleton



#### 6.2 multi_index symday作为主键 

#####  `dailybids`

用于记录每天某一个币种的当期的竞买token数目以及用户名的set，只记录当天结束即记录，不更新

```
 uint64_t symday;
 asset masset;
 std::set<name> biddedusers;
```

#####  `dailyorders`

用于记录关于当期的

每次分红需要修改dailyorders中的参数

使用symbol作为***secondary_index 二级索引***，为了在开奖reward_token(symraw)中**统计所有dailyorders**并

根据每期的剩余的竞买token进行排序

剩余的竞卖token:

> `masset`(dailyorders)= 初始竞买 `masset`(dailybids) - 回购部分 - 分红部分

```
uint64_t symday;
asset masset;
uint64_t symbol;
asset bosasset;
std::set<name> userset;
```

#####  `userbids`

用于记录某个用户每天某一个币种的当期的竞买token数目，用scope区分，只记录当天结束即记录，不更新

```
uint64_t symday; 
asset masset;   // 用户花掉的token数目，用于购买bos
```

##### `userorders`

```
uint64_t symday; 
asset masset;   // 用户花掉的token数目，用于购买bos
uint64_t symbol;
asset bosasset; // 用户手上的的bos
asset reward; // 初始值为0， 仅仅当reward_token
```



> 感谢 deadlock在table设计和massive micro transaction上的建议



### 6.3 constructor & table

在constructor，define 四个multi_index table

```
_issuance(_self, _self.value),
_dailyorders(_self, _self.value),
_dailybids(_self, _self.value),
_pioneeraccts(_self, _self.value)
```





## 7. 其他辅助函数 bos.helper.cpp



>  感谢成松

#### symday解耦和耦合生成主键
`index_encode()` 将两个uint64_t 变成一个uint64_t

讲uint64_t 的symbol_code和 day转换成uint64_t 作为主键

`index_decode()` 将一个uint64_t 变成两个uint64_t

和index_encoded相对应，将symday转换成代币类型



> 感谢韬懿

#### `uint_to_string(const uint64_t name) ` 将uint64_t 转换成string类型

打印出uint64_t类型的参数，

- 在给用户转账时标注memo指定token的类型
- `sendreward()`和`sendbos()`
- 用于debug

#### `MapSortOfValue`  

对dailyorders里面的entry进行排序

MapSortOfValue 用于在第301期时帮助给每天的dailyorders的entry排序（给定一个token类型）





## 8. 难点

#### 8.1 如何handle 每天结束时发送defer trx BOS给客户， 并保证成功

#### t.send(defer)  sendreward() 和 sendbos()

##### 方案一：无入表需求，使用pendxfer和compxfer表来记录操作

- `addtoq`将xfer加入到pendxfer_table

- `processq` 触发 `processxfer` 操作

- `processxfer`

  - 从pendxfer表中读取需要trsfer的entry
  - 执行INLINE_ACTION action(permission_level)

  - 加入到`compxfer` table中

  - 更新payload，并call defer trxs

  - 从pendxfer中删除


##### 方案二：有入表需求，for循环定时去call`sendbos`

#####  sendbos(receiver, masset, symday)

sendbos的功能包括发送INLINE_ACTION和

使用`pendxfer` 



#### 8.2 账户管理

简化设置

bos发币账户，peg发币账户(bbtc, beth, beos)账户和合约issuance账户均为同一个账户

```
static constexpr eosio::name bostoken_account{"bosissuances"_n};
static constexpr eosio::name pegtoken_account{"bosissuances"_n};
static constexpr eosio::name issuance_account{"bosissuances"_n};
```

dividend_account

```
static constexpr eosio::name dividend_account{"bos.fund"_n};
```



####  8.3 refund memo设置

`memo: {symbol: day: }`









## 9. 测试用例

#### 环境搭建

可用我们BOSCore团队的测试网，或自搭建测试网进行测试

脚本 https://github.com/EOSIO/eos/tree/master/tutorials/bios-boot-tutorial



### 普通测试



### eosio.tester模板测试

```
get_default_global() { //TODO：可否在hpp文件中指定
```







## 9. 安全

9.1 安全策略

- 回售的到账期很长，我们
- 托管业务一定要注意账务风险，若被盗
- 兑换期最好要人工审核，一定要和BOS系统DB对账，保证被偷的BOS.BTC无法被偷走，这样可以降低我们的风险
- 若账户公开透明，，强制回滚
- BOS.BTC的提款时间要有一天的间隔
- 7，8个小时反应一笔，持续监控账务监控
- BOS.BTC


9.2 模拟黑客
智能合约需要慢雾审计
极端方法：
- 减缓兑换速率 
- 可人工回滚