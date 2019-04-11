#! /bin/bash

printf "\t=========== Building eosio.contracts ===========\n\n"

RED='\033[0;31m'
NC='\033[0m'

CORES=`getconf _NPROCESSORS_ONLN`
mkdir -p build
pushd build &> /dev/null
cmake -DEOSIO_ROOT=/Users/lisheng/opt/eosio -DCMAKE_MODULE_PATH=/Users/lisheng/opt/eosio/lib/cmake/eosio ../
make -j${CORES}
popd &> /dev/null
