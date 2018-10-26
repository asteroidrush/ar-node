#include <boost/test/unit_test.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/wast_to_wasm.hpp>

#include <Runtime/Runtime.h>

#include "eosio_system_tester.hpp"

using namespace eosio_system;

BOOST_AUTO_TEST_SUITE(eosio_system_tests)

BOOST_FIXTURE_TEST_CASE( set_resources, eosio_system_tester ) try {
   cross_15_percent_threshold();

   produce_blocks( 1 );

   BOOST_REQUIRE_EQUAL( core_from_string("0.0001"), get_balance( "alice1111111" ) );
   transfer( "eosio", "alice1111111", core_from_string("999.9999"));

   BOOST_REQUIRE_EQUAL( core_from_string("1000.0000"), get_balance( "alice1111111" ) );

   BOOST_REQUIRE_EQUAL( error("missing authority of eosio"), setram("alice1111111", 6*1024, "alice1111111") );
   BOOST_REQUIRE_EQUAL( error("missing authority of eosio"), setbw("alice1111111", 5, 5, "alice1111111") );

   auto total = get_total_stake("alice1111111");
   BOOST_REQUIRE_EQUAL( 1, total["net_weight"].as<int64_t>());
   BOOST_REQUIRE_EQUAL( 1, total["cpu_weight"].as<int64_t>());

   BOOST_REQUIRE_EQUAL( success(), setram("alice1111111", 6*1024) );
   BOOST_REQUIRE_EQUAL( success(), setbw("alice1111111", 5, 5) );

   produce_blocks(1);

   total = get_total_stake("alice1111111");
   BOOST_REQUIRE_EQUAL( 6*1024, total["ram_bytes"].as_uint64());
   BOOST_REQUIRE_EQUAL( 5, total["net_weight"].as_uint64());
   BOOST_REQUIRE_EQUAL( 5, total["cpu_weight"].as_uint64());

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( max_accounts_test, eosio_system_tester ) try {
   cross_15_percent_threshold();

   BOOST_CHECK_EQUAL( success(), set_max_accounts(10) );
   produce_blocks( 1 );

   vector<char> alpha = {'1', '2', '3', '4', '5', 'a', 'b', 'c', 'd', 'e'};
   for(int i=0; i < 10 - 4; i++){
        string index = "";
        int divider = i + 1;
        while( divider > 0){
           index += alpha[divider % 10];
           divider /= 10;
        }
        auto acc_name = string_to_name(string("alice" + index).c_str());
   	  create_account_with_resources( acc_name, config::system_account_name, false, 8000, 1, 1, 1);

        produce_blocks( 1 );
   }

   BOOST_CHECK_THROW(create_account_with_resources( "finalacc", config::system_account_name, false, 8000, 1, 1, 1), eosio_assert_message_exception);

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "attempt to set max accounts below reserved"), set_max_accounts(5) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "have no enough ram for this accounts' count"), set_max_accounts(5'000'000'000) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( fail_without_auth, eosio_system_tester ) try {
   cross_15_percent_threshold();

   BOOST_REQUIRE_EQUAL( error("missing authority of eosio"),
                        push_action( N(alice1111111), N(setaccntram), mvo()
                                    ("account",     "alice1111111")
                                    ("ram", 6*1024)
                        )
   );

   BOOST_REQUIRE_EQUAL( error("missing authority of eosio"),
                        push_action(N(alice1111111), N(setaccntbw), mvo()
                                    ("account",     "alice1111111")
                                    ("net", 5)
                                    ("cpu", 5)
                        )
   );
   //REQUIRE_MATCHING_OBJECT( , get_voter_info( "alice1111111" ) );
} FC_LOG_AND_RETHROW()




