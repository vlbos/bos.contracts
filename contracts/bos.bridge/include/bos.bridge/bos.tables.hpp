#pragma once
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include "bos.bridge/bos.config.hpp"

using namespace eosio;
// using std::string;


struct [[eosio::table, eosio::contract("bos.bridge")]] bridge_validator {
   name chain;


   uint64_t primary_key() const { return chain.value; }
};



struct [[eosio::table, eosio::contract("bos.bridge")]] transfer_data_item {
   uint64_t item_id;
   name chain;
   name account;
   asset amount;
   std::string hash;
   uint8_t status;
   uint32_t timestamp;
   std::vector<name>  validators;
   uint64_t primary_key() const { return item_id; }

   EOSLIB_SERIALIZE(transfer_data_item, (item_id)(chain)(account)(amount)(hash)(status)(timestamp))
};

typedef eosio::multi_index<"validators"_n, bridge_validator> bridge_validators;


struct bridge_parameters {

   name  validatorContractAddress;
   uint64_t  gasPrice; // Used by bridge client to determine proper gas price for corresponding chain
   uint64_t  requiredBlockConfirmations; // Used by bridge client to determine proper number of blocks to wait before validating transfer

    uint64_t   minPerTx;
    uint64_t   maxPerTx; // Set to 0 to disable
    uint64_t   dailyLimit; // Set to 0 to disable
    
   // explicit serialization macro is not necessary, used here only to improve compilation time
   EOSLIB_SERIALIZE(bridge_parameters, (validatorContractAddress)(gasPrice)(requiredBlockConfirmations)
   (minPerTx)(maxPerTx)(dailyLimit))
};

struct bridge_parameters_storage {

   name  validatorContractAddress;
   uint64_t  gasPrice; // Used by bridge client to determine proper gas price for corresponding chain
   uint64_t  requiredBlockConfirmations; // Used by bridge client to determine proper number of blocks to wait before validating transfer
   block_timestamp  deployedAtBlock; // Used by bridge client to determine initial block number to start listening for transfers

    std::map<std::string,uint64_t>   minPerTx;
    std::map<std::string,uint64_t>   maxPerTx; // Set to 0 to disable
    std::map<std::string,uint64_t>   dailyLimit; // Set to 0 to disable
    std::map<std::string,std::map<uint64_t,uint64_t>>  totalSpentPerDay;

   // explicit serialization macro is not necessary, used here only to improve compilation time
   EOSLIB_SERIALIZE(bridge_parameters_storage, (validatorContractAddress)(gasPrice)(requiredBlockConfirmations)(deployedAtBlock)
   (minPerTx)(maxPerTx)(dailyLimit)(totalSpentPerDay))
};

struct [[eosio::table("metaparams"), eosio::contract("bos.bridge")]] bridge_meta_parameters {
   bridge_meta_parameters() { }
   uint8_t version;
   std::string core_symbol = default_core_symbol;
   uint8_t precision = default_precision;

   name owner;
   std::map<name,bool>  validators;
   uint64_t validatorCount;
   uint64_t requiredSignatures;
   
    bridge_parameters_storage foreign;
    bridge_parameters_storage home;

    std::map<checksum256,bool>  transfers;
    // mapping between foreign token addresses to home token addresses
    std::map<std::string,std::string>  foreignToHomeTokenMap;
    // mapping between home token addresses to foreign token addresses
    std::map<std::string,std::string> homeToForeignTokenMap;
    // mapping between message hash and transfer message. Message is the hash of (recipientAccount, transferValue, transactionHash)
    std::map<eosio::checksum256,bytes>  messages;
    // mapping between hash of (transfer message hash, validator index) to the validator signature
    std::map<eosio::checksum256,signature> signatures;
    // mapping between hash of (validator, transfer message hash) to whether the transfer was signed by the validator
    std::map<eosio::checksum256,bool>  transfersSigned;
    // mapping between the transfer message hash and the number of validator signatures
    std::map<eosio::checksum256,uint64_t>  numTransfersSigned;
    // mapping between the hash of (validator, transfer message hash) to whether the transfer was signed by the validator
    std::map<eosio::checksum256,bool>  messagesSigned;
    // mapping between the transfer message hash and the number of validator signatures
    std::map<eosio::checksum256,uint64_t> numMessagesSigned;

    EOSLIB_SERIALIZE(
        bridge_meta_parameters,
        (version)(validators)(validatorCount)(requiredSignatures)
        (foreign)(home)(transfers)(foreignToHomeTokenMap)(homeToForeignTokenMap)(
            messages)(signatures)(transfersSigned)(numTransfersSigned)(
            messagesSigned)(numMessagesSigned))
};

typedef eosio::singleton<"metaparams"_n, bridge_meta_parameters> bridge_meta_parameters_singleton;
