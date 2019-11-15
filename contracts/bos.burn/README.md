# 数据
## 获取快照空投账户名单

### msig json转换 csv
  
```
        go get github.com/jehiah/json2csv 
        json2csv -k user.name,remote_ip -i input.json -o airdrop_msig.csv 
```    

输出文件:
* airdrop_msig.csv

## 导出bos主网未激活账户及账户auth_sequence
主网支持命令URL:  https://api.boscore.io/v1/chain/get_unused_accounts 
```
curl localhost:8888/v1/chain/get_unused_accounts 
nodeos >> seq.log
```

输出文件：
* nonactivated_bos_accounts.txt 
* seq.log

## 执行脚本获得主网未激活空投账户

输入文件列表 dataset文件夹下
* airdrop_accounts.csv    空投账户         
* airdrop_msig.csv        空投多签账户
* nonactivated_bos_accounts.txt       主网未激活账户
* seq.log                 主网账户  auth_sequence值

[获取主网未激活空投账户python脚本文件](https://github.com/vlbos/bos.oracle-test/blob/master/oracle.testenv/test/airdropburn/unionset.py)
执行脚本
```
test/airdropburn/unionset.py
```
输出文件
* airdrop_unactive_account.csv
  
# 合约
## 升级部署系统合约，eosio.token合约
* 编译前指定指定燃烧token的执行账户和合约账户(合约账户当前是burn.bos,执行账户burnbos4unac可都是同一账户如合约账户)
## 部署燃烧token合约
[执行燃烧token合约命令脚本文件](https://github.com/vlbos/bos.oracle-test/blob/master/oracle.testenv/test/burn_test.sh)

执行命令
```
./burn.test.sh set
```

命令内容

```
${!cleos} set contract ${contract_burn} ${CONTRACTS_DIR}/${contract_burn_folder} -x 1000 -p ${contract_burn}
```

### 导入未激活空投账户

执行命令
```
./burn.test.sh ci
```

命令内容
```
flag=0
count=0
limits=1000
accs=''
file=./airdropburn/airdrop_unactive_account.csv
test_importaccs() {
    OLD_IFS=$IFS #保存原始值
    IFS="="
    cleos -u http://127.0.0.1:8888 push action ${contract_burn} importacnts '[['$accs']]' -p ${contract_burn}
    IFS=$OLD_IFS #还原IFS的原始值
}
test_csvimport() {
   
    OLD_IFS=$IFS #保存原始值
    IFS=","

    while read name quantity; do
        quantityx=$(echo $quantity | tr -d '\r')
        accs=$accs$(echo '["'$name'","'$quantityx'"]')
        count=$(($count + 1))
        if (($(($count % $limits)) == 0)); then
            test_importaccs
            accs=''
        else
            accs=$accs','
        fi
    done <$file

    IFS=$OLD_IFS #还原IFS的原始值

    if (($(($count % $limits)) > 0)); then
        accs=${accs%","}
        test_importaccs
    fi
}

```

### 设置指定执行账户如合约账户

执行命令
```
./burn.test.sh setp
```

命令内容
```
${cleos1} push action ${contract_burn} setparameter '[1,"'${contract_burn}'"]' -p ${contract_burn}
```

### 设置执行账户（与合约账户为同一账户）是eosio.code 权限给合约
执行命令
```
./burn.test.sh set
```

命令内容
```
${!cleos} set account permission ${contract_burn} active '{"threshold": 1,"keys": [{"key": "'${burn_c_pubkey}'","weight": 1}],"accounts": [{"permission":{"actor":"'${contract_burn}'","permission":"eosio.code"},"weight":1}]}' owner -p ${contract_burn}@owner
```

### 多签合约账户

```
cleos set contract oracle.bos bos.oracle/ -p oracle.bos  -s -j -d > setcontract.json
cleos multisig propose_trx setcontract bppermission.json  setcontract.json  -p bostesterter
cleos multisig approve bostesterter updatasystem '{"actor":"${BP_NAME}","permission":"active"}' -p ${BP_NAME}
cleos multisig exec bostesterter setcontract -p bostesterter@active
```
[详见多签文档](https://github.com/boscore/Documentation/blob/master/Oracle/BOS_Oracle_Deployment.md#22-create-msig)
### 执行从未激活空投账户到hole.bos账户转账空投部署tokens
执行命令
```
./burn.test.sh air
```

命令内容
```
test_transferairs() {
    OLD_IFS=$IFS #保存原始值
    IFS="="
    cleos -u http://127.0.0.1:8888 push action ${contract_burn} transferairs '[['$1']]' -p ${contract_burn}
    IFS=$OLD_IFS #还原IFS的原始值
}

test_csvtransferairs() {
    OLD_IFS=$IFS #保存原始值
    IFS=";"
    while read name quantity; do
        test_transferairs $name
    done <$file

    IFS=$OLD_IFS #还原IFS的原始值
}
```

### 执行燃烧token操作销毁hole.bos指定金额tokens
执行命令
```
./burn.test.sh burn
```

命令内容
```
${cleos1} push action ${contract_burn} burn '["46839967.5494 BOS"]' -p ${contract_burn}
```

### 结束后清除数据

执行命令
```
./burn.test.sh clear
```

命令内容
```
flag=0
count=0
limits=1000
accs=''
file=./airdropburn/airdrop_unactive_account.csv
test_arrclear() {
    OLD_IFS=$IFS #保存原始值
    IFS="="
    cleos -u http://127.0.0.1:8888 push action ${contract_burn} clear '[['$accs']]' -p ${contract_burn}
    IFS=$OLD_IFS #还原IFS的原始值
}
test_clearfromcsv() {
    OLD_IFS=$IFS #保存原始值
    IFS=","
    firstname=''
    while read name quantity; do
        accs=$accs'"'$name'"'
        count=$(($count + 1))
        if (($(($count % $limits)) == 0)); then
            test_arrclear
            accs=''
        else
            accs=$accs','
        fi
    done <$file

    IFS=$OLD_IFS #还原IFS的原始值

    if (($(($count % $limits)) > 0)); then
        accs=${accs%","}
        est_arrclear
    fi
}
```