// Tests for voting
BOOST_FIXTURE_TEST_CASE( producer_register_unregister, eosio_system_tester ) try {
   issue( "alice1111111", core_from_string("999.9999"),  config::system_account_name );

   //fc::variant params = producer_parameters_example(1);
   auto key =  fc::crypto::public_key( std::string("EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV") );
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice1111111), N(regproducer), mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", key )
                                               ("url", "http://block.one")
                                               ("location", 1)
                        )
   );

   auto info = get_producer_info( "alice1111111" );
   BOOST_REQUIRE_EQUAL( "alice1111111", info["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, info["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( "http://block.one", info["url"].as_string() );

   //change parameters one by one to check for things like #3783
   //fc::variant params2 = producer_parameters_example(2);
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice1111111), N(regproducer), mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", key )
                                               ("url", "http://block.two")
                                               ("location", 1)
                        )
   );
   info = get_producer_info( "alice1111111" );
   BOOST_REQUIRE_EQUAL( "alice1111111", info["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( key, fc::crypto::public_key(info["producer_key"].as_string()) );
   BOOST_REQUIRE_EQUAL( "http://block.two", info["url"].as_string() );
   BOOST_REQUIRE_EQUAL( 1, info["location"].as_int64() );

   auto key2 =  fc::crypto::public_key( std::string("EOS5jnmSKrzdBHE9n8hw58y7yxFWBC8SNiG7m8S1crJH3KvAnf9o6") );
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice1111111), N(regproducer), mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", key2 )
                                               ("url", "http://block.two")
                                               ("location", 2)
                        )
   );
   info = get_producer_info( "alice1111111" );
   BOOST_REQUIRE_EQUAL( "alice1111111", info["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( key2, fc::crypto::public_key(info["producer_key"].as_string()) );
   BOOST_REQUIRE_EQUAL( "http://block.two", info["url"].as_string() );
   BOOST_REQUIRE_EQUAL( 2, info["location"].as_int64() );

   //unregister producer
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice1111111), N(unregprod), mvo()
                                               ("producer",  "alice1111111")
                        )
   );
   info = get_producer_info( "alice1111111" );
   //key should be empty
   BOOST_REQUIRE_EQUAL( fc::crypto::public_key(), fc::crypto::public_key(info["producer_key"].as_string()) );
   //everything else should stay the same
   BOOST_REQUIRE_EQUAL( "alice1111111", info["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, info["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( "http://block.two", info["url"].as_string() );

   //unregister bob111111111 who is not a producer
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "producer not found" ),
                        push_action( N(bob111111111), N(unregprod), mvo()
                                     ("producer",  "bob111111111")
                        )
   );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( vote_for_producer, eosio_system_tester, * boost::unit_test::tolerance(1e+5) ) try {
   cross_15_percent_threshold();

   create_account_with_resources("badproducer", config::system_account_name, false, 8000, 1, 1, 0);
   BOOST_REQUIRE_EQUAL(error("assertion failure with message: you must be stakeholder"), push_action( N(badproducer), N(regproducer), mvo()
            ("producer",  "badproducer")
            ("producer_key", get_public_key( N(badproducer), "active") )
            ("url", "http://block123.one")
            ("location", 0 )
    )
   );

   create_account_with_resources("badvoter", config::system_account_name, false, 8000, 1, 1, 0);
   BOOST_REQUIRE_EQUAL(error("assertion failure with message: you must be stakeholder"), vote(N(badvoter), {N(producer1111)}));

   issue( "alice1111111", core_from_string("999.9999"),  config::system_account_name );
   fc::variant params = producer_parameters_example(1);
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproducer), mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key( N(alice1111111), "active") )
                                               ("url", "http://block.one")
                                               ("location", 0 )
                        )
   );
   auto prod = get_producer_info( "alice1111111" );
   BOOST_REQUIRE_EQUAL( "alice1111111", prod["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( 0, prod["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( "http://block.one", prod["url"].as_string() );

   //bob111111111 makes stake
   BOOST_REQUIRE_EQUAL( success(), issue( "bob111111111", core_from_string("1999.9999"),  config::system_account_name ) );
   BOOST_REQUIRE_EQUAL( core_from_string("2000.0000"), get_balance( "bob111111111" ) );
   REQUIRE_MATCHING_OBJECT( voter( "bob111111111", 2000'0000 ), get_voter_info( "bob111111111" ) );

   //bob111111111 votes for alice1111111
   BOOST_REQUIRE_EQUAL( success(), vote( N(bob111111111), { N(alice1111111) } ) );

   //check that producer parameters stay the same after voting
   prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( 2000'0000 == prod["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( "alice1111111", prod["owner"].as_string() );
   BOOST_REQUIRE_EQUAL( "http://block.one", prod["url"].as_string() );

   //carol1111111 makes stake
   BOOST_REQUIRE_EQUAL( success(), issue( "carol1111111", core_from_string("2999.9999"),  config::system_account_name ) );
   REQUIRE_MATCHING_OBJECT( voter( "carol1111111", 3000'0000), get_voter_info( "carol1111111" ) );
   BOOST_REQUIRE_EQUAL( core_from_string("3000.0000"), get_balance( "carol1111111" ) );
   //carol1111111 votes for alice1111111
   BOOST_REQUIRE_EQUAL( success(), vote( N(carol1111111), { N(alice1111111) } ) );

   //new stake votes be added to alice1111111's total_votes
   prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( 5000'0000 == prod["total_votes"].as_double() );

   //bob111111111 increases his stake
   BOOST_REQUIRE_EQUAL( success(), issue( "bob111111111", core_from_string("2000.0000"),  config::system_account_name ) );
   //alice1111111 stake with transfer to bob111111111
   BOOST_REQUIRE_EQUAL( success(), transfer( "alice1111111", "bob111111111", core_from_string("1000.0000")) );
   //should increase alice1111111's total_votes
   prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( 8000'0000 == prod["total_votes"].as_double() );

   //carol1111111 unstakes part of the stake
   BOOST_REQUIRE_EQUAL( success(), transfer( "carol1111111", "alice1111111", core_from_string("2.0000"))/*"2.0000 EOS", "0.0002 EOS"*/ );

   //should decrease alice1111111's total_votes
   prod = get_producer_info( "alice1111111" );
   wdump((prod));
   BOOST_TEST_REQUIRE( 7998'0000 == prod["total_votes"].as_double() );

   //bob111111111 revokes his vote
   BOOST_REQUIRE_EQUAL( success(), vote( N(bob111111111), vector<account_name>() ) );

   //should decrease alice1111111's total_votes
   prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( 2998'0000 == prod["total_votes"].as_double() );

   //carol1111111 unstakes rest of eos
   BOOST_REQUIRE_EQUAL( success(), transfer( "carol1111111", "bob111111111", core_from_string("2998.0000") ) );
   //should decrease alice1111111's total_votes to zero
   prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( 0.0 == prod["total_votes"].as_double() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( unregistered_producer_voting, eosio_system_tester, * boost::unit_test::tolerance(1e+5) ) try {
   BOOST_REQUIRE_EQUAL( success(), issue( "bob111111111", core_from_string("1999.9999"),  config::system_account_name ) );
   REQUIRE_MATCHING_OBJECT( voter( "bob111111111", 2000'0000 ), get_voter_info( "bob111111111" ) );

   //bob111111111 should not be able to vote for alice1111111 who is not a producer
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "producer is not registered" ),
                        vote( N(bob111111111), { N(alice1111111) } ) );

   //alice1111111 registers as a producer
   issue( "alice1111111", core_from_string("999.9999"),  config::system_account_name );
   fc::variant params = producer_parameters_example(1);
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproducer), mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key( N(alice1111111), "active") )
                                               ("url", "")
                                               ("location", 0)
                        )
   );
   //and then unregisters
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(unregprod), mvo()
                                               ("producer",  "alice1111111")
                        )
   );
   //key should be empty
   auto prod = get_producer_info( "alice1111111" );
   BOOST_REQUIRE_EQUAL( fc::crypto::public_key(), fc::crypto::public_key(prod["producer_key"].as_string()) );

   //bob111111111 should not be able to vote for alice1111111 who is an unregistered producer
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "producer is not currently registered" ),
                        vote( N(bob111111111), { N(alice1111111) } ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( more_than_30_producer_voting, eosio_system_tester ) try {
   BOOST_REQUIRE_EQUAL( success(), issue( "bob111111111", core_from_string("1999.9999"),  config::system_account_name ) );
   REQUIRE_MATCHING_OBJECT( voter( "bob111111111", 2000'0000 ), get_voter_info( "bob111111111" ) );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "attempt to vote for too many producers" ),
                        vote( N(bob111111111), vector<account_name>(31, N(alice1111111)) ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( vote_same_producer_30_times, eosio_system_tester ) try {
   BOOST_REQUIRE_EQUAL( success(), issue( "bob111111111", core_from_string("1999.9999"),  config::system_account_name )  );
   REQUIRE_MATCHING_OBJECT( voter( "bob111111111", 2000'0000 ), get_voter_info( "bob111111111" ) );

   //alice1111111 becomes a producer
   issue( "alice1111111", core_from_string("999.9999"),  config::system_account_name );
   fc::variant params = producer_parameters_example(1);
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproducer), mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key(N(alice1111111), "active") )
                                               ("url", "")
                                               ("location", 0)
                        )
   );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "producer votes must be unique and sorted" ),
                        vote( N(bob111111111), vector<account_name>(30, N(alice1111111)) ) );

   auto prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( 0 == prod["total_votes"].as_double() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( producer_keep_votes, eosio_system_tester, * boost::unit_test::tolerance(1e+5) ) try {
   issue( "alice1111111", core_from_string("999.9999"),  config::system_account_name );
   fc::variant params = producer_parameters_example(1);
   vector<char> key = fc::raw::pack( get_public_key( N(alice1111111), "active" ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproducer), mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key( N(alice1111111), "active") )
                                               ("url", "")
                                               ("location", 0)
                        )
   );

   //bob111111111 makes stake
   BOOST_REQUIRE_EQUAL( success(), issue( "bob111111111", core_from_string("1999.9999"),  config::system_account_name ) );
   REQUIRE_MATCHING_OBJECT( voter( "bob111111111", 2000'0000 ), get_voter_info( "bob111111111" ) );

   //bob111111111 votes for alice1111111
   BOOST_REQUIRE_EQUAL( success(), vote(N(bob111111111), { N(alice1111111) } ) );

   auto prod = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( 2000'0000 == prod["total_votes"].as_double() );

   //unregister producer
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice1111111), N(unregprod), mvo()
                                               ("producer",  "alice1111111")
                        )
   );
   prod = get_producer_info( "alice1111111" );
   //key should be empty
   BOOST_REQUIRE_EQUAL( fc::crypto::public_key(), fc::crypto::public_key(prod["producer_key"].as_string()) );
   //check parameters just in case
   //REQUIRE_MATCHING_OBJECT( params, prod["prefs"]);
   //votes should stay the same
   BOOST_TEST_REQUIRE( 2000'0000, prod["total_votes"].as_double() );

   //regtister the same producer again
   params = producer_parameters_example(2);
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproducer), mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key( N(alice1111111), "active") )
                                               ("url", "")
                                               ("location", 0)
                        )
   );
   prod = get_producer_info( "alice1111111" );
   //votes should stay the same
   BOOST_TEST_REQUIRE( 2000'0000, prod["total_votes"].as_double() );

   //change parameters
   params = producer_parameters_example(3);
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproducer), mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key( N(alice1111111), "active") )
                                               ("url","")
                                               ("location", 0)
                        )
   );
   prod = get_producer_info( "alice1111111" );
   //votes should stay the same
   BOOST_TEST_REQUIRE( 2000'0000, prod["total_votes"].as_double() );
   //check parameters just in case
   //REQUIRE_MATCHING_OBJECT( params, prod["prefs"]);

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( vote_for_two_producers, eosio_system_tester, * boost::unit_test::tolerance(1e+5) ) try {
   //alice1111111 becomes a producer
   fc::variant params = producer_parameters_example(1);
   auto key = get_public_key( N(alice1111111), "active" );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproducer), mvo()
                                               ("producer",  "alice1111111")
                                               ("producer_key", get_public_key( N(alice1111111), "active") )
                                               ("url","")
                                               ("location", 0)
                        )
   );
   //bob111111111 becomes a producer
   params = producer_parameters_example(2);
   key = get_public_key( N(bob111111111), "active" );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob111111111), N(regproducer), mvo()
                                               ("producer",  "bob111111111")
                                               ("producer_key", get_public_key( N(alice1111111), "active") )
                                               ("url","")
                                               ("location", 0)
                        )
   );

   //carol1111111 votes for alice1111111 and bob111111111
   BOOST_REQUIRE_EQUAL( success(), issue( "carol1111111", core_from_string("999.9999"),  config::system_account_name ) );
   BOOST_REQUIRE_EQUAL( success(), vote( N(carol1111111), { N(alice1111111), N(bob111111111) } ) );

   auto alice_info = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( 1000'0000 == alice_info["total_votes"].as_double() );
   auto bob_info = get_producer_info( "bob111111111" );
   BOOST_TEST_REQUIRE( 1000'0000  == bob_info["total_votes"].as_double() );

   //carol1111111 votes for alice1111111 (but revokes vote for bob111111111)
   BOOST_REQUIRE_EQUAL( success(), vote( N(carol1111111), { N(alice1111111) } ) );

   alice_info = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( 1000'0000  == alice_info["total_votes"].as_double() );
   bob_info = get_producer_info( "bob111111111" );
   BOOST_TEST_REQUIRE( 0 == bob_info["total_votes"].as_double() );

   //alice1111111 votes for herself and bob111111111
   BOOST_REQUIRE_EQUAL( success(), issue( "alice1111111", core_from_string("1.9999"),  config::system_account_name ) );
   BOOST_REQUIRE_EQUAL( success(), vote(N(alice1111111), { N(alice1111111), N(bob111111111) } ) );

   alice_info = get_producer_info( "alice1111111" );
   BOOST_TEST_REQUIRE( 1002'0000 == alice_info["total_votes"].as_double() );

   bob_info = get_producer_info( "bob111111111" );
   BOOST_TEST_REQUIRE( 2'0000 == bob_info["total_votes"].as_double() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_register_unregister_keeps_stake, eosio_system_tester ) try {
   create_account_with_resources("badproxy", config::system_account_name, false, 8000, 1, 1, 0);
   BOOST_REQUIRE_EQUAL(error("assertion failure with message: you must be stakeholder"), push_action(N(badproxy), N(regproxy), mvo()
            ("proxy",  "badproxy")
            ("isproxy", true )
      )
   );


   //register proxy by first action for this user ever
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice1111111), N(regproxy), mvo()
                                               ("proxy",  "alice1111111")
                                               ("isproxy", true )
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" ), get_voter_info( "alice1111111" ) );

   //unregister proxy
   BOOST_REQUIRE_EQUAL( success(), push_action(N(alice1111111), N(regproxy), mvo()
                                               ("proxy",  "alice1111111")
                                               ("isproxy", false)
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111" ), get_voter_info( "alice1111111" ) );

   //stake and then register as a proxy
   BOOST_REQUIRE_EQUAL( success(), issue( "bob111111111", core_from_string("999.9999"),  config::system_account_name ) );
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob111111111), N(regproxy), mvo()
                                               ("proxy",  "bob111111111")
                                               ("isproxy", true)
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "bob111111111" )( "staked", 1000'0000 ), get_voter_info( "bob111111111" ) );
   //unrgister and check that stake is still in place
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob111111111), N(regproxy), mvo()
                                               ("proxy",  "bob111111111")
                                               ("isproxy", false)
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "bob111111111", 1000'0000 ), get_voter_info( "bob111111111" ) );

   //register as a proxy and then stake
   BOOST_REQUIRE_EQUAL( success(), push_action( N(carol1111111), N(regproxy), mvo()
                                               ("proxy",  "carol1111111")
                                               ("isproxy", true)
                        )
   );
   BOOST_REQUIRE_EQUAL( success(), issue( "carol1111111", core_from_string("999.9999"),  config::system_account_name ) );
   //check that both proxy flag and stake a correct
   REQUIRE_MATCHING_OBJECT( proxy( "carol1111111" )( "staked", 1000'0000 ), get_voter_info( "carol1111111" ) );

   //unregister
   BOOST_REQUIRE_EQUAL( success(), push_action( N(carol1111111), N(regproxy), mvo()
                                                ("proxy",  "carol1111111")
                                                ("isproxy", false)
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "carol1111111", 1000'0000 ), get_voter_info( "carol1111111" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_stake_unstake_keeps_proxy_flag, eosio_system_tester ) try {
   cross_15_percent_threshold();

   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproxy), mvo()
                                               ("proxy",  "alice1111111")
                                               ("isproxy", true)
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" ), get_voter_info( "alice1111111" ) );

   //stake
   BOOST_REQUIRE_EQUAL( success(), issue( "alice1111111", core_from_string("999.9999"),  config::system_account_name ) );
   //check that account is still a proxy
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" )( "staked", 1000'0000 ), get_voter_info( "alice1111111" ) );

   //stake more
   BOOST_REQUIRE_EQUAL( success(), issue( "alice1111111", core_from_string("500.0000"),  config::system_account_name ) );
   //check that account is still a proxy
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" )("staked", 1500'0000 ), get_voter_info( "alice1111111" ) );

   //unstake more
   BOOST_REQUIRE_EQUAL( success(), transfer( "alice1111111", name{config::system_account_name}, core_from_string("500.0000") ) );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" )("staked", 1000'0000 ), get_voter_info( "alice1111111" ) );

   //unstake the rest
   BOOST_REQUIRE_EQUAL( success(), transfer( "alice1111111", name{config::system_account_name}, core_from_string("1000.0000")  ) );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" )( "staked", 0 ), get_voter_info( "alice1111111" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_actions_affect_producers, eosio_system_tester, * boost::unit_test::tolerance(1e+5) ) try {
   cross_15_percent_threshold();

   create_accounts_with_resources( {  N(defproducer1), N(defproducer2), N(defproducer3) } );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer1", 1) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer2", 2) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer3", 3) );

   //register as a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproxy), mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy", true)
                        )
   );

   //accumulate proxied votes
   BOOST_REQUIRE_EQUAL( success(), issue( "bob111111111", core_from_string("999.9999"),  config::system_account_name ) );
   BOOST_REQUIRE_EQUAL( success(), vote(N(bob111111111), vector<account_name>(), N(alice1111111) ) );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" )( "proxied_vote_weight", 1000'0000 ), get_voter_info( "alice1111111" ) );

   //vote for producers
   BOOST_REQUIRE_EQUAL( success(), vote(N(alice1111111), { N(defproducer1), N(defproducer2) } ) );
   BOOST_TEST_REQUIRE( 1000'0001 == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( 1000'0001 == get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( 0 == get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //vote for another producers
   BOOST_REQUIRE_EQUAL( success(), vote( N(alice1111111), { N(defproducer1), N(defproducer3) } ) );
   BOOST_TEST_REQUIRE( 1000'0001 == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( 1000'0001 == get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //unregister proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproxy), mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy", false)
                        )
   );

   BOOST_REQUIRE_EQUAL( 1, get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 1, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //register proxy again
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproxy), mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy", true)
                        )
   );
   BOOST_TEST_REQUIRE( 1000'0001 == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( 1000'0001 == get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //stake increase by proxy itself affects producers
   BOOST_REQUIRE_EQUAL( success(), issue( "alice1111111", core_from_string("199.9999"),  config::system_account_name ) );
   BOOST_REQUIRE_EQUAL( 1200'0000, get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 1200'0000, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //stake decrease by proxy itself affects producers
   BOOST_REQUIRE_EQUAL( success(), transfer( "alice1111111", "eosio", core_from_string("100.0001") ) );
   BOOST_TEST_REQUIRE( 1099'9999 == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( 1099'9999 == get_producer_info( "defproducer3" )["total_votes"].as_double() );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( producer_pay, eosio_system_tester, * boost::unit_test::tolerance(1e-10) ) try {

   const double usecs_per_year  = 52 * 7 * 24 * 3600 * 1000000ll;


   const asset large_asset = core_from_string("80.0000");
   create_account_with_resources( N(defproducera), config::system_account_name, false );
   create_account_with_resources( N(defproducerb), config::system_account_name, false );
   create_account_with_resources( N(defproducerc), config::system_account_name, false );

   create_account_with_resources( N(producvotera), config::system_account_name, false );
   create_account_with_resources( N(producvoterb), config::system_account_name, false );

   BOOST_REQUIRE_EQUAL(success(), regproducer(N(defproducera)));
   auto prod = get_producer_info( N(defproducera) );
   BOOST_REQUIRE_EQUAL("defproducera", prod["owner"].as_string());
   BOOST_REQUIRE_EQUAL(0, prod["total_votes"].as_double());

   BOOST_REQUIRE_EQUAL(success(), transfer( config::system_account_name, "producvotera", core_from_string("400000000.0000")));
   BOOST_REQUIRE_EQUAL(success(), vote( N(producvotera), { N(defproducera) }));

   // defproducera is the only active producer
   // produce enough blocks so new schedule kicks in and defproducera produces some blocks
   {
      produce_blocks(50);

      const auto     initial_global_state      = get_global_state();
      const uint64_t initial_claim_time        = initial_global_state["last_pervote_bucket_fill"].as_uint64();
      const int64_t  initial_pervote_bucket    = initial_global_state["pervote_bucket"].as<int64_t>();
      const int64_t  initial_perblock_bucket   = initial_global_state["perblock_bucket"].as<int64_t>();
      const uint32_t initial_tot_unpaid_blocks = initial_global_state["total_unpaid_blocks"].as<uint32_t>();

      prod = get_producer_info("defproducera");
      const uint32_t unpaid_blocks = prod["unpaid_blocks"].as<uint32_t>();
      BOOST_REQUIRE(1 < unpaid_blocks);
      BOOST_REQUIRE_EQUAL(0, prod["last_claim_time"].as<uint64_t>());

      BOOST_REQUIRE_EQUAL(initial_tot_unpaid_blocks, unpaid_blocks);

      const asset initial_supply  = get_token_supply(SPT_SYMBOL_NAME);
      const asset initial_balance = get_balance(N(defproducera), SPT_SYMBOL);

      BOOST_REQUIRE_EQUAL(success(), push_action(N(defproducera), N(claimrewards), mvo()("owner", "defproducera")));

      const auto     global_state      = get_global_state();
      const uint64_t payment_bucket_per_year = global_state["payment_bucket_per_year"].as_uint64();
      const uint64_t claim_time        = global_state["last_pervote_bucket_fill"].as_uint64();
      const int64_t  pervote_bucket    = global_state["pervote_bucket"].as<int64_t>();
      const int64_t  perblock_bucket   = global_state["perblock_bucket"].as<int64_t>();
      const uint32_t tot_unpaid_blocks = global_state["total_unpaid_blocks"].as<uint32_t>();

      prod = get_producer_info("defproducera");
      BOOST_REQUIRE_EQUAL(1, prod["unpaid_blocks"].as<uint32_t>());
      BOOST_REQUIRE_EQUAL(1, tot_unpaid_blocks);
      const asset supply  = get_token_supply(SPT_SYMBOL_NAME);
      const asset balance = get_balance(N(defproducera), SPT_SYMBOL);

      BOOST_REQUIRE_EQUAL(claim_time, prod["last_claim_time"].as<uint64_t>());

      auto usecs_between_fills = claim_time - initial_claim_time;

      BOOST_REQUIRE_EQUAL(0, initial_perblock_bucket);
      BOOST_REQUIRE_EQUAL(0, initial_pervote_bucket);

      BOOST_REQUIRE_EQUAL(int64_t(  payment_bucket_per_year * ( double(usecs_between_fills)   / usecs_per_year ) ),
                          supply.get_amount() - initial_supply.get_amount());
      BOOST_REQUIRE_EQUAL(int64_t(  payment_bucket_per_year * ( double(usecs_between_fills)   / usecs_per_year ) ),
                          balance.get_amount() - initial_balance.get_amount());

      int64_t bucket = int64_t( payment_bucket_per_year * ( double(usecs_between_fills) / usecs_per_year ) );
      int64_t from_perblock_bucket = bucket / 4 ;
      int64_t from_pervote_bucket  = bucket - from_perblock_bucket;


      BOOST_REQUIRE_EQUAL(from_perblock_bucket + from_pervote_bucket, balance.get_amount() - initial_balance.get_amount());
      BOOST_REQUIRE_EQUAL(0, pervote_bucket);
   }

   {
      BOOST_REQUIRE_EQUAL(wasm_assert_msg("already claimed rewards within past day"),
                          push_action(N(defproducera), N(claimrewards), mvo()("owner", "defproducera")));
   }

   // defproducera waits for 23 hours and 55 minutes, can't claim rewards yet
   {
      produce_block(fc::seconds(23 * 3600 + 55 * 60));
      BOOST_REQUIRE_EQUAL(wasm_assert_msg("already claimed rewards within past day"),
                          push_action(N(defproducera), N(claimrewards), mvo()("owner", "defproducera")));
   }

   // wait 5 more minutes, defproducera can now claim rewards again
   {
      produce_block(fc::seconds(5 * 60));

      const auto     initial_global_state      = get_global_state();
      const uint64_t payment_bucket_per_year   = initial_global_state["payment_bucket_per_year"].as_uint64();
      const uint64_t initial_claim_time        = initial_global_state["last_pervote_bucket_fill"].as_uint64();
      const int64_t  initial_pervote_bucket    = initial_global_state["pervote_bucket"].as<int64_t>();
      const int64_t  initial_perblock_bucket   = initial_global_state["perblock_bucket"].as<int64_t>();
      const uint32_t initial_tot_unpaid_blocks = initial_global_state["total_unpaid_blocks"].as<uint32_t>();
      const double   initial_tot_vote_weight   = initial_global_state["total_producer_vote_weight"].as<double>();

      prod = get_producer_info("defproducera");
      const uint32_t unpaid_blocks = prod["unpaid_blocks"].as<uint32_t>();
      BOOST_REQUIRE(1 < unpaid_blocks);
      BOOST_REQUIRE_EQUAL(initial_tot_unpaid_blocks, unpaid_blocks);
      BOOST_REQUIRE(0 < prod["total_votes"].as<double>());
      BOOST_TEST(initial_tot_vote_weight, prod["total_votes"].as<double>());
      BOOST_REQUIRE(0 < prod["last_claim_time"].as<uint64_t>());

      BOOST_REQUIRE_EQUAL(initial_tot_unpaid_blocks, unpaid_blocks);

      const asset initial_supply  = get_token_supply(SPT_SYMBOL_NAME);
      const asset initial_balance = get_balance(N(defproducera), SPT_SYMBOL);

      BOOST_REQUIRE_EQUAL(success(), push_action(N(defproducera), N(claimrewards), mvo()("owner", "defproducera")));

      const auto global_state          = get_global_state();
      const uint64_t claim_time        = global_state["last_pervote_bucket_fill"].as_uint64();
      const int64_t  pervote_bucket    = global_state["pervote_bucket"].as<int64_t>();
      const int64_t  perblock_bucket   = global_state["perblock_bucket"].as<int64_t>();
      const uint32_t tot_unpaid_blocks = global_state["total_unpaid_blocks"].as<uint32_t>();

      prod = get_producer_info("defproducera");
      BOOST_REQUIRE_EQUAL(1, prod["unpaid_blocks"].as<uint32_t>());
      BOOST_REQUIRE_EQUAL(1, tot_unpaid_blocks);
      const asset supply  = get_token_supply(SPT_SYMBOL_NAME);
      const asset balance = get_balance(N(defproducera), SPT_SYMBOL);

      BOOST_REQUIRE_EQUAL(claim_time, prod["last_claim_time"].as<uint64_t>());
      auto usecs_between_fills = claim_time - initial_claim_time;

      BOOST_REQUIRE_EQUAL(int64_t( payment_bucket_per_year * ( double(usecs_between_fills)   / usecs_per_year ) ),
                          supply.get_amount() - initial_supply.get_amount());
      BOOST_REQUIRE_EQUAL(int64_t( payment_bucket_per_year * ( double(usecs_between_fills)   / usecs_per_year ) ),
                          balance.get_amount() - initial_balance.get_amount());

      int64_t to_producer        = int64_t( payment_bucket_per_year * ( double(usecs_between_fills)   / usecs_per_year ) );
      int64_t to_perblock_bucket = to_producer / 4;
      int64_t to_pervote_bucket  = to_producer - to_perblock_bucket;

      BOOST_REQUIRE_EQUAL(to_perblock_bucket + to_pervote_bucket + initial_pervote_bucket, balance.get_amount() - initial_balance.get_amount());
      BOOST_REQUIRE_EQUAL(0, pervote_bucket);

   }

   // defproducerb tries to claim rewards but he's not on the list
   {
      BOOST_REQUIRE_EQUAL(wasm_assert_msg("unable to find key"),
                          push_action(N(defproducerb), N(claimrewards), mvo()("owner", "defproducerb")));
   }

   // test stability over a year
   {
      regproducer(N(defproducerb));
      regproducer(N(defproducerc));

      const auto     initial_global_state      = get_global_state();
      const uint64_t payment_bucket_per_year   = initial_global_state["payment_bucket_per_year"].as_uint64();
      const asset   initial_supply  = get_token_supply(SPT_SYMBOL_NAME);
      for (uint32_t i = 0; i < 7 * 52; ++i) {
         produce_block(fc::seconds(8 * 3600));
         BOOST_REQUIRE_EQUAL(success(), push_action(N(defproducerc), N(claimrewards), mvo()("owner", "defproducerc")));
         produce_block(fc::seconds(8 * 3600));
         BOOST_REQUIRE_EQUAL(success(), push_action(N(defproducerb), N(claimrewards), mvo()("owner", "defproducerb")));
         produce_block(fc::seconds(8 * 3600));
         BOOST_REQUIRE_EQUAL(success(), push_action(N(defproducera), N(claimrewards), mvo()("owner", "defproducera")));
      }

      const asset   supply  = get_token_supply(SPT_SYMBOL_NAME);
      const double  estimated_error = 0.008;
      // TODO: check estimated error
      BOOST_TEST(std::abs(int64_t(payment_bucket_per_year) - (supply.get_amount() - initial_supply.get_amount())) <  payment_bucket_per_year * estimated_error);
   }
} FC_LOG_AND_RETHROW()



BOOST_FIXTURE_TEST_CASE(multiple_producer_pay, eosio_system_tester, * boost::unit_test::tolerance(1e-10)) try {

   auto within_one = [](int64_t a, int64_t b) -> bool { return std::abs( a - b ) <= 1; };

   const double usecs_per_year  = 52 * 7 * 24 * 3600 * 1000000ll;

   const std::vector<account_name> voters = { N(producvotera), N(producvoterb), N(producvoterc), N(producvoterd) };
   for (const auto& v: voters) {
      create_account_with_resources( v, config::system_account_name, false );
      BOOST_REQUIRE_EQUAL(success(), transfer( config::system_account_name, v, core_from_string("100000000.0000") ));
   }

   // create accounts {defproducera, defproducerb, ..., defproducerz, abcproducera, ..., defproducern} and register as producers
   std::vector<account_name> producer_names;
   {
      producer_names.reserve('z' - 'a' + 1);
      {
         const std::string root("defproducer");
         for ( char c = 'a'; c <= 'z'; ++c ) {
            producer_names.emplace_back(root + std::string(1, c));
         }
      }
      {
         const std::string root("abcproducer");
         for ( char c = 'a'; c <= 'n'; ++c ) {
            producer_names.emplace_back(root + std::string(1, c));
         }
      }
      setup_producer_accounts(producer_names);
      for (const auto& p: producer_names) {
         BOOST_REQUIRE_EQUAL( success(), regproducer(p) );
         produce_blocks(1);
         ilog( "------ get pro----------" );
         wdump((p));
         BOOST_TEST(0 == get_producer_info(p)["total_votes"].as<double>());
      }
   }

   // producvotera votes for defproducera ... defproducerj
   // producvoterb votes for defproducera ... defproduceru
   // producvoterc votes for defproducera ... defproducerz
   // producvoterd votes for abcproducera ... abcproducern
   {
      BOOST_REQUIRE_EQUAL(success(), vote(N(producvotera), vector<account_name>(producer_names.begin(), producer_names.begin()+10)));
      BOOST_REQUIRE_EQUAL(success(), vote(N(producvoterb), vector<account_name>(producer_names.begin(), producer_names.begin()+21)));
      BOOST_REQUIRE_EQUAL(success(), vote(N(producvoterc), vector<account_name>(producer_names.begin(), producer_names.begin()+26)));
      BOOST_REQUIRE_EQUAL(success(), vote(N(producvoterd), vector<account_name>(producer_names.begin()+26, producer_names.end())));
   }

   {
      auto proda = get_producer_info( N(defproducera) );
      auto prodj = get_producer_info( N(defproducerj) );
      auto prodk = get_producer_info( N(defproducerk) );
      auto produ = get_producer_info( N(defproduceru) );
      auto prodv = get_producer_info( N(defproducerv) );
      auto prodz = get_producer_info( N(defproducerz) );

      BOOST_REQUIRE (0 == proda["unpaid_blocks"].as<uint32_t>() && 0 == prodz["unpaid_blocks"].as<uint32_t>());
      BOOST_REQUIRE (0 == proda["last_claim_time"].as<uint64_t>() && 0 == prodz["last_claim_time"].as<uint64_t>());

      // check vote ratios
      BOOST_REQUIRE ( 0 < proda["total_votes"].as<double>() && 0 < prodz["total_votes"].as<double>() );
      BOOST_TEST( proda["total_votes"].as<double>() == prodj["total_votes"].as<double>() );
      BOOST_TEST( prodk["total_votes"].as<double>() == produ["total_votes"].as<double>() );
      BOOST_TEST( prodv["total_votes"].as<double>() == prodz["total_votes"].as<double>() );
      BOOST_TEST( 2 * proda["total_votes"].as<double>() == 3 * produ["total_votes"].as<double>() );
      BOOST_TEST( proda["total_votes"].as<double>() ==  3 * prodz["total_votes"].as<double>() );
   }

   // give a chance for everyone to produce blocks
   {
      produce_blocks(23 * 12 + 20);
      bool all_21_produced = true;
      for (uint32_t i = 0; i < 21; ++i) {
         if (0 == get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            all_21_produced = false;
         }
      }
      bool rest_didnt_produce = true;
      for (uint32_t i = 21; i < producer_names.size(); ++i) {
         if (0 < get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            rest_didnt_produce = false;
         }
      }
      BOOST_REQUIRE(all_21_produced && rest_didnt_produce);
   }

   std::vector<double> vote_shares(producer_names.size());
   {
      double total_votes = 0;
      for (uint32_t i = 0; i < producer_names.size(); ++i) {
         vote_shares[i] = get_producer_info(producer_names[i])["total_votes"].as<double>();
         total_votes += vote_shares[i];
      }
      BOOST_TEST(total_votes == get_global_state()["total_producer_vote_weight"].as<double>());
      std::for_each(vote_shares.begin(), vote_shares.end(), [total_votes](double& x) { x /= total_votes; });

      BOOST_TEST(double(1) == std::accumulate(vote_shares.begin(), vote_shares.end(), double(0)));
      BOOST_TEST(double(3./71.) == vote_shares.front());
      BOOST_TEST(double(1./71.) == vote_shares.back());
   }

   {
      const uint32_t prod_index = 2;
      const auto prod_name = producer_names[prod_index];

      const auto     initial_global_state      = get_global_state();
      const uint64_t initial_fill_time         = initial_global_state["last_pervote_bucket_fill"].as_uint64();
      const uint64_t initial_claim_time        = initial_global_state["first_system_block_time"].as_uint64();
      const int64_t  initial_pervote_bucket    = initial_global_state["pervote_bucket"].as<int64_t>();
      const int64_t  initial_perblock_bucket   = initial_global_state["perblock_bucket"].as<int64_t>();
      const uint32_t initial_tot_unpaid_blocks = initial_global_state["total_unpaid_blocks"].as<uint32_t>();
      const asset    initial_supply            = get_token_supply(SPT_SYMBOL_NAME);
      const asset    initial_bpay_balance      = get_balance(N(eosio.bpay), SPT_SYMBOL);
      const asset    initial_vpay_balance      = get_balance(N(eosio.vpay), SPT_SYMBOL);
      const asset    initial_balance           = get_balance(prod_name, SPT_SYMBOL);
      const uint32_t initial_unpaid_blocks     = get_producer_info(prod_name)["unpaid_blocks"].as<uint32_t>();

      BOOST_REQUIRE_EQUAL(success(), push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));

      const auto     global_state      = get_global_state();
      const uint64_t fill_time        = global_state["last_pervote_bucket_fill"].as_uint64();
      const uint64_t payment_bucket_per_year   = initial_global_state["payment_bucket_per_year"].as_uint64();
      const uint64_t claim_time        = get_producer_info(prod_name)["last_claim_time"].as_uint64();
      const int64_t  pervote_bucket    = global_state["pervote_bucket"].as<int64_t>();
      const int64_t  perblock_bucket   = global_state["perblock_bucket"].as<int64_t>();
      const uint32_t tot_unpaid_blocks = global_state["total_unpaid_blocks"].as<uint32_t>();
      const asset    supply            = get_token_supply(SPT_SYMBOL_NAME);
      const asset    bpay_balance      = get_balance(N(eosio.bpay), SPT_SYMBOL);
      const asset    vpay_balance      = get_balance(N(eosio.vpay), SPT_SYMBOL);
      const asset    balance           = get_balance(prod_name, SPT_SYMBOL);
      const uint32_t unpaid_blocks     = get_producer_info(prod_name)["unpaid_blocks"].as<uint32_t>();

      const uint64_t usecs_between_fills = fill_time - initial_fill_time;
      const uint64_t usecs_between_claims = claim_time - initial_claim_time;

      const int64_t expected_supply_growth = int64_t(payment_bucket_per_year * double(usecs_between_fills)/ double(usecs_per_year));
      const int64_t expected_claim = int64_t(payment_bucket_per_year * double(usecs_between_claims)/ double(usecs_per_year));
      BOOST_REQUIRE_EQUAL( expected_supply_growth, supply.get_amount() - initial_supply.get_amount() );

      const int64_t expected_perblock_bucket = expected_supply_growth / 4;
      const int64_t expected_pervote_bucket  = expected_supply_growth - expected_supply_growth / 4;

      const int64_t from_perblock_bucket =  int64_t(expected_perblock_bucket * double(initial_unpaid_blocks) / double(initial_tot_unpaid_blocks));
      const int64_t from_pervote_bucket  = int64_t( vote_shares[prod_index] * uint64_t( expected_claim - expected_claim / 4 ));

      BOOST_REQUIRE( 1 >= abs(int32_t(initial_tot_unpaid_blocks - tot_unpaid_blocks) - int32_t(initial_unpaid_blocks - unpaid_blocks)) );
      BOOST_REQUIRE( within_one( expected_perblock_bucket - from_perblock_bucket, perblock_bucket ) );
      BOOST_REQUIRE( within_one( expected_perblock_bucket - from_perblock_bucket, bpay_balance.get_amount() ) );
      BOOST_REQUIRE( within_one( expected_pervote_bucket - from_pervote_bucket, pervote_bucket ) );
      BOOST_REQUIRE( within_one( expected_pervote_bucket - from_pervote_bucket, vpay_balance.get_amount() ) );
      BOOST_REQUIRE( within_one( from_perblock_bucket + from_pervote_bucket, balance.get_amount() - initial_balance.get_amount() ) );


      produce_blocks(5);

      BOOST_REQUIRE_EQUAL(wasm_assert_msg("already claimed rewards within past day"),
                          push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
   }

   // Wait for 23 hours. By now, pervote_bucket has grown enough
   // that a producer's share is more than 100 tokens.
   produce_block(fc::seconds(23 * 3600));

   {
      const uint32_t prod_index = 15;
      const auto prod_name = producer_names[prod_index];

      const auto     initial_global_state      = get_global_state();
      const uint64_t payment_bucket_per_year   = initial_global_state["payment_bucket_per_year"].as_uint64();
      const uint64_t initial_fill_time        =  initial_global_state["last_pervote_bucket_fill"].as_uint64();
      const uint64_t initial_claim_time        = initial_global_state["first_system_block_time"].as_uint64();
      const int64_t  initial_pervote_bucket    = initial_global_state["pervote_bucket"].as<int64_t>();
      const int64_t  initial_perblock_bucket   = initial_global_state["perblock_bucket"].as<int64_t>();
      const uint32_t initial_tot_unpaid_blocks = initial_global_state["total_unpaid_blocks"].as<uint32_t>();
      const asset    initial_supply            = get_token_supply(SPT_SYMBOL_NAME);
      const asset    initial_bpay_balance      = get_balance(N(eosio.bpay), SPT_SYMBOL);
      const asset    initial_vpay_balance      = get_balance(N(eosio.vpay), SPT_SYMBOL);
      const asset    initial_balance           = get_balance(prod_name, SPT_SYMBOL);
      const uint32_t initial_unpaid_blocks     = get_producer_info(prod_name)["unpaid_blocks"].as<uint32_t>();

      BOOST_REQUIRE_EQUAL(success(), push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));

      const auto     global_state      = get_global_state();
      const uint64_t fill_time         = global_state["last_pervote_bucket_fill"].as_uint64();
      const uint64_t claim_time        = get_producer_info(prod_name)["last_claim_time"].as_uint64();
      const int64_t  pervote_bucket    = global_state["pervote_bucket"].as<int64_t>();
      const int64_t  perblock_bucket   = global_state["perblock_bucket"].as<int64_t>();
      const uint32_t tot_unpaid_blocks = global_state["total_unpaid_blocks"].as<uint32_t>();
      const asset    supply            = get_token_supply(SPT_SYMBOL_NAME);
      const asset    bpay_balance      = get_balance(N(eosio.bpay), SPT_SYMBOL);
      const asset    vpay_balance      = get_balance(N(eosio.vpay), SPT_SYMBOL);
      const asset    balance           = get_balance(prod_name, SPT_SYMBOL);
      const uint32_t unpaid_blocks     = get_producer_info(prod_name)["unpaid_blocks"].as<uint32_t>();

      const uint64_t usecs_between_fills = fill_time - initial_fill_time;
      const uint64_t usecs_between_claims = claim_time - initial_claim_time;

      const int64_t expected_supply_growth = int64_t(payment_bucket_per_year * double(usecs_between_fills)/ double(usecs_per_year));
      const int64_t expected_claim = int64_t(payment_bucket_per_year * double(usecs_between_claims)/ double(usecs_per_year));
      BOOST_REQUIRE_EQUAL( expected_supply_growth, supply.get_amount() - initial_supply.get_amount() );

      const int64_t cur_expected_perblock_bucket = expected_supply_growth / 4;
      const int64_t cur_expected_pervote_bucket  = expected_supply_growth - cur_expected_perblock_bucket;

      const int64_t expected_perblock_bucket = initial_perblock_bucket + cur_expected_perblock_bucket;
      const int64_t expected_pervote_bucket  = initial_pervote_bucket + cur_expected_pervote_bucket;
      const int64_t from_perblock_bucket = int64_t( expected_perblock_bucket * double(initial_unpaid_blocks) / double(initial_tot_unpaid_blocks) );
      const int64_t from_pervote_bucket  = int64_t( vote_shares[prod_index] * int64_t( expected_claim - expected_claim / 4 ));

      BOOST_REQUIRE( 1 >= abs(int32_t(initial_tot_unpaid_blocks - tot_unpaid_blocks) - int32_t(initial_unpaid_blocks - unpaid_blocks)) );
      BOOST_REQUIRE( within_one( expected_perblock_bucket - from_perblock_bucket, perblock_bucket ) );
      BOOST_REQUIRE( within_one( expected_perblock_bucket - from_perblock_bucket, bpay_balance.get_amount() ) );
      BOOST_REQUIRE( within_one( expected_pervote_bucket - from_pervote_bucket, pervote_bucket ) );
      BOOST_REQUIRE( within_one( expected_pervote_bucket - from_pervote_bucket, vpay_balance.get_amount() ) );
      BOOST_REQUIRE( within_one( from_perblock_bucket + from_pervote_bucket, balance.get_amount() - initial_balance.get_amount() ) );
      

      produce_blocks(5);

      BOOST_REQUIRE_EQUAL(wasm_assert_msg("already claimed rewards within past day"),
                          push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
   }

   {
      const uint32_t prod_index = 24;
      const auto prod_name = producer_names[prod_index];
      BOOST_REQUIRE_EQUAL(success(),
                          push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
      BOOST_REQUIRE(0 <= get_balance(prod_name, SPT_SYMBOL).get_amount());
      BOOST_REQUIRE_EQUAL(wasm_assert_msg("already claimed rewards within past day"),
                          push_action(prod_name, N(claimrewards), mvo()("owner", prod_name)));
   }

   {
      const uint32_t rmv_index = 5;
      account_name prod_name = producer_names[rmv_index];

      auto info = get_producer_info(prod_name);
      BOOST_REQUIRE( info["is_active"].as<bool>() );
      BOOST_REQUIRE( fc::crypto::public_key() != fc::crypto::public_key(info["producer_key"].as_string()) );

      BOOST_REQUIRE_EQUAL( error("missing authority of eosio"),
                           push_action(prod_name, N(rmvproducer), mvo()("producer", prod_name)));
      BOOST_REQUIRE_EQUAL( error("missing authority of eosio"),
                           push_action(producer_names[rmv_index + 2], N(rmvproducer), mvo()("producer", prod_name) ) );
      BOOST_REQUIRE_EQUAL( success(),
                           push_action(config::system_account_name, N(rmvproducer), mvo()("producer", prod_name) ) );
      {
         bool rest_didnt_produce = true;
         for (uint32_t i = 21; i < producer_names.size(); ++i) {
            if (0 < get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
               rest_didnt_produce = false;
            }
         }
         BOOST_REQUIRE(rest_didnt_produce);
      }

      produce_blocks(6 * 21 * 12);
      info = get_producer_info(prod_name);
      const uint32_t init_unpaid_blocks = info["unpaid_blocks"].as<uint32_t>();
      BOOST_REQUIRE( !info["is_active"].as<bool>() );
      BOOST_REQUIRE( fc::crypto::public_key() == fc::crypto::public_key(info["producer_key"].as_string()) );
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("producer does not have an active key"),
                           push_action(prod_name, N(claimrewards), mvo()("owner", prod_name) ) );
      produce_blocks(3 * 21 * 12);
      BOOST_REQUIRE_EQUAL( init_unpaid_blocks, get_producer_info(prod_name)["unpaid_blocks"].as<uint32_t>() );
      {
         bool prod_was_replaced = false;
         for (uint32_t i = 21; i < producer_names.size(); ++i) {
            if (0 < get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
               prod_was_replaced = true;
            }
         }
         BOOST_REQUIRE(prod_was_replaced);
      }
   }

   {
      BOOST_REQUIRE_EQUAL( wasm_assert_msg("producer not found"),
                           push_action( config::system_account_name, N(rmvproducer), mvo()("producer", "nonexistingp") ) );
   }

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(producers_upgrade_system_contract, eosio_system_tester) try {
   //install multisig contract
   abi_serializer msig_abi_ser = initialize_multisig();
   auto producer_names = active_and_vote_producers();

   //helper function
   auto push_action_msig = [&]( const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) -> action_result {
         string action_type_name = msig_abi_ser.get_action_type(name);

         action act;
         act.account = N(eosio.msig);
         act.name = name;
         act.data = msig_abi_ser.variant_to_binary( action_type_name, data, abi_serializer_max_time );

         return base_tester::push_action( std::move(act), auth ? uint64_t(signer) : signer == N(bob111111111) ? N(alice1111111) : N(bob111111111) );
   };
   // test begins
   vector<permission_level> prod_perms;
   for ( auto& x : producer_names ) {
      prod_perms.push_back( { name(x), config::active_name } );
   }
   //prepare system contract with different hash (contract differs in one byte)
   string eosio_system_wast2 = eosio_system_wast;
   string msg = "producer votes must be unique and sorted";
   auto pos = eosio_system_wast2.find(msg);
   BOOST_REQUIRE( pos != std::string::npos );
   msg[0] = 'P';
   eosio_system_wast2.replace( pos, msg.size(), msg );

   transaction trx;
   {
      auto code = wast_to_wasm( eosio_system_wast2 );
      variant pretty_trx = fc::mutable_variant_object()
         ("expiration", "2020-01-01T00:30")
         ("ref_block_num", 2)
         ("ref_block_prefix", 3)
         ("max_net_usage_words", 0)
         ("max_cpu_usage_ms", 0)
         ("delay_sec", 0)
         ("actions", fc::variants({
               fc::mutable_variant_object()
                  ("account", name(config::system_account_name))
                  ("name", "setcode")
                  ("authorization", vector<permission_level>{ { config::system_account_name, config::active_name } })
                  ("data", fc::mutable_variant_object() ("account", name(config::system_account_name))
                   ("vmtype", 0)
                   ("vmversion", "0")
                   ("code", bytes( code.begin(), code.end() ))
                  )
                  })
         );
      abi_serializer::from_variant(pretty_trx, trx, get_resolver(), abi_serializer_max_time);
   }

   BOOST_REQUIRE_EQUAL(success(), push_action_msig( N(alice1111111), N(propose), mvo()
                                                    ("proposer",      "alice1111111")
                                                    ("proposal_name", "upgrade1")
                                                    ("trx",           trx)
                                                    ("requested", prod_perms)
                       )
   );

   // get 15 approvals
   for ( size_t i = 0; i < 14; ++i ) {
      BOOST_REQUIRE_EQUAL(success(), push_action_msig( name(producer_names[i]), N(approve), mvo()
                                                       ("proposer",      "alice1111111")
                                                       ("proposal_name", "upgrade1")
                                                       ("level",         permission_level{ name(producer_names[i]), config::active_name })
                          )
      );
   }

   //should fail
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("transaction authorization failed"),
                       push_action_msig( N(alice1111111), N(exec), mvo()
                                         ("proposer",      "alice1111111")
                                         ("proposal_name", "upgrade1")
                                         ("executer",      "alice1111111")
                       )
   );

   // one more approval
   BOOST_REQUIRE_EQUAL(success(), push_action_msig( name(producer_names[14]), N(approve), mvo()
                                                    ("proposer",      "alice1111111")
                                                    ("proposal_name", "upgrade1")
                                                    ("level",         permission_level{ name(producer_names[14]), config::active_name })
                          )
   );

   transaction_trace_ptr trace;
   control->applied_transaction.connect([&]( const transaction_trace_ptr& t) { if (t->scheduled) { trace = t; } } );
   BOOST_REQUIRE_EQUAL(success(), push_action_msig( N(alice1111111), N(exec), mvo()
                                                    ("proposer",      "alice1111111")
                                                    ("proposal_name", "upgrade1")
                                                    ("executer",      "alice1111111")
                       )
   );

   BOOST_REQUIRE( bool(trace) );
   BOOST_REQUIRE_EQUAL( 1, trace->action_traces.size() );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, trace->receipt->status );

   produce_blocks( 250 );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE(producer_onblock_check, eosio_system_tester) try {

   create_account_with_resources( N(producvotera), config::system_account_name, false );
   create_account_with_resources( N(producvoterb), config::system_account_name, false );
   create_account_with_resources( N(producvoterc), config::system_account_name, false );

   // create accounts {defproducera, defproducerb, ..., defproducerz} and register as producers
   std::vector<account_name> producer_names;
   producer_names.reserve('z' - 'a' + 1);
   const std::string root("defproducer");
   for ( char c = 'a'; c <= 'z'; ++c ) {
      producer_names.emplace_back(root + std::string(1, c));
   }
   setup_producer_accounts(producer_names);

   for (auto a:producer_names)
      regproducer(a);

   BOOST_REQUIRE_EQUAL(0, get_producer_info( producer_names.front() )["total_votes"].as<double>());
   BOOST_REQUIRE_EQUAL(0, get_producer_info( producer_names.back() )["total_votes"].as<double>());


   BOOST_REQUIRE_EQUAL(success(), transfer(config::system_account_name, "producvotera", core_from_string("140000000.0000")));
   BOOST_REQUIRE_EQUAL(success(), vote( N(producvotera), vector<account_name>(producer_names.begin(), producer_names.begin()+10)));

   // give a chance for everyone to produce blocks
   {
      produce_blocks(21 * 12);
      bool all_21_produced = true;
      for (uint32_t i = 0; i < 21; ++i) {
         if (0 == get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            all_21_produced= false;
         }
      }
      bool rest_didnt_produce = true;
      for (uint32_t i = 21; i < producer_names.size(); ++i) {
         if (0 < get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            rest_didnt_produce = false;
         }
      }
      BOOST_REQUIRE_EQUAL(false, all_21_produced);
      BOOST_REQUIRE_EQUAL(true, rest_didnt_produce);
   }

   {
      const char* claimrewards_activation_error_message = "cannot claim rewards until the chain is activated (at least 15% of all tokens participate in voting)";
      BOOST_CHECK_EQUAL(0, get_global_state()["total_unpaid_blocks"].as<uint32_t>());
      BOOST_REQUIRE_EQUAL(wasm_assert_msg( claimrewards_activation_error_message ),
                          push_action(producer_names.front(), N(claimrewards), mvo()("owner", producer_names.front())));
      BOOST_REQUIRE_EQUAL(0, get_balance(producer_names.front(), SPT_SYMBOL).get_amount());
      BOOST_REQUIRE_EQUAL(wasm_assert_msg( claimrewards_activation_error_message ),
                          push_action(producer_names.back(), N(claimrewards), mvo()("owner", producer_names.back())));
      BOOST_REQUIRE_EQUAL(0, get_balance(producer_names.back(), SPT_SYMBOL).get_amount());
   }

   // stake across 15% boundary
   BOOST_REQUIRE_EQUAL(success(), transfer(config::system_account_name, "producvoterb", core_from_string("10000000.0000")));
   BOOST_REQUIRE_EQUAL(success(), transfer(config::system_account_name, "producvoterc", core_from_string("10000000.0000")));

   BOOST_REQUIRE_EQUAL(success(), vote( N(producvoterb), vector<account_name>(producer_names.begin(), producer_names.begin()+21)));
   BOOST_REQUIRE_EQUAL(success(), vote( N(producvoterc), vector<account_name>(producer_names.begin(), producer_names.end())));

   // give a chance for everyone to produce blocks
   {
      produce_blocks(21 * 12);
      bool all_21_produced = true;
      for (uint32_t i = 0; i < 21; ++i) {
         if (0 == get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            all_21_produced= false;
         }
      }
      bool rest_didnt_produce = true;
      for (uint32_t i = 21; i < producer_names.size(); ++i) {
         if (0 < get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            rest_didnt_produce = false;
         }
      }
      BOOST_REQUIRE_EQUAL(true, all_21_produced);
      BOOST_REQUIRE_EQUAL(true, rest_didnt_produce);
      BOOST_REQUIRE_EQUAL(success(),
                          push_action(producer_names.front(), N(claimrewards), mvo()("owner", producer_names.front())));
      BOOST_REQUIRE(0 < get_balance(producer_names.front(), SPT_SYMBOL).get_amount());
   }

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( voters_actions_affect_proxy_and_producers, eosio_system_tester, * boost::unit_test::tolerance(1e+6) ) try {
   cross_15_percent_threshold();

   create_accounts_with_resources( { N(donald111111), N(defproducer1), N(defproducer2), N(defproducer3) } );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer1", 1) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer2", 2) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer3", 3) );

   //alice1111111 becomes a producer
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproxy), mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy", true)
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" ), get_voter_info( "alice1111111" ) );

   //alice1111111 makes stake and votes
   BOOST_REQUIRE_EQUAL( success(), issue( "alice1111111", core_from_string("999.9999"),  config::system_account_name ) );
   BOOST_REQUIRE_EQUAL( success(), vote( N(alice1111111), { N(defproducer1), N(defproducer2) } ) );
   BOOST_TEST_REQUIRE( 1000'0000 == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( 1000'0000 == get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   BOOST_REQUIRE_EQUAL( success(), push_action( N(donald111111), N(regproxy), mvo()
                                                ("proxy",  "donald111111")
                                                ("isproxy", true)
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "donald111111" ), get_voter_info( "donald111111" ) );

   //bob111111111 chooses alice1111111 as a proxy
   BOOST_REQUIRE_EQUAL( success(), issue( "bob111111111", core_from_string("500.0002"),  config::system_account_name ) );
   BOOST_REQUIRE_EQUAL( success(), vote( N(bob111111111), vector<account_name>(), "alice1111111" ) );
   BOOST_TEST_REQUIRE( 500'0003 == get_voter_info( "alice1111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( 1500'0003 == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( 1500'0003 == get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //carol1111111 chooses alice1111111 as a proxy
   BOOST_REQUIRE_EQUAL( success(), issue( "carol1111111", core_from_string("50.0001"),  config::system_account_name ) );
   BOOST_REQUIRE_EQUAL( success(), vote( N(carol1111111), vector<account_name>(), "alice1111111" ) );
   BOOST_TEST_REQUIRE( 550'0005 == get_voter_info( "alice1111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( 1550'0005 == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( 1550'0005 == get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //proxied voter carol1111111 increases stake
   BOOST_REQUIRE_EQUAL( success(), issue( "carol1111111", core_from_string("120.0000"),  config::system_account_name ) );
   BOOST_TEST_REQUIRE( 670'0005 == get_voter_info( "alice1111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( 1670'0005 == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( 1670'0005 == get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //proxied voter bob111111111 decreases stake
   BOOST_REQUIRE_EQUAL( success(), transfer("bob111111111", config::system_account_name, core_from_string("250.0000")) );
   BOOST_TEST_REQUIRE( 420'0005 == get_voter_info( "alice1111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( 1420'0005 == get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( 1420'0005 == get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //proxied voter carol1111111 chooses another proxy
   BOOST_REQUIRE_EQUAL( success(), vote( N(carol1111111), vector<account_name>(), "donald111111" ) );
   BOOST_TEST_REQUIRE( 250'0003, get_voter_info( "alice1111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( 170'0002, get_voter_info( "donald111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( 1250'0003, get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( 1250'0003, get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_REQUIRE_EQUAL( 0, get_producer_info( "defproducer3" )["total_votes"].as_double() );

   //bob111111111 switches to direct voting and votes for one of the same producers, but not for another one
   BOOST_REQUIRE_EQUAL( success(), vote( N(bob111111111), { N(defproducer2) } ) );
   BOOST_TEST_REQUIRE( 0.0 == get_voter_info( "alice1111111" )["proxied_vote_weight"].as_double() );
   BOOST_TEST_REQUIRE( 1000'0000, get_producer_info( "defproducer1" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( 1250'0003, get_producer_info( "defproducer2" )["total_votes"].as_double() );
   BOOST_TEST_REQUIRE( 0.0 == get_producer_info( "defproducer3" )["total_votes"].as_double() );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( vote_both_proxy_and_producers, eosio_system_tester ) try {
   //alice1111111 becomes a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproxy), mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy", true)
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" ), get_voter_info( "alice1111111" ) );

   //carol1111111 becomes a producer
   BOOST_REQUIRE_EQUAL( success(), regproducer( "carol1111111", 1) );

   //bob111111111 chooses alice1111111 as a proxy

   BOOST_REQUIRE_EQUAL( success(), issue( "bob111111111", core_from_string("999.9999"),  config::system_account_name ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("cannot vote for producers and proxy at same time"),
                        vote( N(bob111111111), { N(carol1111111) }, "alice1111111" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( select_invalid_proxy, eosio_system_tester ) try {
   //accumulate proxied votes
   BOOST_REQUIRE_EQUAL( success(), issue( "bob111111111", core_from_string("999.9999"),  config::system_account_name ) );

   //selecting account not registered as a proxy
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "proxy not found" ),
                        vote( N(bob111111111), vector<account_name>(), "alice1111111" ) );

   //selecting not existing account as a proxy
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "invalid proxy specified" ),
                        vote( N(bob111111111), vector<account_name>(), "notexist" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( double_register_unregister_proxy_keeps_votes, eosio_system_tester ) try {
   //alice1111111 becomes a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproxy), mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy",  1)
                        )
   );
   BOOST_REQUIRE_EQUAL( success(), issue( "alice1111111", core_from_string("999.9999"),  config::system_account_name ) );
   edump((get_voter_info("alice1111111")));
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" )( "staked", 1000'0000 ), get_voter_info( "alice1111111" ) );

   //bob111111111 stakes and selects alice1111111 as a proxy
   BOOST_REQUIRE_EQUAL( success(), issue( "bob111111111", core_from_string("999.9999"),  config::system_account_name ) );
   BOOST_REQUIRE_EQUAL( success(), vote( N(bob111111111), vector<account_name>(), "alice1111111" ) );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" )( "proxied_vote_weight", 1000'0000)( "staked", 1000'0000 ), get_voter_info( "alice1111111" ) );

   //double regestering should fail without affecting total votes and stake
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "action has no effect" ),
                        push_action( N(alice1111111), N(regproxy), mvo()
                                     ("proxy",  "alice1111111")
                                     ("isproxy",  1)
                        )
   );
   REQUIRE_MATCHING_OBJECT( proxy( "alice1111111" )( "proxied_vote_weight", 1000'0000 )( "staked", 1000'0000 ), get_voter_info( "alice1111111" ) );

   //uregister
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproxy), mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy",  0)
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111" )( "proxied_vote_weight", 1000'0000 )( "staked", 1000'0000 ), get_voter_info( "alice1111111" ) );

   //double unregistering should not affect proxied_votes and stake
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "action has no effect" ),
                        push_action( N(alice1111111), N(regproxy), mvo()
                                     ("proxy",  "alice1111111")
                                     ("isproxy",  0)
                        )
   );
   REQUIRE_MATCHING_OBJECT( voter( "alice1111111" )( "proxied_vote_weight", 1000'0000)( "staked", 1000'0000 ), get_voter_info( "alice1111111" ) );

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( proxy_cannot_use_another_proxy, eosio_system_tester ) try {
   //alice1111111 becomes a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(alice1111111), N(regproxy), mvo()
                                                ("proxy",  "alice1111111")
                                                ("isproxy",  1)
                        )
   );

   //bob111111111 becomes a proxy
   BOOST_REQUIRE_EQUAL( success(), push_action( N(bob111111111), N(regproxy), mvo()
                                                ("proxy",  "bob111111111")
                                                ("isproxy",  1)
                        )
   );

   //proxy should not be able to use a proxy
   BOOST_REQUIRE_EQUAL( success(), issue( "bob111111111", core_from_string("1000.0000"),  config::system_account_name ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "account registered as a proxy is not allowed to use a proxy" ),
                        vote( N(bob111111111), vector<account_name>(), "alice1111111" ) );

   //voter that uses a proxy should not be allowed to become a proxy
   BOOST_REQUIRE_EQUAL( success(), issue( "carol1111111", core_from_string("999.9999"),  config::system_account_name ) );
   BOOST_REQUIRE_EQUAL( success(), vote( N(carol1111111), vector<account_name>(), "alice1111111" ) );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "account that uses a proxy is not allowed to become a proxy" ),
                        push_action( N(carol1111111), N(regproxy), mvo()
                                     ("proxy",  "carol1111111")
                                     ("isproxy",  1)
                        )
   );

   //proxy should not be able to use itself as a proxy
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "cannot proxy to self" ),
                        vote( N(bob111111111), vector<account_name>(), "bob111111111" ) );

} FC_LOG_AND_RETHROW()

fc::mutable_variant_object config_to_variant( const eosio::chain::chain_config& config ) {
   return mutable_variant_object()
      ( "max_block_net_usage", config.max_block_net_usage )
      ( "target_block_net_usage_pct", config.target_block_net_usage_pct )
      ( "max_transaction_net_usage", config.max_transaction_net_usage )
      ( "base_per_transaction_net_usage", config.base_per_transaction_net_usage )
      ( "context_free_discount_net_usage_num", config.context_free_discount_net_usage_num )
      ( "context_free_discount_net_usage_den", config.context_free_discount_net_usage_den )
      ( "max_block_cpu_usage", config.max_block_cpu_usage )
      ( "target_block_cpu_usage_pct", config.target_block_cpu_usage_pct )
      ( "max_transaction_cpu_usage", config.max_transaction_cpu_usage )
      ( "min_transaction_cpu_usage", config.min_transaction_cpu_usage )
      ( "max_transaction_lifetime", config.max_transaction_lifetime )
      ( "deferred_trx_expiration_window", config.deferred_trx_expiration_window )
      ( "max_transaction_delay", config.max_transaction_delay )
      ( "max_inline_action_size", config.max_inline_action_size )
      ( "max_inline_action_depth", config.max_inline_action_depth )
      ( "max_authority_depth", config.max_authority_depth );
}

BOOST_FIXTURE_TEST_CASE( elect_producers /*_and_parameters*/, eosio_system_tester ) try {
   create_accounts_with_resources( {  N(defproducer1), N(defproducer2), N(defproducer3) } );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer1", 1) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer2", 2) );
   BOOST_REQUIRE_EQUAL( success(), regproducer( "defproducer3", 3) );

   //stake more than 15% of total EOS supply to activate chain
   BOOST_REQUIRE_EQUAL( success(), transfer( "eosio", "alice1111111", core_from_string("600000000.0000") ) );
   //vote for producers
   BOOST_REQUIRE_EQUAL( success(), vote( N(alice1111111), { N(defproducer1) } ) );
   produce_blocks(250);
   auto producer_keys = control->head_block_state()->active_schedule.producers;
   BOOST_REQUIRE_EQUAL( 1, producer_keys.size() );
   BOOST_REQUIRE_EQUAL( name("defproducer1"), producer_keys[0].producer_name );

   //auto config = config_to_variant( control->get_global_properties().configuration );
   //auto prod1_config = testing::filter_fields( config, producer_parameters_example( 1 ) );
   //REQUIRE_EQUAL_OBJECTS(prod1_config, config);

   // elect 2 producers
   ilog("stake");
   BOOST_REQUIRE_EQUAL( success(), issue( "bob111111111", core_from_string("80000.0000"),  config::system_account_name ) );
   ilog("start vote");
   BOOST_REQUIRE_EQUAL( success(), vote( N(bob111111111), { N(defproducer2) } ) );
   ilog(".");
   produce_blocks(250);
   producer_keys = control->head_block_state()->active_schedule.producers;
   BOOST_REQUIRE_EQUAL( 2, producer_keys.size() );
   BOOST_REQUIRE_EQUAL( name("defproducer1"), producer_keys[0].producer_name );
   BOOST_REQUIRE_EQUAL( name("defproducer2"), producer_keys[1].producer_name );
   //config = config_to_variant( control->get_global_properties().configuration );
   //auto prod2_config = testing::filter_fields( config, producer_parameters_example( 2 ) );
   //REQUIRE_EQUAL_OBJECTS(prod2_config, config);

   // elect 3 producers
   BOOST_REQUIRE_EQUAL( success(), vote( N(bob111111111), { N(defproducer2), N(defproducer3) } ) );
   produce_blocks(250);
   producer_keys = control->head_block_state()->active_schedule.producers;
   BOOST_REQUIRE_EQUAL( 3, producer_keys.size() );
   BOOST_REQUIRE_EQUAL( name("defproducer1"), producer_keys[0].producer_name );
   BOOST_REQUIRE_EQUAL( name("defproducer2"), producer_keys[1].producer_name );
   BOOST_REQUIRE_EQUAL( name("defproducer3"), producer_keys[2].producer_name );
   //config = config_to_variant( control->get_global_properties().configuration );
   //REQUIRE_EQUAL_OBJECTS(prod2_config, config);

   // try to go back to 2 producers and fail
   BOOST_REQUIRE_EQUAL( success(), vote( N(bob111111111), { N(defproducer3) } ) );
   produce_blocks(250);
   producer_keys = control->head_block_state()->active_schedule.producers;
   BOOST_REQUIRE_EQUAL( 3, producer_keys.size() );

   // The test below is invalid now, producer schedule is not updated if there are
   // fewer producers in the new schedule
   /*
   BOOST_REQUIRE_EQUAL( 2, producer_keys.size() );
   BOOST_REQUIRE_EQUAL( name("defproducer1"), producer_keys[0].producer_name );
   BOOST_REQUIRE_EQUAL( name("defproducer3"), producer_keys[1].producer_name );
   //config = config_to_variant( control->get_global_properties().configuration );
   //auto prod3_config = testing::filter_fields( config, producer_parameters_example( 3 ) );
   //REQUIRE_EQUAL_OBJECTS(prod3_config, config);
   */

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE( vote_producers_in_and_out, eosio_system_tester ) try {

   std::vector<account_name> voters = { N(producvotera), N(producvoterb), N(producvoterc), N(producvoterd) };
   for (const auto& v: voters) {
      create_account_with_resources(v, config::system_account_name, false);
   }

   // create accounts {defproducera, defproducerb, ..., defproducerz} and register as producers
   std::vector<account_name> producer_names;
   {
      producer_names.reserve('z' - 'a' + 1);
      const std::string root("defproducer");
      for ( char c = 'a'; c <= 'z'; ++c ) {
         producer_names.emplace_back(root + std::string(1, c));
      }
      setup_producer_accounts(producer_names);
      for (const auto& p: producer_names) {
         BOOST_REQUIRE_EQUAL( success(), regproducer(p) );
         produce_blocks(1);
         ilog( "------ get pro----------" );
         wdump((p));
         BOOST_TEST(0 == get_producer_info(p)["total_votes"].as<double>());
      }
   }

   for (const auto& v: voters) {
      BOOST_REQUIRE_EQUAL(success(), transfer( config::system_account_name, v, core_from_string("200000000.0000") ) );
   }

   {
      BOOST_REQUIRE_EQUAL(success(), vote(N(producvotera), vector<account_name>(producer_names.begin(), producer_names.begin()+20)));
      BOOST_REQUIRE_EQUAL(success(), vote(N(producvoterb), vector<account_name>(producer_names.begin(), producer_names.begin()+21)));
      BOOST_REQUIRE_EQUAL(success(), vote(N(producvoterc), vector<account_name>(producer_names.begin(), producer_names.end())));
   }

   // give a chance for everyone to produce blocks
   {
      produce_blocks(120 + 14 * 120  + 21);
      bool all_21_produced = true;
      for (uint32_t i = 0; i < 21; ++i) {
         if (0 == get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            all_21_produced = false;
         }
      }
      bool rest_didnt_produce = true;
      for (uint32_t i = 21; i < producer_names.size(); ++i) {
         if (0 < get_producer_info(producer_names[i])["unpaid_blocks"].as<uint32_t>()) {
            rest_didnt_produce = false;
         }
      }
      BOOST_REQUIRE(all_21_produced && rest_didnt_produce);
   }

   {
      produce_block(fc::hours(7));
      const uint32_t voted_out_index = 20;
      const uint32_t new_prod_index  = 23;
      BOOST_REQUIRE_EQUAL(success(), issue("producvoterd", core_from_string("80000000.0000"), config::system_account_name));
      BOOST_REQUIRE_EQUAL(success(), vote(N(producvoterd), { producer_names[new_prod_index] }));
      BOOST_REQUIRE_EQUAL(0, get_producer_info(producer_names[new_prod_index])["unpaid_blocks"].as<uint32_t>());
      produce_blocks(6 * 12 * 21);
      BOOST_REQUIRE(0 < get_producer_info(producer_names[new_prod_index])["unpaid_blocks"].as<uint32_t>());
      const uint32_t initial_unpaid_blocks = get_producer_info(producer_names[voted_out_index])["unpaid_blocks"].as<uint32_t>();
      produce_blocks(2 * 12 * 21);
      BOOST_REQUIRE_EQUAL(initial_unpaid_blocks, get_producer_info(producer_names[voted_out_index])["unpaid_blocks"].as<uint32_t>());
      produce_block(fc::hours(24));
      BOOST_REQUIRE_EQUAL(success(), vote(N(producvoterd), { producer_names[voted_out_index] }));
      produce_blocks(2 * 12 * 21);
      BOOST_REQUIRE(fc::crypto::public_key() != fc::crypto::public_key(get_producer_info(producer_names[voted_out_index])["producer_key"].as_string()));
      BOOST_REQUIRE_EQUAL(success(), push_action(producer_names[voted_out_index], N(claimrewards), mvo()("owner", producer_names[voted_out_index])));
   }

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( setparams, eosio_system_tester ) try {
   //install multisig contract
   abi_serializer msig_abi_ser = initialize_multisig();
   auto producer_names = active_and_vote_producers();

   //helper function
   auto push_action_msig = [&]( const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) -> action_result {
         string action_type_name = msig_abi_ser.get_action_type(name);

         action act;
         act.account = N(eosio.msig);
         act.name = name;
         act.data = msig_abi_ser.variant_to_binary( action_type_name, data, abi_serializer_max_time );

         return base_tester::push_action( std::move(act), auth ? uint64_t(signer) : signer == N(bob111111111) ? N(alice1111111) : N(bob111111111) );
   };

   // test begins
   vector<permission_level> prod_perms;
   for ( auto& x : producer_names ) {
      prod_perms.push_back( { name(x), config::active_name } );
   }

   eosio::chain::chain_config params;
   params = control->get_global_properties().configuration;
   //change some values
   params.max_block_net_usage += 10;
   params.max_transaction_lifetime += 1;

   transaction trx;
   {
      variant pretty_trx = fc::mutable_variant_object()
         ("expiration", "2020-01-01T00:30")
         ("ref_block_num", 2)
         ("ref_block_prefix", 3)
         ("max_net_usage_words", 0)
         ("max_cpu_usage_ms", 0)
         ("delay_sec", 0)
         ("actions", fc::variants({
               fc::mutable_variant_object()
                  ("account", name(config::system_account_name))
                  ("name", "setparams")
                  ("authorization", vector<permission_level>{ { config::system_account_name, config::active_name } })
                  ("data", fc::mutable_variant_object()
                   ("params", params)
                  )
                  })
         );
      abi_serializer::from_variant(pretty_trx, trx, get_resolver(), abi_serializer_max_time);
   }

   BOOST_REQUIRE_EQUAL(success(), push_action_msig( N(alice1111111), N(propose), mvo()
                                                    ("proposer",      "alice1111111")
                                                    ("proposal_name", "setparams1")
                                                    ("trx",           trx)
                                                    ("requested", prod_perms)
                       )
   );

   // get 16 approvals
   for ( size_t i = 0; i < 15; ++i ) {
      BOOST_REQUIRE_EQUAL(success(), push_action_msig( name(producer_names[i]), N(approve), mvo()
                                                       ("proposer",      "alice1111111")
                                                       ("proposal_name", "setparams1")
                                                       ("level",         permission_level{ name(producer_names[i]), config::active_name })
                          )
      );
   }

   transaction_trace_ptr trace;
   control->applied_transaction.connect([&]( const transaction_trace_ptr& t) { if (t->scheduled) { trace = t; } } );
   BOOST_REQUIRE_EQUAL(success(), push_action_msig( N(alice1111111), N(exec), mvo()
                                                    ("proposer",      "alice1111111")
                                                    ("proposal_name", "setparams1")
                                                    ("executer",      "alice1111111")
                       )
   );

   BOOST_REQUIRE( bool(trace) );
   BOOST_REQUIRE_EQUAL( 1, trace->action_traces.size() );
   BOOST_REQUIRE_EQUAL( transaction_receipt::executed, trace->receipt->status );

   produce_blocks( 250 );

   // make sure that changed parameters were applied
   auto active_params = control->get_global_properties().configuration;
   BOOST_REQUIRE_EQUAL( params.max_block_net_usage, active_params.max_block_net_usage );
   BOOST_REQUIRE_EQUAL( params.max_transaction_lifetime, active_params.max_transaction_lifetime );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
