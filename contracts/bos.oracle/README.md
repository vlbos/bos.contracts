

## 20190926 version

transfer 安全检查
###### 推送数据
增加不确定性数据写表
regservice action 去掉update_start_time 去掉declaration 内容合并到criteria;


###### 仲裁
仲裁结束，重新开始同一服务新仲裁时 抵押处理问题修复


## 20190906 version
###### 推送数据
一个提供者表对多个服务存储
update_number 检查验证
push_data超时验证 单元测试 

###### 仲裁
修复多人申诉问题
修复多人应诉问题
修复接受邀请超时，上传结果超时问题，增加相应单元测试
修改抵押金额超过服务提供者

## 20190911 version
###### 推送数据
一个提供者pushdata 数据不相同大于等于1/2问题
参数合法性检查
pushdata超时定时器优化

###### 仲裁
修复应诉超时问题
修复解抵押问题
修复申诉表role_type发起者不对问题
修复仲裁员收入计算错误问题

```
├── include  
│   └── bos.oracle 
│       ├── bos.constants.hpp 
│       ├── bos.functions.hpp  
│       ├── bos.oracle.hpp  
│       ├── bos.util.hpp  
│       ├── example  
│       │   └── consumer.contract.hpp  
│       ├── murmurhash.hpp  
│       └── tables  
│           ├── arbitration.hpp 
│           ├── consumer.hpp  
│           ├── oracle_api.hpp 
│           ├── provider.hpp 
│           └── riskcontrol.hpp 
└── src 
    ├── bos.arbitration.cpp 
    ├── bos.consumer.cpp 
    ├── bos.fee.cpp 
    ├── bos.oracle.cpp 
    ├── bos.provider.cpp 
    ├── bos.riskcontrol.cpp 
    ├── bos.util.cpp 
    └── example 
        └── consumer.contract.cpp 
```

```
主要文档
https://note.youdao.com/ynoteshare1/index.html?id=8af464f04688e9e56d4fa941523c66f6&type=note  设计说明 
https://github.com/vlbos/bos.oracle-test/blob/master/oracle.testenv/%E4%B8%80%E4%B8%AA%E7%AE%80%E5%8D%95%E7%9A%84bos%20oracle%20%E4%BD%BF%E7%94%A8%E6%B5%81%E7%A8%8B.pdf 一个简单的bos oracle 使用流程.pdf
https://shimo.im/folder/uuKYMwTgDGcH5e7m    其中初始设计 早期文档
https://shimo.im/docs/9hRgkXqrtqg3QGdc 使用文档
https://shimo.im/docs/JjPdChcjY9QWtpP9 部署文档

主要源文件

数据提供者https://github.com/boscore/bos.contracts/tree/bos.oracle/contracts/bos.oracle/src/bos.provider.cpp
数据使用者https://github.com/boscore/bos.contracts/tree/bos.oracle/contracts/bos.oracle/src/bos.consumer.cpp
仲裁https://github.com/boscore/bos.contracts/tree/bos.oracle/contracts/bos.oracle/src/bos.arbitration.cpp
风控https://github.com/boscore/bos.contracts/tree/bos.oracle/contracts/bos.oracle/src/bos.riskcontrol.cpp
计费统计https://github.com/boscore/bos.contracts/tree/bos.oracle/contracts/bos.oracle/src/bos.fee.cpp
****
```

oracle1.0 问题修复列表 

###### 推送数据

1. 恢复注册服务基础抵押金额参数, 注册服务实现创建服务功能, 提供者抵押金额成为相应服务提供者
2. 增加限制在系统里限制注册服务, 基础抵押金额不低于1000, 该服务提供者抵押不低于基础抵押金额
3. 推送数据验证 服务是否可用, 提供者是否注册
4. 修复问题
   
###### 仲裁

1. 申诉或再申诉修改参数is_provider为role_type 值: consumer(1),provider(2)
2. 申诉, 应诉, 接受邀请, 上传结果, 检查重复提交
3. 检查接受邀请是否受邀账户, 上传结果是否是接受邀请账户
4. 修复计算仲裁正确率问题
5. 修复计算仲裁结果, 罚没计算问题


arbistakeacc
arbiincomes

appealreq    scope  service_id*4+round
arbiresults  scope  arbitration_id*4+round




update_number 计算公式

