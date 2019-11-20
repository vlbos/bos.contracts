# 数据
## 获取快照空投账户名单
  
```
     get clone  https://github.com/boscore/bos-airdrop-snapshots
```    
输出文件:
* accounts_info_bos_snapshot.airdrop.msig.json
* accounts_info_bos_snapshot.airdrop.normal.csv

## 导出bos主网未激活账户及msig账户

```
curl localhost:8888/v1/chain/get_unused_accounts 
或
curl -X POST --url http://127.0.0.1:8888/v1/chain/get_unused_accounts  -d '{
  "file_path": "/Users/lisheng/Downloads/nonactivated_bos_accounts.txt"
}'
```

输出文件：
* nonactivated_bos_accounts.txt
* nonactivated_bos_accounts.msig


## 执行脚本获得主网未激活空投账户

输入文件列表 dataset文件夹下
* accounts_info_bos_snapshot.airdrop.normal.csv    空投账户         
* accounts_info_bos_snapshot.airdrop.msig.json        空投多签账户
* nonactivated_bos_accounts.txt       主网未激活账户
* nonactivated_bos_accounts.msig                 主网未激活多签账户  

[获取主网未激活空投账户python脚本文件](https://github.com/vlbos/bos.oracle-test/blob/master/oracle.testenv/test/airdropburn/unionset.py)
执行脚本

```
python ./scripts/unionset.py
```
输出文件
* unactive_airdrop_accounts.csv

# 合约
## 升级部署系统合约，eosio.token合约
* 编译前指定指定燃烧token的执行账户和合约账户(合约账户当前是burn.bos,执行账户burnbos4unac可都是同一账户如合约账户)
## 部署燃烧token合约
[执行燃烧token合约命令脚本文件](https://github.com/vlbos/bos.oracle-test/blob/master/oracle.testenv/test/burn_test.sh)

执行命令
```
./burn_tests.sh set
```

命令内容

```
${cleos} set contract ${contract_burn} ${CONTRACTS_DIR}/${contract_burn_folder} -x 1000 -p ${contract_burn}
```

### 导入未激活空投账户

执行命令
```
./burn_testa.sh imp
```

### 设置指定执行账户如合约账户

执行命令
```
./burn_tests.sh setp
```

命令内容
```
${cleos1} push action ${contract_burn} setparameter '[1,"'${contract_burn}'"]' -p ${contract_burn}
```

### 设置执行账户（与合约账户为同一账户）是eosio.code 权限给合约
执行命令
```
./burn_tests.sh set
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
### 执行从未激活空投账户到hole.bos 账户转账空投部署tokens
执行命令
```
./burn_tests.sh air
```

### 检查执行结果，
执行命令
```
./burn_tests.sh chk
```
未成功重新执行上一步
```
mv ./unactive_airdrop_accounts.csv ./unactive_airdrop_accounts.csv.old
mv ./unactive_airdrop_accounts_result.csv ./unactive_airdrop_accounts.csv
./burn_tests.sh air
```

### 执行燃烧token操作销毁hole.bos指定金额tokens
执行命令
```
./burn_tests.sh burn
```

命令内容
```
${cleos1} push action ${contract_burn} burn '["46839967.5494 BOS"]' -p ${contract_burn}
```

### 结束后清除数据

执行命令
```
./burn_tests.sh clr
```
