
```
// BasicBridge  begin
   /**
    * @brief  
    */
   [[eosio::action]] void impvalidator(uint64_t requiredSignatures, std::vector<name>& initialValidators, name owner);
// BasicBridge end

// ForeignBridge  begin
   /**
    * @brief    void transferNativeToHome(name _recipient)
    */
   [[eosio::action]] void transfern2h(name sender,name recipient,uint64_t value);

   /**
    * @brief    transferTokenToHome(symbol _token, name _recipient, uint64_t _value)
    */
   [[eosio::action]] void transfert2h(name sender,std::string token, name recipient, uint64_t value);

 
    /**
    * @brief      // Event created on transfer to home.   event TransferToHome(address indexed token, address recipient, uint256 value);
    */
   [[eosio::action]] void transfer2he( const std::string& token, name recipient,uint64_t value);//


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
   [[eosio::action]] void transfern2f(name sender,name recipient,uint64_t value);

   /**
    * @brief    transferTokenToHome(symbol _token, name _recipient, uint64_t _value)
    */
   [[eosio::action]] void transfert2f(name sender,std::string token, name recipient, uint64_t value);

 
    /**
    * @brief       // Event created on transfer to home.   event TransferToHome(address indexed token, address recipient, uint256 value);
    */
   [[eosio::action]] void transfer2fe(std::string token, name recipient, uint64_t value);


   /**
    * @brief  transferFromForeign(std::string foreignToken, name recipient,uint64_t value, checksum256 transactionHash) 
    */
   [[eosio::action]] void transferfrof(name sender,std::string foreignToken, name recipient,uint64_t value, checksum256 transactionHash);


    /**
    * @brief  submitSignature(name sender, public_key sender_key, signature sig,bytes message) 
    */
   [[eosio::action]] void submitsig(name sender, public_key sender_key, signature sig, bytes message) ;

/// HomeBridge end

 [[eosio::action]] void setparameter(ignore<uint8_t> version,
                              ignore<std::string> core_symbol,
                              ignore<uint8_t> precision,
                              ignore<bridge_parameters> foreign,
                              ignore<bridge_parameters> home);
```