c++ code
```
  uint32_t now_sec = bos_oracle::current_time_point_sec().sec_since_epoch();
   uint32_t update_start_time = service_itr->update_start_time.sec_since_epoch();
   uint32_t update_cycle = service_itr->update_cycle;
   uint32_t duration = service_itr->duration;
   uint32_t expected_update_number = (now_sec - update_start_time) / update_cycle + 1;
   uint32_t current_duration_begin_time = time_point_sec(update_start_time + (expected_update_number - 1) * update_cycle).sec_since_epoch();
   uint32_t current_duration_end_time = current_duration_begin_time + duration;
```

```
service_duration=200
update_cycle=300
update_start_time_date="2019-07-29"
update_start_time_time="15:27:33"
update_start_time=$update_start_time_date"T"$update_start_time_time".216857+00:00"

current_update_number=0
datetime1=$(date "+%s#%N")
datetime2=$(echo $datetime1 | cut -d"#" -f1) #取出秒

get_current_date() {
    datetime1=$(date "+%s#%N")
    datetime2=$(echo $datetime1 | cut -d"#" -f1) #取出秒
}

waitnext() {
    get_current_date
    i=$(($datetime2))
    while [ $i -le $1 ]; do
        get_current_date
        i=$datetime2
        sleep 10
    done
}

get_update_number() {
    a=$1
    b=$2
    if [ $a -eq 0 ]; then return 0; fi
    if [ $b -eq 0 ]; then return 0; fi

    get_current_date
    start_time=$(date -j -u -f "%Y-%m-%d %H:%M:%S" $update_start_time_date" "$update_start_time_time "+%s")
    diff_time=$(($datetime2 - $start_time))
    current_update_number=$(($diff_time / $a + 1))
    begin_time=$(($start_time + ($current_update_number - 1) * $a))
    end_time=$(($begin_time + $b))
    if [ $datetime2 -le $end_time ]; then return $current_update_number; fi
    next_begin_time=$(($begin_time + $a))
    waitnext $next_begin_time
    get_update_number $a $b

}

```

transfer memo format type

格式： ','隔开 
第一元素 类型  
具体类型如下
索引号以0开始  tc_service_stake = 0 ,以次递增
```
enum transfer_category : 
tc_service_stake  , 
tc_pay_service,
tc_deposit,
tc_arbitration_stake_appeal,
tc_arbitration_stake_arbitrator,
tc_arbitration_stake_resp_case,
tc_risk_guarantee};

具体类型 格式  最后一个参数数量不在memo出现 
数据提供者抵押，数据使用者支付充值
'类型=0或1，id'
# // index_category,index_id 
风控存款
'类型=2，转出账户，转入账户，是否通知，'
# //  deposit_category,deposit_from ,deposit_to,deposit_notify 

申诉
'类型=3，服务id，公示信息，证据，申诉原因,角色（1=consumer,2=provider)'
# //  appeal_category,index_id ,index_info,index_evidence,index_reason,role_type
注册仲裁员
'类型=4，仲裁员类型'
# // arbitrator_category,index_type 

应诉
'类型=5，服务id，仲裁轮次，证据'
# //  resp_case_category,index_id ,index_evidence

添加风险担保金
'类型=6，服务id，有效时长'
# //  risk_guarantee_category,index_id ,index_duration


    执行状态
   arbi_init = 1,
   arbi_choosing_arbitrator,
   arbi_wait_for_resp_appeal,
   arbi_wait_for_accept_arbitrate_invitation,
   arbi_wait_for_upload_result,
   arbi_wait_for_reappeal,
   arbi_resp_appeal_timeout_end,
   arbi_reappeal_timeout_end,
   arbi_public_end


   参数合法性检查
    check(provider_limit >= 3 && provider_limit <= 100, "provider_limit could not be less than 3 or greater than 100");
   check(update_cycle >= 60 && update_cycle <= 3600 * 24 * uint32_t(100), "update_cycle could not be less than 60 seconds or greater than 100 days");
   check(duration >= 30 && duration <= 3600, "duration could not be less than 30 or greater than 3600 seconds");
   check(duration < update_cycle - 10, "duration could not be  greater than update_cycle-10 seconds");
   check(injection_method == chain_indirect || injection_method == chain_direct || injection_method == chain_outside,
         "injection_method only set chain_indirect(0) or chain_direct(1)or chain_outside(2)");
   check(data_type == data_deterministic || data_type == data_non_deterministic, "data_type only set value data_deterministic(0) or data_non_deterministic(1)1");
   check(acceptance >= 3 && acceptance <= 100, "acceptance could not be less than 3 or greater than 100 ");
   check(data_format.size() <= 256, "data_format could not be greater than 256");
   check(criteria.size() <= 256, "criteria could not be greater than 256");
   check(declaration.size() <= 256, "declaration could not be greater than 256");

```

