# BOS 未激活空投账户燃烧方案


## 流程
1. 生成待燃烧账户列表
2. 将待燃烧账户列表导入 `burn.bos` 合约
3. 开放社区验证燃烧账户、合约代码
4. 升级燃烧版本的 `eosio.system`、`eosio.token` 合约
5. 执行燃烧
6. 收尾


# 生成待燃烧账户列表

## 获取快照空投账户名单
  
将空投文件放到 `dataset` 目录

```
mkdir dataset && cd dataset
git clone  https://github.com/boscore/bos-airdrop-snapshots
cp ./bos-airdrop-snapshots/accounts_info_bos_snapshot.airdrop.msig.json .
cp ./bos-airdrop-snapshots/accounts_info_bos_snapshot.airdrop.normal.csv .
```    

使用 [bos.burn](https://github.com/boscore/bos/tree/bos.burn) 分支的 `nodeos` 导出 `auth_sequence=0||auth_sequence=2` 的账户；
燃烧截止的时间点是 `2019-11-27 03:26:27 UTC-0`，对应块高度为 `54,172,308`，可以使用此时间之前的 `snapshot` 来启动 `bos.burn` 的节点，并指定 ` --netruncate-at-block=54172308`。

比如，docker-compose.yml 可以为：
```
version: "3"

services:
  burnnode:
    image: boscore/burnbos:v1.0.2
    command: /opt/eosio/bin/nodeosd.sh --data-dir /opt/eosio/bin/data-dir --max-irreversible-block-age=5000000 --max-transaction-time=100000 --wasm-runtime wabt --netruncate-at-block=321899 --snapshot /opt/eosio/bin/data-dir/snapshots/snapshot-2019112702.bin
    hostname: burnnode
    ports:
      - 280:80
      - 643:443
    environment:
      - NODEOSPORT=80
      - WALLETPORT=8888
    volumes:
      - /data/bos/fullnode:/opt/eosio/bin/data-dir
```

注：`bos.burn` 分支对应的 Docker Image 为：boscore/burnbos:v1.0.2

可以通过 `get_info` API 来检查 `head` 是否停止到指定高度：

```
curl 127.0.0.1:280/v1/chain/get_info
```

调用以下接口生成中间文件：

```
curl 127.0.0.1:280/v1/chain/get_unused_accounts 
#OR
curl -X POST --url http://127.0.0.1:8888/v1/chain/get_unused_accounts  -d '{
  "file_path": "/xxx/xxx/nonactivated_bos_accounts.txt"  
}'
```

输出文件：
* nonactivated_bos_accounts.txt
* nonactivated_bos_accounts.msig

将上面的输出文件放到 `dataset` 目录下，将 [unionset.py](https://github.com/boscore/bos.contracts/blob/bos.burn/contracts/bos.burn/scripts/unionset.py) 放至 `dataset` 同一目录下，然后执行：

```
python3 unionset.py
```

输出文件
* unactive_airdrop_accounts.csv


# 燃烧

## 创建燃烧账户

燃烧合约将会部署在 `burn.bos` 上面，启动以后任何账户都可以触发燃烧。

具有燃烧功能的合约代码：[bos.contracts/bos.burn](https://github.com/boscore/bos.contracts/tree/bos.burn/contracts/bos.burn)  
对应的编译版本：[bos.contract-prebuild/bos.burn](https://github.com/boscore/bos.contract-prebuild/tree/bos.burn)

## 部署合约

### 升级 burn.bos 合约

```
CONTRACTS_FOLDER='./bos.contract-prebuild' 
cleos set contract burn.bos ${CONTRACTS_FOLDER}/bos.burn -p burn.bos
```

### burn.bos 导入未激活账户

可以使用 [burntool.sh](https://github.com/boscore/bos.contracts/blob/bos.burn/contracts/bos.burn/scripts/burntool.sh) 来进行导入。

需要调整 `burntool.sh` 里面的变量：`http_endpoint`，将 `burntool.sh` 和 待燃烧账户文件 `unactive_airdrop_accounts.csv` 位于同一目录下，然后执行：

```
bash burntool.sh imp
```

_注意：在执行导入之前，请确保 `burn.bos` 资源充足：_
* RAM，整个未激活账户在50万以内，需要购买 160M RAM；
* CPU 40s；
* NET 16MB；
* 整个导入过程应该在 30 分钟以内完成

### 进行社区公示3天

将 `burn.bos` 权限更改为 `eosio`，并启用 `eosio.code` 权限，等待社区进行验证：

```
# resign active
cleos set account permission burn.bos active '{"threshold": 1,"keys": [],"accounts": [{"permission":{"actor":"burn.bos","permission":"eosio.code"},"weight":1},{"permission":{"actor":"eosio","permission":"active"},"weight":1}]}' owner -p burn.bos@owner
# resign owner
cleos set account permission burn.bos owner '{"threshold": 1,"keys": [],"accounts": [{"permission":{"actor":"eosio","permission":"owner"},"weight":1}]}' -p burn.bos@owner
```

社区成员自己生成 `unactive_airdrop_accounts.csv` 可以通过以下命令来与 `burn.bos` 导入数据进行对比：
 `unactive_airdrop_accounts.csv` 放到脚本同一目录下

```
python3 vmp.py
```

输出文件(若合约不存在账户或金额不一致会输出no.csv文件)
* no.csv

### 升级 eosio.system 

经过社区共识没有异议以后，需要使用编译版本 [bos.contract-prebuild/bos.burn](https://github.com/boscore/bos.contract-prebuild/tree/bos.burn) 中的 `eosio.system` 和 `eosio.token` 合约进行升级。本次以 `eosio.system` 升级为例。

准备 top 30 的 `bp.json` ：
```
[
    {"actor":"bponeoneonee","permission":"active"},
    ....
]
```

发起多签升级 `eosio` 合约：

```
CONTRACTS_FOLDER='./bos.contract-prebuild' 

```

同时发起 `eosio.system` 和 `eosio.token` 的多签升级，等待 BP 多签通过。

### 开启 `burn.bos` 燃烧

```
# set burning account: burn.bos
# burnbosooooo is a common acount
cleos multisig propose enablebrun bp.json '[{"actor": "burn.bos", "permission": "active"}]' burn.bos setparameter '{"version":1,"executer":"burn.bos"}' burnbosooooo 336 -p  burnbosooooo@active

# review proposal
cleos multisig review burnbosooooo enablebrun

# approve proposal
cleos multisig approve burnbosooooo enablebrun  '{"actor":"bponeoneonee","permission":"active"}' -p bponeoneonee@active 

# exec proposal
cleos multisig exec burnbosooooo enablebrun -p burnbosooooo@active
```

设置完燃烧账户以后，就可以进行燃烧：

```
bash burntool.sh air
```

_注意：在执行导入之前，请确保 `burn.bos` 资源充足：_
* RAM，需要购买到 180M RAM；
* CPU 10min；
* NET 200MB；
* 整个导入过程应该在3个小时以内完成

### 检查燃烧结果

```
bash burntool.sh chk
```

燃烧失败的账户会存放到 `unactive_airdrop_accounts_result.csv`，可以针对这部分账户再次发起燃烧，直到全部成功：

```
mv ./unactive_airdrop_accounts.csv ./unactive_airdrop_accounts.csv.old
mv ./unactive_airdrop_accounts_result.csv ./unactive_airdrop_accounts.csv
bash burntool.sh air
```

### 销毁 hole.bos 余额

获取 `hole.bos` 的余额，比如 40000.0000 BOS，修改 `burntool.sh` 中 `burn_total_quantity` 值：

```
burn_total_quantity="40000.0000 BOS"
```
然后执行：
```
bash burntool.sh burn
```

# 收尾

### 恢复系统合约 

需要使用编译版本 [bos.contract-prebuild/master](https://github.com/boscore/bos.contract-prebuild/tree/master) 中的 `eosio.system` 和 `eosio.token` 合约进行升级，过程参考之前的步骤。

### 释放 burn.bos 资源

```
bash burntool.sh clr
```

然后发起 BP 多签将 `burn.bos` 的 `active` 权限更新回来：

```
cleos multisig propose upactive bp.json '[{"actor": "burn.bos", "permission": "owner"}]' eosio updateauth '{"threshold":1,"keys":[{"key":"EOS8FsuQAe7vXzYnGWoDXtdMgTXc2Hv9ctqAMvtRPrYAvn17nCftR", "weight":"1"}], "accounts":[]}' burnbosooooo 366 -p burnbosooooo@active

cleos multisig approve burnbosooooo upactive '{"actor":"bponeoneonee","permission":"active"}' -p bponeoneonee@owner

cleos multisig exec burnbosooooo upactive -p burnbosooooo@active
```