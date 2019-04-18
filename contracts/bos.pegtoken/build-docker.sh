#!/bin/sh

cd /bos-mnt/src
rm ./*.abi 
rm ./*.was*
cp -r /bos-mnt/include/bos.pegtoken .
eosio-abigen -contract=bos.pegtoken -output=./bos.pegtoken.abi   ./bos.pegtoken.cpp 
eosio-cpp -I=/bos-mnt/include/ -o ./bos.pegtoken.wast ./bos.pegtoken.cpp
rm -r ./bos.pegtoken