###### 1. 部署合约

```
./boracle_test.sh set

- test_set_contracts
- oracle.bos
- consumer.bos

contract_oracle=oraclebosbos
contract_oracle_folder=bos.oracle

contract_consumer=consumer1234
contract_consumer_folder=consumer.contract


  ${!cleos} set contract ${contract_oracle} ${CONTRACTS_DIR}/${contract_oracle_folder} -x 1000 -p ${contract_oracle}
 
    ${!cleos} set contract ${contract_consumer} ${CONTRACTS_DIR}/${contract_consumer_folder} -x 1000 -p ${contract_consumer}@active
  
```

##### 2. 注册服务

```
test_reg_service
 ${!cleos} push action ${contract_oracle} regservice '{ "account":"provider1111", "base_stake_amount":"1000.0000 BOS","data_format":"", "data_type":0, "criteria":"",
                          "acceptance":3, "injection_method":0, "duration":1,
                          "provider_limit":3, "update_cycle":1}' -p provider1111@active

```

##### 3. 初始化服务如费用

```
test_fee

    ${!cleos} push action ${contract_oracle} addfeetypes '{"service_id":"0","fee_types":[0,1],"service_prices":["1.0000 BOS","2.0000 BOS"] }' -p ${contract_oracle}@active

```

##### 4.抵押

```
transfer stake
stake unstake  eosio.code
  $cleos1 transfer provider1111 ${contract_oracle} "0.0001 BOS" "0,0" -p provider1111

```

##### 5. 订阅/请求

```
test_subs
  ${!cleos} push action ${contract_oracle} subscribe '{"service_id":"0", 
    "contract_account":"'${contract_consumer}'",  "publickey":"",
                          "account":"consumer1111", "amount":"10.0000 BOS", "memo":""}' -p consumer1111@active
}
test_req

  ${!cleos} push action ${contract_oracle} requestdata '{"service_id":0,  "contract_account":"'${contract_consumer}'", 
                         "requester":"consumer1111", "request_content":"eth usd"}' -p consumer1111@active

```

##### 6. 支付服务费用

```
transfer pay
    $cleos1 transfer consumer2222 ${contract_oracle} "0.0001 BOS" "1,0" -p consumer2222

```

##### 7.推送

```
"mpush") test_multipush c1 "$2" ;;
  # ${!cleos}  set account permission provider1111  active '{"threshold": 1,"keys": [{"key": "'${provider1111_pubkey}'","weight": 1}],"accounts": [{"permission":{"actor":"'${contract_oracle}'","permission":"eosio.code"},"weight":1}]}' owner -p provider1111@owner
    # sleep .2
    ${!cleos} push action ${contract_oracle} multipush '{"service_id":0, "provider":"provider1111",  "data_json":"test multipush data json","is_request":'${reqflag}'}' -p provider1111

"push") test_push c1 ;;
    ${!cleos} push action ${contract_oracle} pushdata '{"service_id":0, "provider":"provider1111", "contract_account":"'${contract_consumer}'",  "request_id":0, "data_json":"test data json"}' -p provider1111

"pushr") test_pushforreq c1 "$2" ;;
    ${!cleos} push action ${contract_oracle} pushdata '{"service_id":0, "provider":"provider1111", "contract_account":"'${contract_consumer}'",  "request_id":'"$2"', "data_json":"test data json"}' -p provider1111

${!cleos} push action ${contract_oracle} oraclepush '{"service_id":1, "provider":"'${p}'",  "request_id":0, "data_json":"auto publish test data json"}' -p ${p}

```

#### 风控

##### 1. 存入金额

```
"deposit") test_deposit c1 ;; consumer()->dapp(data consumer) save

    $cleos1 transfer consumer2222 ${contract_oracle} "0.0001 BOS" "2,consumer2222,consumer1111,0" -p consumer2222
   
```

##### 2. 提取金额 

```
"withdraw") test_withdraw c1 ;; dapp(data consumer) -> consumer()

  ${!cleos} push action ${contract_oracle} withdraw '{"service_id":0,  "from":"consumer1111", "to":"oraclize1111",
                         "quantity":"1.0000 BOS", "memo":""}' -p ${contract_oracle}@active
}
```


#### 仲裁
##### 前提条件  
注册服务

 #####  注册仲裁员（抵押）
专业，大众

