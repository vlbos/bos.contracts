#!/bin/sh


rm /bos-mnt/bos.pegtoken.abi
eosio-cpp -abigen /bos-mnt/src/bos.pegtoken.cpp --contract=bos.pegtoken -abigen_output=/bos-mnt/bos.pegtoken.abi -I=/bos-mnt/include/


rm /bos-mnt/bos.pegtoken.wast
eosio-cpp -I=/bos-mnt/include/ -o /bos-mnt/bos.pegtoken.wast /bos-mnt/src/bos.pegtoken.cpp

rm /bos-mnt/bos.pegtoken.wasm
eosio-cpp -I=/bos-mnt/include/ -o /bos-mnt/bos.pegtoken.wasm /bos-mnt/src/bos.pegtoken.cpp



