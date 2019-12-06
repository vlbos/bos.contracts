#pragma once
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>


    using namespace eosio;

    CONTRACT randomtest : public contract
    {
        typedef std::chrono::milliseconds duration;
    public:
        using contract::contract;
        
        ACTION apply( name contract, name act );

        struct args{
            uint64_t loop;
            uint64_t num;

            EOSLIB_SERIALIZE(args, (loop)(num))
        };
   
        ACTION generate(const args& t);

   
        ACTION clear();

        struct args_name
        {
            name name;
        };
   
        ACTION hascontract(const args_name& t);

        struct argsinline
        {
            name payer;
            name in;

            EOSLIB_SERIALIZE(argsinline, (payer)(in))
        };
   
        ACTION inlineact(const argsinline& t);

    public:
        // @abi table seedobjs i64
        TABLE seedobj
        {
            uint64_t    id;
            time_point  create;
            std::string seedstr;
            std::string txid;
            uint64_t    action;

            uint64_t primary_key()const { return id; }
            EOSLIB_SERIALIZE(seedobj,(id)(create)(seedstr)(txid)(action))
        };
        typedef eosio::multi_index< "seedobjs"_n, seedobj> seedobjs;


    };

