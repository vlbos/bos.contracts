#pragma once
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include "bos.bridge/bos.config.hpp"

using namespace eosio;
// using std::string;


struct [[eosio::table, eosio::contract("bos.bridge")]] bridge_validator {
   name chain;
   std::map<name,bool>  validators;
   uint64_t validatorCount;
   uint64_t requiredSignatures;
   
    name  validatorContractAddress;
    uint64_t  gasPrice; // Used by bridge client to determine proper gas price for corresponding chain
    uint64_t  requiredBlockConfirmations; // Used by bridge client to determine proper number of blocks to wait before validating transfer
    uint64_t  deployedAtBlock; // Used by bridge client to determine initial block number to start listening for transfers
    
    std::map<name,uint64_t>   minPerTx;
    std::map<name,uint64_t>   maxPerTx; // Set to 0 to disable
    std::map<name,uint64_t>   dailyLimit; // Set to 0 to disable
    std::map<name,std::map<uint64_t,uint64_t>>  totalSpentPerDay;

    std::map<std::string,bool>  transfers;
    // mapping between foreign token addresses to home token addresses
    std::map<name,name>  foreignToHomeTokenMap;
    // mapping between home token addresses to foreign token addresses
    std::map<name,name> homeToForeignTokenMap;
    // mapping between message hash and transfer message. Message is the hash of (recipientAccount, transferValue, transactionHash)
    std::map<eosio::checksum256,bytes>  messages;
    // mapping between hash of (transfer message hash, validator index) to the validator signature
    std::map<eosio::checksum256,bytes> signatures;
    // mapping between hash of (validator, transfer message hash) to whether the transfer was signed by the validator
    std::map<eosio::checksum256,bool>  transfersSigned;
    // mapping between the transfer message hash and the number of validator signatures
    std::map<eosio::checksum256,uint64_t>  numTransfersSigned;
    // mapping between the hash of (validator, transfer message hash) to whether the transfer was signed by the validator
    std::map<eosio::checksum256,bool>  messagesSigned;
    // mapping between the transfer message hash and the number of validator signatures
    std::map<eosio::checksum256,uint64_t> numMessagesSigned;

   uint64_t primary_key() const { return chain.value; }
};



struct [[eosio::table, eosio::contract("bos.oracle")]] transfer_data_item {
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
   std::string core_symbol = default_core_symbol;
   uint8_t precision = default_precision;


   // explicit serialization macro is not necessary, used here only to improve compilation time
   EOSLIB_SERIALIZE(bridge_parameters, (core_symbol)(precision))
};

struct [[eosio::table("metaparams"), eosio::contract("bos.bridge")]] bridge_meta_parameters {
   bridge_meta_parameters() {parameters_data = pack(bridge_parameters{}); }
   uint8_t version;
   std::vector<char> parameters_data;

   EOSLIB_SERIALIZE(bridge_meta_parameters, (version)(parameters_data))
};

typedef eosio::singleton<"metaparams"_n, bridge_meta_parameters> bridge_meta_parameters_singleton;
