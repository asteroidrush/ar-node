/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>

#include <eosio.system/eosio.system.wast.hpp>
#include <eosio.system/eosio.system.abi.hpp>

#include <eosio.token/eosio.token.wast.hpp>
#include <eosio.token/eosio.token.abi.hpp>

#include <eosio.msig/eosio.msig.wast.hpp>
#include <eosio.msig/eosio.msig.abi.hpp>

#include <fc/variant_object.hpp>

using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

using mvo = fc::mutable_variant_object;

#ifndef TESTER
#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif
#endif

#define SPT_SYMBOL SY(4,SPT)
#define SPT_SYMBOL_NAME "SPT"

namespace eosio_system {

class eosio_system_tester : public TESTER {
public:

   eosio_system_tester()
   : eosio_system_tester([](TESTER& ) {}){}


   eosio::chain::asset spt_from_string(const std::string& s) {
      return eosio::chain::asset::from_string(s + " " SPT_SYMBOL_NAME);
   }

   template<typename Lambda>
   eosio_system_tester(Lambda setup) {
      setup(*this);

      produce_blocks( 2 );

      create_accounts({ N(eosio.token), N(eosio.bpay), N(eosio.vpay), N(eosio.names) });


      produce_blocks( 100 );

      set_code( N(eosio.token), eosio_token_wast );
      set_abi( N(eosio.token), eosio_token_abi );

      {
         const auto& accnt = control->db().get<account_object,by_name>( N(eosio.token) );
         abi_def abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
         token_abi_ser.set_abi(abi, abi_serializer_max_time);
      }

      set_code( config::system_account_name, eosio_system_wast );
      set_abi( config::system_account_name, eosio_system_abi );

      create_currency( N(eosio.token), config::system_account_name, core_from_string("10000000000.0000") );
      issue(config::system_account_name,      core_from_string("1000000000.0000"));
      BOOST_REQUIRE_EQUAL( core_from_string("1000000000.0000"), get_balance( "eosio" ) );

      create_currency( N(eosio.token), config::system_account_name, spt_from_string("10000000000.0000") );

      {
         const auto& accnt = control->db().get<account_object,by_name>( config::system_account_name );
         abi_def abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
         abi_ser.set_abi(abi, abi_serializer_max_time);
      }

      produce_blocks();

      create_account_with_resources( N(alice1111111), config::system_account_name, false );
      create_account_with_resources( N(bob111111111), config::system_account_name, false );
      create_account_with_resources( N(carol1111111), config::system_account_name, false );

      BOOST_REQUIRE_EQUAL( core_from_string("1000000000.0000"), get_balance("eosio") );
   }


   void create_accounts_with_resources( vector<account_name> accounts, account_name creator = config::system_account_name ) {
      for( auto a : accounts ) {
         create_account_with_resources( a, creator );
      }
   }

   transaction_trace_ptr create_account_with_resources( account_name a, account_name creator ) {
      signed_transaction trx;
      set_transaction_headers(trx);

      authority owner_auth;
      owner_auth =  authority( get_public_key( a, "owner" ) );

      trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                newaccount{
                                   .creator  = creator,
                                   .name     = a,
                                   .owner    = owner_auth,
                                   .active   = authority( get_public_key( a, "active" ) )
                                });

      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );

      const transaction_trace_ptr trace = push_transaction( trx );
      setram(a, 8000);

