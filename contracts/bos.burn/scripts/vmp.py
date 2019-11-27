# coding:utf-8
#!/usr/bin/env python3
import csv
import re
import json

import argparse
import json
import numpy
import os
import random
import re
import subprocess
import sys
import time
import csv
import logging
import time
import datetime
import requests
from concurrent.futures import ProcessPoolExecutor
import time,os
import copy

airdrop_accounts_file = './dataset/accounts_info_bos_snapshot.airdrop.normal.csv'
airdrop_msig_accounts_file = './dataset/accounts_info_bos_snapshot.airdrop.msig.json'
nonactive_accounts_file = './dataset/nonactivated_bos_accounts.txt'
nonactive_msig_accounts_file = './dataset/nonactivated_bos_accounts.msig'
nonactive_airdrop_accounts_file = "./unactive_airdrop_accounts.csv"

cleos="/Users/lisheng/mygit/boscore/bos/build/programs/cleos/cleos --url http://127.0.0.1:8888 "

def jsonArg(a):
    return " '" + json.dumps(a) + "' "

def getOutput(args):
    # print('bios-boot-tutorial.py:', args)
    # logFile.write(args + '\n')
    proc = subprocess.Popen(args, shell=True, stdout=subprocess.PIPE)
    return proc.communicate()[0].decode('utf-8')

def getJsonOutput(args):
    return json.loads(getOutput(args))

def writeResult(resCsv):
    if len(resCsv)==0:
        return
    nonactive_airdrop_accounts_file=str(os.getpid())+"".join(resCsv[0])+"no.csv"

    with open(nonactive_airdrop_accounts_file, 'w') as cfile:
        writer = csv.writer(cfile)
        for item in resCsv:
            writer.writerow(item)

    cfile.close()

def loadcsv(csvFile,t):
    limit_length=10000
    f = open(csvFile, 'r')
    accounts = f.readlines()
    flen =  len(accounts)
    # reader = csv.reader(f)
    # accounts=[]
    # for item in reader:
    #     accounts.append([item[0],item[1]])
    #     if len(accounts) > limit_length:
    #         # validAccounts(accounts)
    #         t.submit(validAccounts,accounts).add_done_callback(parse)
    #         accounts=[]
    i=0
    while i < flen:
        end = i+limit_length
        accs=copy.deepcopy(accounts[i:])
        if end < flen:
            accs=copy.deepcopy(accounts[i:end])
        dt_ms = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f') # 含微秒的日期时间，来源 比特量化
        print("loadcsv",dt_ms)    
        t.submit(validAccounts,accs,i).add_done_callback(parse)

        if i>0 and (i/limit_length%10)==0:
            time.sleep(30)

        i += limit_length


    f.close()


def validAccount(account,quantity):
    table = getJsonOutput(cleos + 'get table burn.bos ' + account + ' accounts -l 100')
    resCsv = []
    for row in table['rows']:
        if not row['account'] or row['quantity']!=quantity:
            resCsv.append([row['account'], row['quantity'],quantity])
        # else:
        #     print(row['account'], row['quantity']," Yes ")
  
    writeResult(resCsv)

def validAccounts(accounts,start):
    print('pid:%s %s validAccounts first: %s' % (os.getpid(),len(accounts),"".join(accounts[0])))
    count=0
    for acc in accounts:
       acc = acc.strip('\n')
       item=acc.split(",")
       validAccount(item[0],item[1])
       count += 1
       if count%1000==0:
          print('pid:%s count : %s' % (os.getpid(),str(start+count)))

    return {'first':accounts[0],'last':accounts[-2:-1],'len':len(accounts)}

# 进程池方式


def parse(obj):
    res=obj.result()
    print('[%s] <%s> (%s) =%s=' % (os.getpid(), res['first'], res['last'],res['len']))
    dt_ms = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f') # 含微秒的日期时间，来源 比特量化
    print(dt_ms)    

if __name__ == '__main__':

    dt_ms = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f') # 含微秒的日期时间，来源 比特量化
    print(dt_ms)    
    #  t.__exit__方法会调用t.shutdown(wait=True)方法，
    with ProcessPoolExecutor(max_workers=10) as t:
        csvFile="./unactive_airdrop_accounts.csv"
        loadcsv(csvFile, t)
  
            
    print('主',os.getpid())
    dt_ms = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f') # 含微秒的日期时间，来源 比特量化
    print(dt_ms)    
