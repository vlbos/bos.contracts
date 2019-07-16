#!/bin/sh

YAML=docker-compose.yml
CONTAINER=cdt
MNT_PATH=/bos-mnt
CONTRACT_NAME=$1
CONTRACT_PATH=$MNT_PATH/$CONTRACT_NAME

docker-compose -f $YAML up -d

docker-compose -f $YAML exec $CONTAINER bash -c "rm -rf $CONTRACT_PATH/$CONTRACT_NAME.abi $CONTRACT_PATH/$CONTRACT_NAME.wast $CONTRACT_PATH/$CONTRACT_NAME.wasm"
docker-compose -f $YAML exec $CONTAINER bash -c "cd $CONTRACT_PATH && eosio-cpp -I=$CONTRACT_PATH/include/ -abigen $CONTRACT_PATH/src/$CONTRACT_NAME.cpp --contract=$CONTRACT_NAME -abigen_output=$CONTRACT_PATH/$CONTRACT_NAME.abi"
docker-compose -f $YAML exec $CONTAINER bash -c "cd $CONTRACT_PATH && eosio-cpp -I=$CONTRACT_PATH/include/ -o $CONTRACT_PATH/$CONTRACT_NAME.wast $CONTRACT_PATH/src/$CONTRACT_NAME.cpp"
docker-compose -f $YAML exec $CONTAINER bash -c "cd $CONTRACT_PATH && eosio-cpp -I=$CONTRACT_PATH/include/ -o $CONTRACT_PATH/$CONTRACT_NAME.wasm $CONTRACT_PATH/src/$CONTRACT_NAME.cpp"

docker-compose -f $YAML down