```
./boracle_test.sh arbi rega

cleos push action $EOS_ORACLE regarbitrat '["arbitrator11", "EOS7UCx8GSeEHC4XE8jQ1R5WJqw5Vp2vZqWgQx94obFVbebnYg6eq", 1, "1.0000 BOS", "hello world"]' -p arbitrator11@active

    for i in {1..5}; do
        for j in {1..5}; do
            account='arbitrator'${i}${j}
            ${!cleos}  transfer ${account} ${contract_oracle} "10000.0000 BOS" "4,1" -p ${account}
            sleep 1
        done
    done
${!cleos} get table ${contract_oracle} ${contract_oracle} arbitrators


```

 #####  申诉（抵押）仲裁开始

```

    #appeal   role_type 1 consume 2 provider
    ${!cleos} transfer  appellants11 ${contract_oracle} "200.0000 BOS" "3,1,'evidence','info','reason',1" -p appellants11


```

 ##### 上传证据

```  
cleos push action $EOS_ORACLE uploadeviden '["appellant1", 0, "evidence"]' - p appellant1 @active
```

 #####  应诉（抵押）

```
    #resp_case
    ${!cleos}  transfer provider1111 ${contract_oracle} "200.0000 BOS" "5,1,''" -p provider1111
}

```

 #####  接受仲裁邀请

```
  ${!cleos} push action ${contract_oracle} acceptarbi '["'$account'", 1]' -p $account@active 
```

#####  上仲裁结果

```
 ${!cleos} push action ${contract_oracle} uploadresult '["'${account}'", 1, 1,"comment:"'${account}']' -p ${account}@active
当前仲裁结果得出   通知transfer memo 再申诉等待
```

##### 再申诉（抵押）

```
  ${!cleos} transfer  appellants11 ${contract_oracle} "400.0000 BOS" "3,1,'evidence','info','reason',1" -p appellants11
```

#####  再应诉（抵押）

```
  ${!cleos}  transfer provider1111 ${contract_oracle} "400.0000 BOS" "5,1,''" -p provider1111
```

#####  解抵押
  void unstakearbi(uint64_t arbitration_id, name account, asset amount, std::string memo);
```  
cleos push action $EOS_ORACLE unstakearbi '[1,"appellant1",  "400.0000 BOS",'']' - p appellant1 @active
```

#####   领取仲裁收益
    void claimarbi(name account, name receive_account);

```  
cleos push action $EOS_ORACLE claimarbi '["appellant1","appellant1"]' - p appellant1 @active
```

# 1. 部署合约 


    ${!cleos} set contract ${contract_oracle} ${CONTRACTS_DIR}/${contract_oracle_folder} -x 1000 -p ${contract_oracle}
   
    ${!cleos} set contract ${contract_consumer} ${CONTRACTS_DIR}/${contract_consumer_folder} -x 1000 -p ${contract_consumer}@active


1. 注册预言机数据服务接口

| 中文接口名       | 注册预言机数据服务接口                 |                            |          |                                                                                                                         |
| :--------------- | :------------------------------------- | :------------------------- | :------- | :---------------------------------------------------------------------------------------------------------------------- |
| 英文接口名       | Register oracle Data Service Interface |                            |          |                                                                                                                         |
| 定义接口名       | regservice                             |                            |          |                                                                                                                         |
| 接口功能描述     | 注册oracle数据服务                     |                            |          |                                                                                                                         |
| 中文参数名       | 英文参数名                             | 参数定义                   | 参数类型 | 参数描述                                                                                                                |
| 数据服务ID       | Data Service ID                        | uint64_t service_id        | 整型     | 注册id：int 从1开始自增，用于表示一类预言机服务                                                                         |
| 数据格式         | data_format                            | std::string data_format    | 字符串   | data_format：提前约定数据展现形式，比如：Jianeng2Hongyang:100                                                           |
| 数据类型         | Data Type                              | uint8_t data_type          | 整型     | (确定性/非确定性) type：是指数据提者提供的数据是否允许差异                                                              |
| 准则             | criteria                               | std::string criteria       | 字符串   | （出现时评判准则）  备注类型                                                                                            |
| 接受方式         | accept_mode                            | uint64_t  acceptance       | 整型     | 数据接受规则  比例/人数                                                                                                 |
| 数据注入方式     | Data injection method                  | uint64_t injection _method | 整型     | 数据注入方式    链上直接，链接间接（over oracle），链外                                                                 |
| 基础抵押金额     | basic_mortgage_amount                  | uint64_t amount            | 整型     | 基础抵押金额                                                                                                            |
| 数据收集持续时间 | Data Collection Duration               | uint32_t duration          | 整型     | 数据收集持续时间（从第一个数据提供者注入数据算起，多久后不再接受同一project_id ^update_number 的数据）duration 单位：秒 |
| 数据提供者下限   | Data Provider Limit                    | uint8_t provider_limit     | 整型     | 数据提供者下限（大于3） data_provider_min_number                                                                        |
| 数据更新周期     | Data Update Cycle                      | uint32_t update_cycle      | 整型     | 数据更新周期 单位：秒                                                                                                   |