      return trace;
   }

   transaction_trace_ptr create_account_with_resources( account_name a, account_name creator, bool multisig, int64_t ram = 8000, int64_t net = 1, int64_t cpu = 1 ) {
      signed_transaction trx;
      set_transaction_headers(trx);

      authority owner_auth;
      if (multisig) {
         // multisig between account's owner key and creators active permission
         owner_auth = authority(2, {key_weight{get_public_key( a, "owner" ), 1}}, {permission_level_weight{{creator, config::active_name}, 1}});
      } else {
         owner_auth =  authority( get_public_key( a, "owner" ) );
      }

      trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                newaccount{
                                   .creator  = creator,
                                   .name     = a,
                                   .owner    = owner_auth,
                                   .active   = authority( get_public_key( a, "active" ) )
                                });

      trx.actions.emplace_back( get_action(config::system_account_name, N(setaccntram), vector<permission_level>{{config::system_account_name,config::active_name}},
                                mvo()
                                ("account", a)
                                ("ram", ram)
                                ));

      trx.actions.emplace_back( get_action(config::system_account_name, N(setaccntbw), vector<permission_level>{{config::system_account_name,config::active_name}},
                                mvo()
                                ("account", a)
                                ("net", net)
                                ("cpu", cpu)
                                ));

      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );
      return push_transaction( trx );
   }

   transaction_trace_ptr setup_producer_accounts( const std::vector<account_name>& accounts ) {
      int64_t ram = 8000, net = 1, cpu = 1;
      account_name creator(config::system_account_name);
      signed_transaction trx;
      set_transaction_headers(trx);

      for (const auto& a: accounts) {
         authority owner_auth( get_public_key( a, "owner" ) );
         trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                   newaccount{
                                         .creator  = creator,
                                         .name     = a,
                                         .owner    = owner_auth,
                                         .active   = authority( get_public_key( a, "active" ) )
                                         });

         trx.actions.emplace_back( get_action(config::system_account_name, N(setaccntram), vector<permission_level>{{config::system_account_name,config::active_name}},
                                              mvo()
                                              ("account", a)
                                              ("ram", ram)
         ));

         trx.actions.emplace_back( get_action(config::system_account_name, N(setaccntbw), vector<permission_level>{{config::system_account_name,config::active_name}},
                                              mvo()
                                              ("account", a)
                                              ("net", net)
                                              ("cpu", cpu)
         ));
      }

      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );
      return push_transaction( trx );
   }

   action_result setram( const account_name& account, int64_t ram ) {
      return push_action( config::system_account_name, N(setaccntram), mvo()( "account",account)("ram",ram) );
   }

   action_result setbw( const account_name& account, int64_t net, int64_t cpu ) {
      return push_action( config::system_account_name, N(setaccntbw), mvo()( "account",account)("net",net)("cpu", cpu) );
   }

   action_result push_action( const account_name& signer, const action_name &name, const variant_object &data,
                              bool auth = true ) {
         string action_type_name = abi_ser.get_action_type(name);

         action act;
         act.account = config::system_account_name;
         act.name = name;
         act.data = abi_ser.variant_to_binary( action_type_name, data, abi_serializer_max_time );

         return base_tester::push_action( std::move(act), auth ? uint64_t(signer) : signer == N(bob111111111) ? N(alice1111111) : N(bob111111111) );
   }

   action_result bidname( const account_name& bidder, const account_name& newname, const asset& bid ) {
      return push_action( name(bidder), N(bidname), mvo()
                          ("bidder",  bidder)
                          ("newname", newname)
                          ("bid", bid)
                          );
   }

   static fc::variant_object producer_parameters_example( int n ) {
      return mutable_variant_object()
         ("max_block_net_usage", 10000000 + n )
         ("target_block_net_usage_pct", 10 + n )
         ("max_transaction_net_usage", 1000000 + n )
         ("base_per_transaction_net_usage", 100 + n)
         ("net_usage_leeway", 500 + n )
         ("context_free_discount_net_usage_num", 1 + n )
         ("context_free_discount_net_usage_den", 100 + n )
         ("max_block_cpu_usage", 10000000 + n )
         ("target_block_cpu_usage_pct", 10 + n )
         ("max_transaction_cpu_usage", 1000000 + n )
         ("min_transaction_cpu_usage", 100 + n )
         ("max_transaction_lifetime", 3600 + n)
         ("deferred_trx_expiration_window", 600 + n)
         ("max_transaction_delay", 10*86400+n)
         ("max_inline_action_size", 4096 + n)
         ("max_inline_action_depth", 4 + n)
         ("max_authority_depth", 6 + n)
         ("max_ram_size", (n % 10 + 1) * 1024 * 1024)
         ("ram_reserve_ratio", 100 + n);
   }

   action_result regproducer( const account_name& acnt, int params_fixture = 1 ) {
      action_result r = push_action( acnt, N(regproducer), mvo()
                          ("producer",  acnt )
                          ("producer_key", get_public_key( acnt, "active" ) )
                          ("url", "" )
                          ("location", 0 )
      );
      BOOST_REQUIRE_EQUAL( success(), r);
      return r;
   }

   action_result vote( const account_name& voter, const std::vector<account_name>& producers, const account_name& proxy = name(0) ) {
      return push_action(voter, N(voteproducer), mvo()
                         ("voter",     voter)
                         ("proxy",     proxy)
                         ("producers", producers));
   }

   uint32_t last_block_time() const {
      return time_point_sec( control->head_block_time() ).sec_since_epoch();
   }

   asset get_balance( const account_name& act, const uint64_t sym = CORE_SYMBOL ) {
      vector<char> data = get_row_by_account( N(eosio.token), act, N(accounts), symbol(sym).to_symbol_code().value );
      return data.empty() ? asset(0, symbol(sym)) : token_abi_ser.binary_to_variant("account", data, abi_serializer_max_time)["balance"].as<asset>();
   }

   fc::variant get_total_stake( const account_name& act ) {
      vector<char> data = get_row_by_account( config::system_account_name, act, N(userres), act );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "user_resources", data, abi_serializer_max_time );
   }

   fc::variant get_voter_info( const account_name& act ) {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(voters), act );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "voter_info", data, abi_serializer_max_time );
   }

   fc::variant get_producer_info( const account_name& act ) {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(producers), act );
      return abi_ser.binary_to_variant( "producer_info", data, abi_serializer_max_time );
   }

   void create_currency( name contract, name manager, asset maxsupply ) {
      auto act =  mutable_variant_object()
         ("issuer",       manager )
         ("maximum_supply", maxsupply );

      base_tester::push_action(contract, N(create), contract, act );
   }

   action_result issue( name to, const asset& amount, name issuer = config::system_account_name ) {
      string action_type_name = token_abi_ser.get_action_type(N(issue));

      action act;
      act.account = N(eosio.token);
      act.name = N(issue);
      act.data = token_abi_ser.variant_to_binary( action_type_name,  mvo()
            ("to",      to )
            ("quantity", amount )
            ("memo", ""), abi_serializer_max_time );

      return base_tester::push_action( std::move(act), issuer );
   }
   action_result transfer( name from, name to, const asset& amount ) {
      string action_type_name = token_abi_ser.get_action_type(N(transfer));

      action act;
      act.account = N(eosio.token);
      act.name = N(transfer);
      act.data = token_abi_ser.variant_to_binary( action_type_name,  mvo()
            ("from",    from)
            ("to",      to )
            ("quantity", amount)
            ("memo", ""), abi_serializer_max_time );

      return base_tester::push_action( std::move(act), from );
   }

   double stake2votes( asset stake ) {
      auto now = control->pending_block_time().time_since_epoch().count() / 1000000;
      return stake.get_amount() * pow(2, int64_t((now - (config::block_timestamp_epoch / 1000)) / (86400 * 7))/ double(52) ); // 52 week periods (i.e. ~years)
   }

   double stake2votes( const string& s ) {
      return stake2votes( core_from_string(s) );
   }

   fc::variant get_stats( const string& symbolname ) {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      vector<char> data = get_row_by_account( N(eosio.token), symbol_code, N(stat), symbol_code );
      return data.empty() ? fc::variant() : token_abi_ser.binary_to_variant( "currency_stats", data, abi_serializer_max_time );
   }

   asset get_token_supply(const string symbol_name = CORE_SYMBOL_NAME) {
      return get_stats("4," + symbol_name)["supply"].as<asset>();
   }

   fc::variant get_global_state() {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(global), N(global) );
      if (data.empty()) std::cout << "\nData is empty\n" << std::endl;
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "eosio_global_state", data, abi_serializer_max_time );

   }

   fc::variant get_refund_request( name account ) {
      vector<char> data = get_row_by_account( config::system_account_name, account, N(refunds), account );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "refund_request", data, abi_serializer_max_time );
   }

   abi_serializer initialize_multisig() {
      abi_serializer msig_abi_ser;
      {
         create_account_with_resources( N(eosio.msig), config::system_account_name );
         BOOST_REQUIRE_EQUAL( success(), setram( "eosio.msig", 500000 ) );
         produce_block();

         auto trace = base_tester::push_action(config::system_account_name, N(setpriv),
                                               config::system_account_name,  mutable_variant_object()
                                               ("account", "eosio.msig")
                                               ("is_priv", 1)
         );

         set_code( N(eosio.msig), eosio_msig_wast );
         set_abi( N(eosio.msig), eosio_msig_abi );

         produce_blocks();
         const auto& accnt = control->db().get<account_object,by_name>( N(eosio.msig) );
         abi_def msig_abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, msig_abi), true);
         msig_abi_ser.set_abi(msig_abi, abi_serializer_max_time);
      }
      return msig_abi_ser;
   }

   vector<name> active_and_vote_producers() {
      //stake more than 15% of total EOS supply to activate chain
      BOOST_REQUIRE_EQUAL( success(), transfer( "eosio", "alice1111111", core_from_string("300000000.0000") ) );

      // create accounts {defproducera, defproducerb, ..., defproducerz} and register as producers
      std::vector<account_name> producer_names;
      {
         producer_names.reserve('z' - 'a' + 1);
         const std::string root("defproducer");
         for ( char c = 'a'; c < 'a'+21; ++c ) {
            producer_names.emplace_back(root + std::string(1, c));
         }
         setup_producer_accounts(producer_names);
         for (const auto& p: producer_names) {

            BOOST_REQUIRE_EQUAL( success(), regproducer(p) );
         }
      }
      produce_blocks( 250);

      auto trace_auth = TESTER::push_action(config::system_account_name, updateauth::get_name(), config::system_account_name, mvo()
                                            ("account", name(config::system_account_name).to_string())
                                            ("permission", name(config::active_name).to_string())
                                            ("parent", name(config::owner_name).to_string())
                                            ("auth",  authority(1, {key_weight{get_public_key( config::system_account_name, "active" ), 1}}, {
                                                  permission_level_weight{{config::system_account_name, config::eosio_code_name}, 1},
                                                     permission_level_weight{{config::producers_account_name,  config::active_name}, 1}
                                               }
                                            ))
      );
      BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace_auth->receipt->status);

      //vote for producers
      {
         BOOST_REQUIRE_EQUAL(success(), setram( "alice1111111", 300000 ) );
         BOOST_REQUIRE_EQUAL(success(), push_action(N(alice1111111), N(voteproducer), mvo()
                                                    ("voter",  "alice1111111")
                                                    ("proxy", name(0).to_string())
                                                    ("producers", vector<account_name>(producer_names.begin(), producer_names.begin()+21))
                             )
         );
      }
      produce_blocks( 250 );

      auto producer_keys = control->head_block_state()->active_schedule.producers;
      BOOST_REQUIRE_EQUAL( 21, producer_keys.size() );
      BOOST_REQUIRE_EQUAL( name("defproducera"), producer_keys[0].producer_name );

      return producer_names;
   }

   void cross_15_percent_threshold() {
      setup_producer_accounts({N(producer1111)});
      regproducer(N(producer1111));
      {
         signed_transaction trx;
         set_transaction_headers(trx);

         trx.actions.emplace_back( get_action( N(eosio.token), N(transfer),
                                               vector<permission_level>{{config::system_account_name, config::active_name}},
                                               mvo()
                                               ("from", name{config::system_account_name})
                                               ("to", "producer1111")
                                               ("quantity", core_from_string("150000000.0000"))
                                               ("memo", "")
                                             )
                                 );
         trx.actions.emplace_back( get_action( config::system_account_name, N(voteproducer),
                                               vector<permission_level>{{N(producer1111), config::active_name}},
                                               mvo()
                                               ("voter", "producer1111")
                                               ("proxy", name(0).to_string())
                                               ("producers", vector<account_name>(1, N(producer1111)))
                                             )
                                 );
         trx.actions.emplace_back( get_action( N(eosio.token), N(transfer),
                                               vector<permission_level>{{N(producer1111), config::active_name}},
                                               mvo()
                                               ("from", "producer1111")
                                               ("to", name{config::system_account_name})
                                               ("quantity", core_from_string("150000000.0000"))
                                               ("memo", "")
                                              )
                                 );

         set_transaction_headers(trx);
         trx.sign( get_private_key( config::system_account_name, "active" ), control->get_chain_id()  );
         trx.sign( get_private_key( N(producer1111), "active" ), control->get_chain_id()  );
         push_transaction( trx );
      }
   }

   abi_serializer abi_ser;
   abi_serializer token_abi_ser;
};

inline fc::mutable_variant_object voter( account_name acct ) {
   return mutable_variant_object()
      ("owner", acct)
      ("proxy", name(0).to_string())
      ("producers", variants() )
      ("staked", int64_t(0))
      //("last_vote_weight", double(0))
      ("proxied_vote_weight", double(0))
      ("is_proxy", 0)
      ;
}

inline fc::mutable_variant_object voter( account_name acct, const asset& vote_stake ) {
   return voter( acct )( "staked", vote_stake.get_amount() );
}

inline fc::mutable_variant_object voter( account_name acct, int64_t vote_stake ) {
   return voter( acct )( "staked", vote_stake );
}

inline fc::mutable_variant_object proxy( account_name acct ) {
   return voter( acct )( "is_proxy", 1 );
}

inline uint64_t M( const string& eos_str ) {
   return core_from_string( eos_str ).get_amount();
}

}
