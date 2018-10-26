/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include "eosio.system.hpp"

#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/datastream.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/privileged.h>
#include <eosiolib/transaction.hpp>

#include <eosio.token/eosio.token.hpp>


#include <cmath>
#include <map>

namespace eosiosystem {
   using eosio::asset;
   using eosio::indexed_by;
   using eosio::const_mem_fun;
   using eosio::bytes;
   using eosio::print;
   using eosio::permission_level;
   using std::map;
   using std::pair;

   struct user_resources {
      account_name  owner;
      uint64_t       net_weight = 0;
      uint64_t       cpu_weight = 0;
      uint64_t       ram_bytes = 0;

      uint64_t primary_key()const { return owner; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( user_resources, (owner)(net_weight)(cpu_weight)(ram_bytes) )
   };

   /**
    *  These tables are designed to be constructed in the scope of the relevant user, this
    *  facilitates simpler API for per-user queries
    */
   typedef eosio::multi_index< N(userres), user_resources>      user_resources_table;


   void system_contract::setmaxram( uint64_t max_ram_size ) {
      require_auth( _self );

      eosio_assert( max_ram_size < SYSTEM_MAXIMUM_RAM, "ram size is unrealistic" );
      eosio_assert( max_ram_size > _gstate.total_ram_bytes_reserved + _gstate.free_accounts_ram(), "attempt to set max below reserved" );

      _gstate.max_ram_size = max_ram_size;
      _global.set( _gstate, _self );
   }

   void system_contract::setmaxaccnts( uint64_t max_accounts ) {
      require_auth( _self );

      uint64_t ram_size_for_accounts = max_accounts * _gstate.account_ram_size;
      eosio_assert( ram_size_for_accounts > _gstate.total_ram_bytes_reserved_for_accounts, "attempt to set max accounts below reserved" );
      eosio_assert( ram_size_for_accounts < _gstate.free_ram(), "have no enough ram for this accounts' count" );

      _gstate.max_accounts = max_accounts;
      _gstate.max_ram_size_for_accounts = ram_size_for_accounts;
      _global.set( _gstate, _self );
   }
   
   void system_contract::set_account_resource_limits(account_name account, uint64_t *ram, uint64_t *net, uint64_t *cpu){
      user_resources_table  userres( _self, account );
      auto res_itr = userres.find( account );

      const auto set_userres = [&]( auto& res ) {
         if( ram != nullptr) {
            int64_t delta = *ram - res.ram_bytes;
            _gstate.total_ram_bytes_reserved += delta;

            res.ram_bytes = *ram;
         }
         if( net != nullptr)
            res.net_weight = *net;
         if( cpu != nullptr)
            res.cpu_weight = *cpu;

         set_resource_limits(account, res.ram_bytes, res.net_weight, res.cpu_weight);
      };

      if( res_itr == userres.end() ){
         res_itr = userres.emplace( _self, [&]( auto& res ) {
            res.owner = account;
            set_userres( res );
         });
      }else{
         userres.modify( res_itr, _self, set_userres);
      }


   }

   void system_contract::init_account_resources(account_name account){

      eosio_assert( _gstate.account_ram_size <= _gstate.free_accounts_ram(), "system have no ram for new account" );
      _gstate.total_ram_bytes_reserved_for_accounts += _gstate.account_ram_size;
      _gstate.total_ram_bytes_reserved += _gstate.account_info_ram_size;

      uint64_t account_ram_size = _gstate.account_ram_size - _gstate.account_info_ram_size, net = 1, cpu = 1;
      set_account_resource_limits( account, &account_ram_size, &net, &cpu );
   }

   void system_contract::setaccntbw(account_name account, uint64_t net, uint64_t cpu){
      require_auth( N(eosio) );

      set_account_resource_limits( account, nullptr, &net, &cpu );
   }

   void system_contract::setaccntram(account_name account, uint64_t ram){
      require_auth( N(eosio) );
      eosio_assert( ram >= _gstate.account_ram_size - _gstate.account_info_ram_size, "ram be must more than minimal account ram size" );
      eosio_assert( ram <= SYSTEM_MAXIMUM_RAM, "ram is unrealistic" );

      user_resources_table  userres( _self, account );
      auto res_itr = userres.find( account );

      eosio_assert(res_itr != userres.end(), "account does not exists");

      int64_t delta = ram - res_itr->ram_bytes;
      if(delta < 0)
         delta = 0;

      eosio_assert( delta < _gstate.free_ram() - _gstate.free_accounts_ram(), "system have no such ram" );

      set_account_resource_limits( account, &ram );
   }


} //namespace eosiosystem