```
  ${!cleos} push action ${contract_oracle} regservice '{ "account":"provider1111", "base_stake_amount":"1000.0000 BOS",  "data_format":"", "data_type":0, "criteria":"",
                          "acceptance":3,  "injection_method":0, "duration":1,
                          "provider_limit":3, "update_cycle":1}' -p provider1111@active
```


2. 注销数据服务接口

|   中文接口名   |              注销数据服务接口               |                       |          |               |
| :------------: | :-----------------------------------------: | :-------------------- | :------- | :------------ |
|   英文接口名   | unregister Data service provision interface |                       |          |               |
|   定义接口名   |                unregservice                 |                       |          |               |
|  接口功能描述  |    定义提供数据服务与数据服务提供者 关系    |                       |          |               |
|   中文参数名   |                 英文参数名                  | 参数定义              | 参数类型 | 参数描述      |
|   数据服务ID   |               Data Service ID               | uint64_t service _id  | 整型     |               |
| 数据提供者签名 |           Data provider signature           | std::string signature | 字符串   |               |
| 数据提供者账户 |            Data Provider Account            | name account          | 整型     |               |
|  是否临时注销  |              Stop Service Flag              | uint64_t is_suspense  | 整型     | 为1为临时注销 |


3. 数据服务操作接口

|  中文接口名  | 数据服务操作接口                 |                            |          |                |
| :----------: | :------------------------------- | :------------------------- | :------- | :------------- |
|  英文接口名  | Data  service action interface   |                            |          |                |
|  定义接口名  | servaction                       |                            |          |                |
| 接口功能描述 | 定义数据服务操作如冻结，紧急处理 |                            |          |                |
|  中文参数名  | 英文参数名                       | 参数定义                   | 参数类型 | 参数描述       |
|  数据服务ID  | Data Service ID                  | uint64_t service _id       | 整型     |                |
|   操作类型   | operation type                   | std::string operation_type | 整型     | 1 冻结  2 紧急 |


4. 增减数据提供者抵押金额接口

|   中文接口名   | 增减数据提供者抵押金额接口                                     |                              |          |          |
| :------------: | :------------------------------------------------------------- | :--------------------------- | :------- | :------- |
|   英文接口名   | Data provider  stake amount  of increase or decrease interface |                              |          |          |
|   定义接口名   | stakeamount                                                    |                              |          |          |
|  接口功能描述  | 数据提供者增减抵押金额接口                                     |                              |          |          |
|   中文参数名   | 英文参数名                                                     | 参数定义                     | 参数类型 | 参数描述 |
|   数据服务ID   | Data Service ID                                                | uint64_t service _id         | 整型     |          |
|  数据提供者ID  | Data Provider ID                                               | uint64_t provider _id        | 整型     |          |
| 数据提供者账户 | Data Provider Account                                          | name account                 | 整型     |          |
| 数据提供者签名 | Data Provider signature                                        | std::string signature        | 字符串   |          |
|    抵押金额    | Total Mortgage Amount                                          | uint64_t total _stake_amount | 整型     |          |


5. 订阅数据服务接口

|    中文接口名     | 订阅数据服务接口              |                       |          |          |
| :---------------: | :---------------------------- | :-------------------- | :------- | :------- |
|    英文接口名     | Data Service Usage Interface  |                       |          |          |
|    定义接口名     | subscribeserv                 |                       |          |          |
|   接口功能描述    | 定义数据使用者订阅数据服务    |                       |          |          |
|    中文参数名     | 英文参数名                    | 参数定义              | 参数类型 | 参数描述 |
|    数据服务ID     | Data Service ID               | uint64_t service _id  | 整型     |          |
| 接收数据合约账户  | Receive Data Contract Account | name contract_account | 整型     |          |
| 接收数据acion名称 | Receive data acion name       | name action_name      | 整型     |          |
|     转账账户      | Transfer account              | uint64_t account      | 整型     |          |
|     充值金额      | deposit amount                | uint64_t amount       | 整型     |          |

    ${!cleos} push action ${contract_oracle} subscribe '{"service_id":"0", 
    "contract_account":"'${contract_consumer}'", 
                          "account":"consumer1111", "amount":"10.0000 BOS", "memo":""}' -p consumer1111@active

6. 请求服务数据接口

