#pragma once
#include "bos.bridge/interfaces/IBridgeValidators.hpp"
#include "bos.bridge/bos.config.hpp"
#include "bos.bridge/bos.types.hpp"

class Message {
    public:
    static bool addressArrayContains(std::vector<eosio::public_key> array, name value)  {
        for (size_t i = 0; i < array.size(); i++) {
            if (array[i] == value) {
                return true;
            }
        }
        return false;
    }

    // layout of message :: bytes:
    // offset  0: 32 bytes :: uint256 - message length
    // offset 32: 20 bytes :: address - token address
    // offset 52: 20 bytes :: address - recipient address
    // offset 72: 32 bytes :: uint256 - value
    // offset 104: 32 bytes :: bytes32 - transaction hash

    // bytes 1 to 32 are 0 because message length is stored as little endian.
    // mload always reads 32 bytes.
    // so we can and have to start reading recipient at offset 20 instead of 32.
    // if we were to read at 32 the address would contain part of value and be corrupted.
    // when reading from offset 20 mload will read 12 zero bytes followed
    // by the 20 recipient address bytes and correctly convert it into an address.
    // this saves some storage/gas over the alternative solution
    // which is padding address to 32 bytes and reading recipient at offset 32.
    // for more details see discussion in:
    // https://github.com/paritytech/parity-bridge/issues/61
     static std::tuple<name,name,int64_t,bytes> parseMessage(bytes message)
     {
        check(isMessageValid(message), "Incorrect message format");
        // token := and(mload(add(message, 20)), 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF)
        // recipient := and(mload(add(message, 40)), 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF)
        // amount := mload(add(message, 72))
        // txHash := mload(add(message, 104))
        uint64_t token;
        uint64_t recipient;
        int64_t amount =0;
        uint8_t length=20;
        uint8_t offset=32;
        
        std::memcpy(token, &message.data()[offset], length);
        std::memcpy(recipient, &message.data()[offset+length], length);
        std::memcpy(amount, &message.data()[offset+length+length], offset);
        uint8_t hashoffset=offset+length+length+offset;
        bytes txHash(message.begin()+hashoffset,message.begin()+hashoffset+offset)

        return std::make_tuple(name(token),name(recipient),amount,txHash);
    }

   static  bool isMessageValid(bytes _msg) {
        return _msg.size() == requiredMessageLength();
    }

   static uint64_t requiredMessageLength() {
        return 104;
    }

    static eosio::public_key recoverAddressFromSignedMessage(signature sig, bytes message)  {
        check(sig.size() == 65, "Incorrect signature length");
        return recover_key(hashMessage(message), sig);
    }

    static eosio::checksum256 hashMessage(bytes message)  {
        std::string  prefix = "\x19BOS Signed Message:\n";
        // message is always 84 length
        std::string  msgLength = "104";
        // return keccak256(abi.encodePacked(prefix, msgLength, message));
        return get_checksum256((prefix, msgLength, message));
    }

    static void hasEnoughValidSignatures(
        bytes _message,
        bytes _vs,
        std::vector<name>& _rs,
        std::vector<public_key>& _ss,
        IBridgeValidators _validatorContract) {
        check(isMessageValid(_message), "Invalid message format");
        uint64_t requiredSignatures = _validatorContract.requiredSignatures();
        check(_vs.size() >= requiredSignatures, "Num of signatures in message is less than requiredSignatures");
        bytes hash = hashMessage(_message);
        std::vector<eosio::public_key>  encounteredAddresses;// = new address[](requiredSignatures);

        for (uint64_t i = 0; i < requiredSignatures; i++) {
            public_key recoveredAddress = recover_key(hash,_ss[i]);
            check(_validatorContract.isValidator(recoveredAddress), "Signer of message is not a validator");
            if (addressArrayContains(encounteredAddresses, recoveredAddress)) {
                check(false);
            }
            encounteredAddresses.push_back(recoveredAddress);
        }
    }
};
