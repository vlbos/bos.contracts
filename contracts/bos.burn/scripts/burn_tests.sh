#!/usr/bin/env bash

contract_repo_dir=bos.contracts

# CONTRACTS_DIR=~/${contract_repo_dir}/build/contracts/
CONTRACTS_DIR=~/mygit/vlbos/burn/${contract_repo_dir}/build/contracts/
# CONTRACTS_DIR=~/abos/${contract_repo_dir}
WALLET_DIR=~/bosburn-wallet

http_endpoint='http://127.0.0.1:8888'
cleos1="cleos -u "$http_endpoint

contract_burn=burn.bos
contract_burn_folder=bos.burn

burn_c_pubkey=EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
burn_c_prikey=5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
burn_total_quantity="0.0001 BOS"

flag=0
count=0
limits=100
accs=''
file=./unactive_airdrop_accounts.csv
result_file=./unactive_airdrop_accounts_result.csv

start_keosd() {
    killall keosd 2>/dev/null
    rm -rf ${WALLET_DIR}
    mkdir ${WALLET_DIR}
    nohup keosd --wallet-dir ${WALLET_DIR} --unlock-timeout 90000 1>/dev/null 2>/dev/null &
}
start_keosd

create_wallet() {
    # rm -rf ~/eosio-wallet/
    cleos wallet create -f ${WALLET_DIR}/password.txt
    cleos wallet import --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
}
create_wallet

import_key() {
    echo prikey=$1
    cleos wallet import --private-key $1
}

import_key ${burn_c_prikey}

get_burn_table() {
    ${cleos1} get table ${contract_burn} ${contract_burn} $1 --limit 100

}

get_burn_table_scope() {
    ${cleos1} get table ${contract_burn} $1 $2 --limit 100
}

get_info() {
    ${cleos1} get info
}
get_scope() {
    ${cleos1} get scope -t stat eosio.token
    ${cleos1} get scope -t accounts ${contract_burn}
}

set_contracts() {
    ${cleos1} set contract ${contract_burn} ${CONTRACTS_DIR}/${contract_burn_folder} -x 1000 -p ${contract_burn}
    sleep .2

    ${cleos1} set account permission ${contract_burn} active '{"threshold": 1,"keys": [{"key": "'${burn_c_pubkey}'","weight": 1}],"accounts": [{"permission":{"actor":"'${contract_burn}'","permission":"eosio.code"},"weight":1}]}' owner -p ${contract_burn}@owner
}

setparameter() {
    # ${cleos1} push action ${contract_burn} setparameter '[1,"'${contract_burn}'"]' -p ${contract_burn}
    ${cleos1} push action ${contract_burn} setparameter '[1,"'${contract_burn}'"]' -p ${contract_burn}
}


OLD_IFS='='
save_ifs_() {
    OLD_IFS=$IFS #保存原始值
    IFS=$1
}

save_ifs_eq() {
    save_ifs_ "="
}

save_ifs() {
    save_ifs_ ","
}

restore_ifs() {
    IFS=$OLD_IFS #还原IFS的原始值
}

Start=$(date +%s)
End=$(date +%s)
get_start() {
    Start=$(date +%s)
}

get_end() {
    End=$(date +%s)
    Time=$(($End - $Start))
    echo $Time"==============end======Time======="
}


import_accs() {
    save_ifs_eq
    cleos -u $http_endpoint push action ${contract_burn} importacnts '[['$accs']]' -p ${contract_burn}
    restore_ifs
}

import_from_csv() {
    get_start

    while IFS="," read name quantity; do
        quantityx=$(echo $quantity | tr -d '\r')
        accs=$accs$(echo '["'$name'","'$quantityx'"]')
        count=$(($count + 1))
        if (($(($count % $limits)) == 0)); then
            import_accs
            echo "=========count========"$count
            End=$(date +%s)
            echo $End
            accs=''
        else
            accs=$accs','
        fi
    done <$file

    if (($(($count % $limits)) > 0)); then
        accs=${accs%","}
        import_accs
        echo "=========count========"$count
    fi

    get_end
}

transferairs() {
    save_ifs_eq
    cleos -u $http_endpoint push action ${contract_burn} transferairs '["'$1'"]' -p ${contract_burn}
    restore_ifs
}

transferairs_from_csv() {
    get_start
    count=0
    while IFS="," read name quantity; do
        transferairs $name
        End=$(date +%s)
        count=$(($count + 1))
        echo $count"=====transferairs==Time===="
    done <$file

    get_end
}

checkresult() {
    save_ifs_eq
    result=$(cleos -u $http_endpoint  get table ${contract_burn} $1 accounts)
    status=${result##*is_burned} #结果为 @@@
    status=$(echo $status | tr -d "\]")
    status=$(echo $status | tr -cd "[0-9]")
    if [[ temp -eq 0 ]]; then
        echo $1','$2' '$3 >> $result_file
    else
        echo $1 done !
    fi
    restore_ifs
}

checkresult_from_csv() {
    get_start
    count=0
    while IFS="," read name quantity; do
        quantityx=$(echo $quantity | tr -d '\r')
        checkresult $name $quantityx
        End=$(date +%s)
        count=$(($count + 1))
        echo $count"=====checkresult_from_csv==Time===="
    done <$file
    get_end
}

burn_from_hole() {
    save_ifs_eq
    cleos -u $http_endpoint push action ${contract_burn} burn '["'$burn_total_quantity'"]' -p ${contract_burn}
    restore_ifs
}

arr_clear() {
    save_ifs_eq
    cleos -u $http_endpoint push action ${contract_burn} clear '[['$accs']]' -p ${contract_burn}
    restore_ifs
}
clear_from_csv() {
    get_start

    while IFS="," read name quantity; do
        accs=$accs'"'$name'"'
        count=$(($count + 1))
        if (($(($count % $limits)) == 0)); then
            arr_clear
            echo "=========count========"$count
            accs=''
        else
            accs=$accs','
        fi
    done <$file

    if (($(($count % $limits)) > 0)); then
        accs=${accs%","}
        arr_clear
        echo "=========count========"$count
    fi

    get_end
}

test_() {
    # OLD_IFS=$IFS #保存原始值
    # IFS="="
    # # cleos -u $http_endpoint push action ${contract_burn} importacnts '[[["provider3333","0.0001 BOS"]]]' -p ${contract_burn}
    # IFS=$OLD_IFS #还原IFS的原始值
    checkresult "aabcftaqpqak"
}

case "$1" in
"set") set_contracts ;;
"imp") import_from_csv ;;
"setp") setparameter ;;
"air") transferairs_from_csv ;;
"chk") checkresult_from_csv ;;
"burn") burn_from_hole ;;
"clr") clear_from_csv ;;
"acc") get_account "$2" ;;
"table") get_burn_table "$2" ;;
"tables") get_burn_table_scope "$2" "$3" ;;
"info") get_info ;;
"scope") get_scope ;;
"test") test_ "$2" ;;
*) echo "usage: burn_test.sh set|imp|setp|air|chk|burn|clr|acc|table {name}|tables {scope name}|info|scope|test" ;;
esac
