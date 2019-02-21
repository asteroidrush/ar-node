#include "eosio.system.hpp"
#include <eosiolib/dispatcher.hpp>

#include "resource_management.cpp"
#include "producer_pay.cpp"
#include "voting.cpp"

/**
 * @addtogroup dispatcher
 * @{
 */
#define EOSIO_ABI_EX( TYPE, MEMBERS ) \
extern "C" { \
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
      auto self = receiver; \
      if( action == N(onerror)) { \
         /* onerror is only valid if it is for the "eosio" code account and authorized by "eosio"'s "active permission */ \
         eosio_assert(code == N(eosio), "onerror action's are only valid from the \"eosio\" system account"); \
      } \
      if( code == self && action != N(transfer) && action != N(issue) \
         || code == N(eosio.token) && (action == N(transfer) || action == N(issue)) \
         || action == N(onerror))  { \
         TYPE thiscontract( self ); \
         switch( action ) { \
            EOSIO_API( TYPE, MEMBERS ) \
         } \
         /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
      } \
   } \
} \
/// @}  dispatcher

namespace eosiosystem {

   system_contract::system_contract( account_name s )
   :native(s),
    _voters(_self,_self),
    _producers(_self,_self),
    _global(_self,_self)
   {
      //print( "construct system\n" );
      _gstate = _global.exists() ? _global.get() : get_default_parameters();
   }

   eosio_global_state system_contract::get_default_parameters() {
      eosio_global_state dp;
      get_blockchain_parameters(dp);
      return dp;
   }


   system_contract::~system_contract() {
      //print( "destruct system\n" );
      _global.set( _gstate, _self );
      //eosio_exit(0);
   }

   void system_contract::setparams( const eosio::blockchain_parameters& params ) {
      require_auth( _self );
      (eosio::blockchain_parameters&)(_gstate) = params;
      eosio_assert( 3 <= _gstate.max_authority_depth, "max_authority_depth should be at least 3" );
      set_blockchain_parameters( params );
   }

   void system_contract::setpriv( account_name account, uint8_t ispriv ) {
      require_auth( _self );
      set_privileged( account, ispriv );
   }

   void system_contract::rmvproducer( account_name producer ) {
      require_auth( _self );
      auto prod = _producers.find( producer );
      eosio_assert( prod != _producers.end(), "producer not found" );
      _producers.modify( prod, _self, [&](auto& p) {
            p.deactivate();
         });
   }
   
   void system_contract::require_be_stakeholder( account_name account ){
      balances accounts_table(N(eosio.token), account);
      accounts_table.get(eosio::symbol_type(CORE_SYMBOL).name(), "you must be stakeholder");
   }


   /**
    *  Called after a new account is created. This code enforces resource-limits rules
    *  for new accounts as well as new account naming conventions.
    *
    *  Account names containing '.' symbols must have a suffix equal to the name of the creator.
    *  This allows users who buy a premium name (shorter than 12 characters with no dots) to be the only ones
    *  who can create accounts with the creator's name as a suffix.
    *
    */
   void system_contract::newaccount( account_name     creator,
                            account_name     newact
                            /*  no need to parse authorities
                            const authority& owner,
                            const authority& active*/ ) {
      require_auth2(N(eosio), N(createaccnt));

      auto tmp = newact >> 4;
      uint32_t mask = 0;
      bool has_dot = false;

      for( int i = 0; i < 12; ++i ) {
         has_dot |= !(tmp & 0x1f);
         tmp >>= 5;
      }

      if( has_dot ) { // or is less than 12 characters
         auto prefix = eosio::name_prefix(newact);
         if(prefix != newact) // it means name has dot
            eosio_assert( creator == prefix, "only prefix may create this account" );
      }

      user_resources_table  userres( _self, newact);

      userres.emplace( _self, [&]( auto& res ) {
        res.owner = newact;
      });


      init_account_resources(newact);
   }

} /// eosio.system


EOSIO_ABI_EX( eosiosystem::system_contract,
     // native.hpp (newaccount definition is actually in eosio.system.cpp)
     (updateauth)(deleteauth)(linkauth)(unlinkauth)(canceldelay)(onerror)
     // eosio.system.cpp
     (newaccount)(setparams)(setpriv)(rmvproducer)
     // resource_management.cpp
     (setmaxram)(setmaxaccnts)(setaccntbw)(setaccntram)
     // voting.cpp
     (issue)(transfer)(regproducer)(unregprod)(voteproducer)(regproxy)
     // producer_pay.cpp
     (onblock)(claimrewards)
)
