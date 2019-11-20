# coding:utf-8
import csv
import re
import json
airdrop_accounts_file = './dataset/accounts_info_bos_snapshot.airdrop.normal.csv'
airdrop_msig_accounts_file = './dataset/accounts_info_bos_snapshot.airdrop.msig.json'
nonactive_accounts_file = './dataset/nonactivated_bos_accounts.txt'
nonactive_msig_accounts_file = './dataset/nonactivated_bos_accounts.msig'
nonactive_airdrop_accounts_file = "./unactive_airdrop_accounts.csv"

# txt
def loadtxt(txt):
    f = open(txt, 'r')
    sourceInline = f.readlines()
    dataset = []
    for line in sourceInline:
        temp1 = line.strip('\x00')
        temp1 = temp1.strip('\n')
        if (temp1 == ''):
            continue
        dataset.append(temp1.strip())
    f.close
    return dataset

# csv


def loadcsv(csvFile):
    f = open(csvFile, 'r')
    reader = csv.reader(f)
    csvset = {}
    csvlist = []
    for item in reader:
        csvset[item[4]] = item[5]
        csvlist.append(item[4])
    f.close()
    return csvset, csvlist


def unionset():
    # 读取txt获取主网未激活账户
    tacclist = loadtxt(nonactive_accounts_file)

    # 读取空投账户 csv集合  
    caccset, cacclist = loadcsv(airdrop_accounts_file)

    # 创建数值集合
    tc = set(tacclist) & set(cacclist)

    resCsv = []
    sumBurn = 0
    # 构造交集结果
    for item in tc:
        resCsv.append([item, caccset[item]])
        sumBurn += float(caccset[item].replace('BOS', '').strip())

    print('len of unactive', len(resCsv))
    msig, sumBurnmsig = intersectmsigset()
    resCsv += msig
    sumBurn += sumBurnmsig

    # 写结果
    with open(nonactive_airdrop_accounts_file, 'w') as cfile:
        writer = csv.writer(cfile)
        for item in resCsv:
            writer.writerow(item)

    cfile.close()

    print('unactive airdrop accounts count:', len(resCsv))
    print('unactive airdrop accounts quantity:', sumBurn)


def msigfromjson():
    # 由于文件中有多行，直接读取会出现错误，因此一行一行读取
    file = open(airdrop_msig_accounts_file, 'r', encoding='utf-8')
    csvset = {}
    lst = []
    for line in file.readlines():
        item = json.loads(line)
        csvset[item['bos_account']] = item['bos_balance']
        lst.append(item['bos_account'])

    file.close()
    return csvset,lst

def intersectmsigset():
    # 读取csv集合
    msiglist = loadtxt(nonactive_msig_accounts_file)

    accquan,unactive_list = msigfromjson()

    # 创建数值集合
    tc = set(msiglist) & set(unactive_list)

    resCsv = []
    sumBurn = 0
    # 构造交集结果
    for item in tc:
            resCsv.append([item, accquan[item]])
            sumBurn += float(accquan[item].replace('BOS', '').strip())

    print('unactive airdrop msig accounts count:', len(resCsv))
    print('unactive airdrop msig accounts quantity:', sumBurn)

    return resCsv, sumBurn


unionset()
