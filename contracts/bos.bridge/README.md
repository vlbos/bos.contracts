
```

// BasicBridge  begin
   /**
    * @brief  
    */
   [[eosio::action]] void impvalidator(uint64_t requiredSignatures, std::vector<std::pair<name,public_key>>& initialValidators, name owner);
// BasicBridge end

// ForeignBridge  begin
   /**
    * @brief    void transferNativeToHome(name _recipient)
    */
   [[eosio::action]] void transfern2h(name sender,std::string recipient,uint64_t value);

   /**
    * @brief    transferTokenToHome(symbol _token, name _recipient, uint64_t _value)
    */
   [[eosio::action]] void transfert2h(name sender,std::string token, std::string recipient, uint64_t value);

 
    /**
    * @brief       // Event created on transfer to home.   event TransferToHome(address indexed token, address recipient, uint256 value);
    */
   [[eosio::action]] void transfer2he( const std::string& token, std::string recipient,uint64_t value);//


   /**
    * @brief  transferFromHome(signature sig, bytes message)
    */
   [[eosio::action]] void transferfrom(name sender,std::vector<signature> sig, bytes message);

/// ForeignBridge  end

/// HomeBridge begin

   /**
    * @brief    void registerToken(name foreignAddress, name homeAddress)
    */
   [[eosio::action]] void regtoken(name sender,std::string foreignAddress, std::string homeAddress);

   /**
    * @brief    void transferNativeToHome(name _recipient)
    */
   [[eosio::action]] void transfern2f(name sender,std::string recipient,uint64_t value);

   /**
    * @brief    transferTokenToHome(symbol _token, name _recipient, uint64_t _value)
    */
   [[eosio::action]] void transfert2f(name sender,std::string token, std::string recipient, uint64_t value);

 
    /**
    * @brief       // Event created on transfer to home.   event TransferToHome(address indexed token, address recipient, uint256 value);
    */
   [[eosio::action]] void transfer2fe(std::string token, std::string recipient, uint64_t value);


   /**
    * @brief  transferFromForeign(std::string foreignToken, name recipient,uint64_t value, checksum256 transactionHash) 
    */
   [[eosio::action]] void transferfrof(name sender,std::string foreignToken, name recipient,uint64_t value, checksum256 transactionHash);


    /**
    * @brief  submitSignature(name sender, std::string sender_key, signature sig,bytes message) 
    */
   [[eosio::action]] void submitsig(name sender, public_key sender_key, signature sig, bytes message) ;

   

   /**
    * @brief   event CollectedSignatures(address authorityResponsibleForRelay, bytes32 messageHash, uint256 NumberOfCollectedSignatures);
    */
   [[eosio::action]] void collectedsig(name sender, checksum256 messageHash, uint64_t NumberOfCollectedSignatures) ;

   /**
 * @brief 
 * 
 * @param sender 
 * @param ishome 
 * @param token 
 * @param minPerTx 
 * @param maxPerTx 
 * @param dailyLimit 
 */
 [[eosio::action]] void settokenpara(name sender,bool ishome ,std::string token, uint64_t minPerTx,
                        uint64_t maxPerTx,uint64_t dailyLimit);


/**
 * @brief 
 * 
 * @param version 
 * @param core_symbol 
 * @param precision 
 * @param foreign 
 * @param home 
 */
 [[eosio::action]] void setparameter(ignore<uint8_t> version,
                              ignore<std::string> core_symbol,
                              ignore<uint8_t> precision,
                              ignore<bridge_parameters> foreign,
                              ignore<bridge_parameters> home);
                              
```