|   中文接口名   | 请求服务数据接口                     |                          |          |          |
| :------------: | :----------------------------------- | :----------------------- | :------- | :------- |
|   英文接口名   | Request Service Data   Interface     |                          |          |          |
|   定义接口名   | reqservdata                          |                          |          |          |
|  接口功能描述  | 定义数据使用者主动请求数据服务的接口 |                          |          |          |
|   中文参数名   | 英文参数名                           | 参数定义                 | 参数类型 | 参数描述 |
| 请求更新序列号 | update_number                        | uint64_t update _number  | 整型     |          |
| 请求数据服务ID | Request Data Service ID              | uint64_t service _id     | 整型     |          |
|    请求签名    | Request Signature                    | name request_signature   | 字符串   |          |
|    请求内容    | Request Content                      | uint64_t request_content | 字符串   | 定义规则 |


    ${!cleos} push action ${contract_oracle} requestdata '{"service_id":0,  "contract_account":"'${contract_consumer}'", 
                         "requester":"consumer1111", "request_content":"eth usd"}' -p consumer1111@active


7. 推送服务数据接口  

|   中文接口名   |                          推送服务数据接口                          |                              |          |          |
| :------------: | :----------------------------------------------------------------: | :--------------------------- | :------- | :------- |
|   英文接口名   |                   push Service Data   Interface                    |                              |          |          |
|   定义接口名   |                            pushservdata                            |                              |          |          |
|  接口功能描述  | 定义推送服务数据，包括直接推送数据使用者，间接通过oracle合约推送。 |                              |          |          |
|   中文参数名   |                             英文参数名                             | 参数定义                     | 参数类型 | 参数描述 |
|   数据服务ID   |                          Data Service ID                           | uint64_t service _id         | 整型     |          |
| 数据更新序列号 |                     Data Update Serial Number                      | uint64_t update_number       | 整型     |          |
|  具体数据json  |                         Specific data json                         | uint64_t data_json           | 字符串   |          |
| 数据提供者签名 |                      Data Provider Signature                       | uint64_t provider _signature | 字符串   |          |
| 数据服务请求ID |                      Data Service Request ID                       | uint64_t request_id          | 整型     |          |

  ${!cleos} push action ${contract_oracle} pushdata '{"service_id":0, "provider":"provider1111", "contract_account":"'${contract_consumer}'", 
                         "request_id":'"$2"', "data_json":"test data json"}' -p provider1111

  # ${!cleos}  set account permission provider1111  active '{"threshold": 1,"keys": [{"key": "'${provider1111_pubkey}'","weight": 1}],"accounts": [{"permission":{"actor":"'${contract_oracle}'","permission":"eosio.code"},"weight":1}]}' owner -p provider1111@owner


    # sleep .2
    ${!cleos} push action ${contract_oracle} multipush '{"service_id":0, "provider":"provider1111", 
                          "data_json":"test multipush data json","is_request":'${reqflag}'}' -p provider1111

  reqflag=false && if [ "$2" != "" ]; then reqflag="$2"; fi

    echo ===multipush
    # ${!cleos}  set account permission provider1111  active '{"threshold": 1,"keys": [{"key": "'${provider1111_pubkey}'","weight": 1}],"accounts": [{"permission":{"actor":"'${contract_oracle}'","permission":"eosio.code"},"weight":1}]}' owner -p provider1111@owner

    # sleep .2
    ${!cleos} push action ${contract_oracle} multipush '{"service_id":0, "provider":"provider1111", 
                          "data_json":"test multipush data json","is_request":'${reqflag}'}' -p provider1111

# 2.  
8. 申诉接口

|    中文接口名    | 申诉接口                                       |                         |          |                              |
| :--------------: | :--------------------------------------------- | :---------------------- | :------- | :--------------------------- |
|    英文接口名    | Complain Interface                             |                         |          |                              |
|    定义接口名    | appeal                                         |                         |          |                              |
|   接口功能描述   | 定义数据服务使用者对提供数据质疑时提出申诉接口 |                         |          |                              |
|    中文参数名    | 英文参数名                                     | 参数定义                | 参数类型 | 参数描述                     |
|    申诉者签名    | Appellant                                      | name appellant         | 整型     |                              |
|    数据服务ID    | Data Service ID                                | uint64_t service_id     | 整型     |                              |
|   申诉抵押金额   | stake amount                                   | asset amount            | 整型     |                              |
|     申诉原因     | Reason for appeal                              | std::string reason      | 字符串   |                              |
| ~~申诉者发起人~~ | ~~Sponsors~~                                   | ~~bool is_~~~~sponsor~~ | ~~布尔~~ |                              |
|     仲裁方式     | Arbitration                                    | uint8_t arbi_method     | 整型     | 仲裁方式  大众仲裁，多轮仲裁 |

