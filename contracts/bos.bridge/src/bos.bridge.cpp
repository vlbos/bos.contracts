#include <eosio/eosio.hpp>
#include "bos.bridge/bos.bridge.hpp"
#include <cmath>
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/transaction.hpp>
using namespace eosio;
using std::string;


void bos_bridge::regservice(name account, asset base_stake_amount, std::string data_format, uint8_t data_type, std::string criteria, uint8_t acceptance, uint8_t injection_method, uint32_t duration,
                            uint8_t provider_limit, uint32_t update_cycle) {
   require_auth(account);
   check(_bridge_meta_parameters.version == current_bridge_version, "config parameters must first be initialized ");
   auto paras = unpack<bridge_parameters>(_bridge_meta_parameters.parameters_data);
   check_stake(base_stake_amount, "base_stake_amount", paras.min_service_stake_limit);

   check(provider_limit >= paras.min_provider_limit && provider_limit <= paras.max_provider_limit, "provider_limit could not be less than 1 or greater than 100");
   check(update_cycle >= paras.min_update_cycle && update_cycle <= paras.max_update_cycle, "update_cycle could not be less than 3 seconds or greater than 100 days");
   check(duration >= paras.min_duration && duration <= paras.max_duration, "duration could not be less than 1 or greater than 3600 seconds");
   check(duration < update_cycle, "duration could not be  greater than update_cycle ");
   check(injection_method == chain_indirect || injection_method == chain_direct || injection_method == chain_outside,
         "injection_method only set chain_indirect(0) or chain_direct(1)or chain_outside(2)");
   check(data_type == data_deterministic || data_type == data_non_deterministic, "data_type only set value data_deterministic(0) or data_non_deterministic(1)1");
   check(acceptance >= paras.min_acceptance && acceptance <= paras.max_acceptance, "acceptance could not be less than 3 or greater than 100 ");
   check_data(data_format, "data_format");
   check_data(criteria, "criteria");

   data_services svctable(_self, _self.value);

   uint64_t id = svctable.available_primary_key();
   if (0 == id) {
      id++;
   }

   save_id(0, id);

   // add service
   svctable.emplace(_self, [&](auto& s) {
      s.service_id = id;
      s.data_format = data_format;
      s.data_type = data_type;
      s.criteria = criteria;
      s.acceptance = acceptance;
      s.injection_method = injection_method;
      s.base_stake_amount = base_stake_amount;
      s.duration = duration;
      s.provider_limit = provider_limit;
      s.update_cycle = update_cycle;
      s.update_start_time = bos_bridge::current_time_point_sec(); // update_start_time;
      s.last_cycle_number = 0;
      s.appeal_freeze_period = 0;
      s.exceeded_risk_control_freeze_period = 0;
      s.guarantee_id = 0;
      s.risk_control_amount = asset(0, core_symbol());
      s.pause_service_stake_amount = asset(0, core_symbol());
      s.status = service_status::service_init;
   });
}



void bos_bridge::transfertof(name account, asset amount) {
 
}



void bos_bridge::unregservice(uint64_t service_id, name account, uint8_t status) {
  
}



// } // namespace bosbridge

/**
 * @brief  Sets config parameters
 *
 * @param version   parameters version  changed when paramters changes
 * @param parameters  config paramters such as core symbol,precision etc
 */
void bos_bridge::setparameter(ignore<uint8_t> version, ignore<bridge_parameters> parameters) {
   require_auth(_self);
   uint8_t _version;
   bridge_parameters _parameters;
   _ds >> _version >> _parameters;
   //  print(_version, _parameters.core_symbol, _parameters.precision);
   check(!_parameters.core_symbol.empty(), "core_symbol could not be empty");
   check(_parameters.precision > 0, "precision must be greater than 0");
   check(_parameters.min_service_stake_limit > 0, "min_service_stake_limit must be greater than 0");
   check(_parameters.min_appeal_stake_limit > 0, "min_appeal_stake_limit must be greater than 0");
   check(_parameters.min_reg_arbitrator_stake_limit > 0, "min_reg_arbitrator_stake_limit must be greater than 0");
   check(_parameters.arbitration_correct_rate > 0, "arbitration_correct_rate must be greater than 0");
   check(_parameters.round_limit > 0, "round_limit must be greater than 0");
   check(_parameters.arbi_timeout_value > 0, "arbi_timeout_value must be greater than 0");
   check(_parameters.arbi_freeze_stake_duration > 0, "arbi_freeze_stake_duration must be greater than 0");
   check(_parameters.time_deadline > 0, "time_deadline must be greater than 0");
   check(_parameters.clear_data_time_length > 0, "clear_data_time_length must be greater than 0");
   check(_parameters.max_data_size > 0, "max_data_size must be greater than 0");
   check(_parameters.min_provider_limit > 0, "min_provider_limit must be greater than 0");
   check(_parameters.max_provider_limit > 0, "max_provider_limit must be greater than 0");
   check(_parameters.min_update_cycle > 0, "min_update_cycle must be greater than 0");
   check(_parameters.max_update_cycle > 0, "max_update_cycle must be greater than 0");
   check(_parameters.min_duration > 0, "min_duration must be greater than 0");
   check(_parameters.max_duration > 0, "max_duration must be greater than 0");
   check(_parameters.min_acceptance > 0, "min_acceptance must be greater than 0");
   check(_parameters.max_acceptance > 0, "max_acceptance must be greater than 0");

   std::string checkmsg = "unsupported version for setparameter action,current_bridge_version=" + std::to_string(current_bridge_version);
   check(_version == current_bridge_version, checkmsg.c_str());
   _bridge_meta_parameters.version = _version;
   _bridge_meta_parameters.parameters_data = pack(_parameters);
}






EOSIO_DISPATCH(bos_oracle,
 (regservice)(unregservice)(execaction)(unstakeasset)(pushdata)(oraclepush)(
              addfeetypes)(claim)(subscribe)(requestdata)(starttimer)(cleardata)
              (uploadeviden)(uploadresult)(acceptarbi)(unstakearbi)(claimarbi)(timertimeout)
              (deposit)(withdraw)
              (importwps)(setstatus)
              (setparameter)
)
