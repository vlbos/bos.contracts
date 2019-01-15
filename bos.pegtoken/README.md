macos下, 假设当前目录为

/Users/huobi

编译：

$ cd /Users/huobi/bos.pegtoken

$ docker-compose up -d

测试链:
$ docker-compose exec cdt /bin/sh /bos-mnt/build-testnet.sh
主链:
$ docker-compose exec cdt /bin/sh /bos-mnt/build-mainnet.sh

$ docker-compose down

发布：

$ cleos set contract pegtoken /Users/huobi/bos.pegtoken -p pegtoken