# 3.  
9. 应诉接口  

| 中文接口名   | 应诉接口                          |                         |          |          |
| :----------- | :-------------------------------- | :---------------------- | :------- | :------- |
| 英文接口名   | response to arbitration Interface |                         |          |          |
| 定义接口名   | acceptarbi                        |                         |          |          |
| 接口功能描述 | 应诉仲裁案件                      |                         |          |          |
| 中文参数名   | 英文参数名                        | 参数定义                | 参数类型 | 参数描述 |
| 应诉者       | arbitrator                        | name arbitrator         | name     |          |
| 仲裁项ID     | Arbitration ID                    | uint64_t arbitration_id | 整型     |          |

10. 注册仲裁员接口   

|   中文接口名    |                   仲裁员接口                   |                         |          |                               |
| :-------------: | :--------------------------------------------: | :---------------------- | :------- | :---------------------------- |
|   英文接口名    |         register Arbitrators Interface         |                         |          |                               |
|   定义接口名    |                  regarbitrat                   |                         |          |                               |
|  接口功能描述   | 定义注册仲裁员接口，包括职业仲裁员，大众仲裁员 |                         |          |                               |
|   中文参数名    |                   英文参数名                   | 参数定义                | 参数类型 | 参数描述                      |
|   仲裁员账户    |               Arbitrator Account               | name account            | 整型     |                               |
|   仲裁员类型    |                Arbitrator type                 | uint8_t type            | 整型     | 1 - 职业仲裁员，2 -大众仲裁员 |
| 仲裁员抵押金额  |           Arbitrator Mortgage Amount           | asset amount            | 整型     |                               |
| 仲裁员公示信息  |         Arbitrator Public Information          | std::string public_info | 字符串   |                               |


11. 仲裁员应答接口   

|  中文接口名  | 仲裁员应答接口                |                       |          |          |
| :----------: | :---------------------------- | :-------------------- | :------- | :------- |
|  英文接口名  | Arbitrator response Interface |                       |          |          |
|  定义接口名  | acceptarbi                      |                       |          |          |
| 接口功能描述 | 定义仲裁员接受邀请仲裁案件    |                       |          |          |
|  中文参数名  | 英文参数名                    | 参数定义              | 参数类型 | 参数描述 |
|    仲裁员    | Arbitrator Name               | name arbitrator       | name     |          |
|   仲裁项ID   | Arbitration ID                | uint64_t arbitrate_id | 整型     |          |


12. 上传仲裁结果接口  

|  中文接口名  | 上传仲裁结果接口                     |                         |          |          |
| :----------: | :----------------------------------- | :---------------------- | :------- | :------- |
|  英文接口名  | Upload Arbitration results interface |                         |          |          |
|  定义接口名  | uploadresult                         |                         |          |          |
| 接口功能描述 | 定义仲裁人上传仲裁结果接口           |                         |          |          |
|  中文参数名  | 英文参数名                           | 参数定义                | 参数类型 | 参数描述 |
|  仲裁员名称  | Arbitrator Name                      | name  arbitrator        | 整型     |          |
|   仲裁项ID   | Arbitration ID                       | uint64_t arbitration_id | 整型     |          |
|   仲裁结果   | Arbitration Results                  | uint8_t result          | 整型     |          |
| 仲裁员评述  |         Arbitrator comment           | std::string comment | 字符串   |    

# 4.  
13. 上传证据接口

|  中文接口名  | 上传证据接口              |                         |          |                           |
| :----------: | :------------------------ | :---------------------- | :------- | :------------------------ |
|  英文接口名  | upload evidence Interface |                         |          |                           |
|  定义接口名  | uploadeviden              |                         |          |                           |
| 接口功能描述 | 上传仲裁案件相关证据      |                         |          |                           |
|  中文参数名  | 英文参数名                | 参数定义                | 参数类型 | 参数描述                  |
|   仲裁项ID   | Arbitration ID            | uint64_t arbitration_id | 整型     |                           |
|  仲裁员证据  | Arbitrator Evidence       | std::string evidence    | 字符串   | 仲裁员证据  ipfs hash链接 |

14. 转账风险担保金接口 

