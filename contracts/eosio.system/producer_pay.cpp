#include "eosio.system.hpp"

#include <eosio.token/eosio.token.hpp>

namespace eosiosystem {

   const int64_t  min_activated_stake   = 150'000'000'0000;
   const uint32_t blocks_per_year       = 52*7*24*2*3600;   // half seconds per year
   const uint32_t seconds_per_year      = 52*7*24*3600;
   const uint32_t blocks_per_day        = 2 * 24 * 3600;
   const uint32_t blocks_per_hour       = 2 * 3600;
   const uint64_t useconds_per_day      = 24 * 3600 * uint64_t(1000000);
   const uint64_t useconds_per_year     = seconds_per_year*1000000ll;


   void system_contract::onblock( block_timestamp timestamp, account_name producer ) {
      using namespace eosio;

      require_auth(N(eosio));

      /** until activated stake crosses this threshold no new rewards are paid */
      if( _gstate.total_activated_stake < min_activated_stake )
         return;

      if( _gstate.first_system_block_time == 0 ) {  /// start the presses
         _gstate.first_system_block_time = current_time();
         _gstate.last_pervote_bucket_fill = current_time();
      }


      /**
       * At startup the initial producer may not be one that is registered / elected
       * and therefore there may be no producer object for them.
       */
      auto prod = _producers.find(producer);
      if ( prod != _producers.end() ) {
         _gstate.total_unpaid_blocks++;
         _producers.modify( prod, 0, [&](auto& p ) {
               p.unpaid_blocks++;
         });
      }

      /// only update block producers once every minute, block_timestamp is in half seconds
      if( timestamp.slot - _gstate.last_producer_schedule_update.slot > 120 ) {
         update_elected_producers( timestamp );

         if( (timestamp.slot - _gstate.last_name_close.slot) > blocks_per_day ) {
            name_bid_table bids(_self,_self);
            auto idx = bids.get_index<N(highbid)>();
            auto highest = idx.begin();
            if( highest != idx.end() &&
                highest->high_bid > 0 &&
                highest->last_bid_time < (current_time() - useconds_per_day) &&
                _gstate.thresh_activated_stake_time > 0 &&
                (current_time() - _gstate.thresh_activated_stake_time) > 14 * useconds_per_day ) {
                   _gstate.last_name_close = timestamp;
                   idx.modify( highest, 0, [&]( auto& b ){
                         b.high_bid = -b.high_bid;
               });
            }
         }
      }
   }

   using namespace eosio;
   void system_contract::claimrewards( const account_name& owner ) {
      require_auth(owner);

      const auto& prod = _producers.get( owner );
      eosio_assert( prod.active(), "producer does not have an active key" );

      eosio_assert( _gstate.total_activated_stake >= min_activated_stake,
                    "cannot claim rewards until the chain is activated (at least 15% of all tokens participate in voting)" );

      auto ct = current_time();
      const auto time_since_last_claim = ct - (prod.last_claim_time > 0 ? prod.last_claim_time : _gstate.first_system_block_time);

      if( prod.last_claim_time > 0) {
         eosio_assert(time_since_last_claim > useconds_per_day, "already claimed rewards within past day");
      }

      const auto usecs_since_last_fill = ct - _gstate.last_pervote_bucket_fill;

      if( usecs_since_last_fill > 0 && _gstate.last_pervote_bucket_fill > 0 ) {
         auto new_tokens = static_cast<int64_t>( _gstate.payment_bucket_per_year * (double(usecs_since_last_fill) / double(useconds_per_year)) );

         auto to_per_block_pay   = new_tokens / 4;
         auto to_per_vote_pay    = new_tokens - to_per_block_pay;

         INLINE_ACTION_SENDER(eosio::token, issue)( N(eosio.token), {{N(eosio),N(active)}},
                                                    {N(eosio), asset(new_tokens, support_token_symbol), std::string("issue tokens for producer pay")} );

         INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio),N(active)},
                                                       { N(eosio), N(eosio.bpay), asset(to_per_block_pay, support_token_symbol), "fund per-block bucket" } );

         INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio),N(active)},
                                                       { N(eosio), N(eosio.vpay), asset(to_per_vote_pay, support_token_symbol), "fund per-vote bucket" } );

         _gstate.pervote_bucket  += to_per_vote_pay;
         _gstate.perblock_bucket += to_per_block_pay;

         _gstate.last_pervote_bucket_fill = ct;
      }

      int64_t producer_per_block_pay = 0;
      if( _gstate.total_unpaid_blocks > 0 ) {
         producer_per_block_pay = static_cast<int64_t>(_gstate.perblock_bucket * ( double(prod.unpaid_blocks) / double(_gstate.total_unpaid_blocks)));

         if( std::abs( producer_per_block_pay - _gstate.perblock_bucket ) <= 1){
            producer_per_block_pay = _gstate.perblock_bucket;
         }
      }
      int64_t producer_per_vote_pay = 0;
      if( _gstate.total_producer_vote_weight > 0 ) {
         int64_t tokens_since_last_claim = static_cast<int64_t>( _gstate.payment_bucket_per_year * (double(time_since_last_claim) / double(useconds_per_year)) );
         int64_t full_pervote_bucket = tokens_since_last_claim - tokens_since_last_claim / 4 ;
         producer_per_vote_pay  = int64_t(full_pervote_bucket * ( prod.total_votes  / _gstate.total_producer_vote_weight ) );

         if( std::abs( producer_per_vote_pay - _gstate.pervote_bucket ) <= 1 ){
            producer_per_vote_pay = _gstate.pervote_bucket;
         }
      }

      _gstate.pervote_bucket      -= producer_per_vote_pay;
      _gstate.perblock_bucket     -= producer_per_block_pay;
      _gstate.total_unpaid_blocks -= prod.unpaid_blocks;

      _producers.modify( prod, 0, [&](auto& p) {
          p.last_claim_time = ct;
          p.unpaid_blocks = 0;
      });

      if( producer_per_block_pay > 0 ) {
         INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio.bpay),N(active)},
                                                       { N(eosio.bpay), owner, asset(producer_per_block_pay, support_token_symbol), std::string("producer block pay") } );
      }
      if( producer_per_vote_pay > 0 ) {
         INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {N(eosio.vpay),N(active)},
                                                       { N(eosio.vpay), owner, asset(producer_per_vote_pay, support_token_symbol), std::string("producer vote pay") } );
      }
   }

} //namespace eosiosystem
