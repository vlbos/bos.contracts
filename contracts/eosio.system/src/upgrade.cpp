#include <eosio.system/eosio.system.hpp>

namespace eosiosystem {

    void system_contract::setupgrade( const upgrade_proposal& up) {
        require_auth( _self );

        auto params = eosio::upgrade_parameters{};
        params.target_block_num = up.target_block_num;
        (eosio::upgrade_parameters&)(_ustate) = params;
        set_upgrade_parameters( params );

//        _ustate.active_proposal = up.proposal_name;
        _ustate.target_block_num = up.target_block_num;
    }
}