|  中文接口名  | 转账风险担保金接口                       |                    |          |                                  |
| :----------: | :--------------------------------------- | :----------------- | :------- | :------------------------------- |
|  英文接口名  | Transfer risk guarantee amount interface |                    |          |                                  |
|  定义接口名  | transferrisk                             |                    |          |                                  |
| 接口功能描述 | 转账风险担保金                           |                    |          |                                  |
|  中文参数名  | 英文参数名                               | 参数定义           | 参数类型 | 参数描述                         |
|    发送者    | sender                                   | name sender        | 整型     |                                  |
|    接收者    | receiver                                 | name receiver      | 整型     |                                  |
|     金额     | amount                                   | uint64_t amount    | 整型     |                                  |
|   转账方向   | transfer direction                       | uint64_t direction | 整型     | 1 转入预言机合约2 转出预言机合约 |

15. 领取数据服务收入接口 

|  中文接口名  | 领取数据服务收入接口           |                      |          |          |
| :----------: | :----------------------------- | :------------------- | :------- | :------- |
|  英文接口名  | claim reward                   |                      |          |          |
|  定义接口名  | claimreward                    |                      |          |          |
| 接口功能描述 | 数据服务提供者领取数据服务收入 |                      |          |          |
|  中文参数名  | 英文参数名                     | 参数定义             | 参数类型 | 参数描述 |
|  数据服务ID  | Data Service ID                | uint64_t service _id | 整型     |          |
|   发放账户   | issuance of accounts           | name account         | 整型     |          |

16. 添加数据服务风险担保接口

|  中文接口名  | 添加数据服务风险担保接口                  |                       |          |          |
| :----------: | :---------------------------------------- | :-------------------- | :------- | :------- |
|  英文接口名  | Add data serivce Risk Guarantee Interface |                       |          |          |
|  定义接口名  | addriskguara                              |                       |          |          |
| 接口功能描述 | 定义添加数据服务风险担保接口。            |                       |          |          |
|  中文参数名  | 英文参数名                                | 参数定义              | 参数类型 | 参数描述 |
|  数据服务ID  | Data Service ID                           | uint64_t service _id  | 整型     |          |
|  风险担保ID  | Risk Guarantee ID                         | uint64_t risk _id     | 整型     |          |
| 风险担保账户 | Risk Guarantee Account                    | name account          | 整型     |          |
| 风险担保金额 | Risk Guarantee Amount                     | name amount           | 整型     |          |
| 风险担保时长 | Risk Guarantee Duration                   | uint32_t duration     | 整型     |          |
| 风险担保签名 | Risk Guarantee Signature                  | std::string signature | 字符串   |          |

17. 链外握手协议接口(不实现）

|     中文接口名      |                                         链外握手协议                                         |                          |          |          |
| :-----------------: | :------------------------------------------------------------------------------------------: | :----------------------- | :------- | :------- |
|     英文接口名      |                                 handshakeoffchain Interface                                  |                          |          |          |
|     定义接口名      |                                          handshake                                           |                          |          |          |
|    接口功能描述     | 定义数据服务使用者与数据服务提供者链外交互协议，不实现，另有单独接口提供验证双方身份合法性。 |                          |          |          |
|     中文参数名      |                                          英文参数名                                          | 参数定义                 | 参数类型 | 参数描述 |
| 数据提供者publickey |                                          publickey                                           | uint64_t publickey       | 整型     |          |
|        hash         |                                             hash                                             | uint64_t hash            | 字符串   |          |
|   数据提供者签名    |                                     Data User Signature                                      | uint64_t user _signature | 字符串   |          |
|     数据服务ID      |                                   Data Service Request ID                                    | uint64_t request_id      | 整型     |          |
|      数据 hash      |                                          data hash                                           | uint64_t data_hash       | 字符串   |          |


# 5. 疑问
### 5.0.1. 申诉接口: appeal
初次申诉时, `arbitratcase`表, `update_number` 更新逻辑
答：`arbitratcase`表, `update_number` 更新逻辑，暂时没有了。表设计时申诉考虑具体到哪条数据。上次讨论具体到服务ID。
### 5.0.2. 应诉接口: acceptarbi
应诉接口签名参数有什么用？签名验证的内容是什么? 应诉逻辑? 修改arbitratcase哪几个字段?
答：签名为出现争议使用，链就行了，action验证有效格式就行了。应诉，转账抵押金额。更新arbitratcase，arbi_step为应诉状态，触发仲裁启动。判断仲裁方式，执行仲裁流程逻辑。
### 5.0.3. 上传证据: uploadeviden
签名参数有什么用? 签名验证什么内容? 
答：现在实现到验证签名是有效格式就行了，设计考虑如果有争议，发生抵赖或冒名等问题，保存证据。


