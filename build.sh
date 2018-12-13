#! /bin/bash

printf "\t=========== Building bos.contracts ===========\n\n"

RED='\033[0;31m'
NC='\033[0m'

CORES=`getconf _NPROCESSORS_ONLN`
mkdir -p build
rm -f build/CMakeCache.txt
pushd build &> /dev/null
cmake ../
make -j${CORES}
popd &> /dev/null
