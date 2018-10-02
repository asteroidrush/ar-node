/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <algorithm>
#include <vector>
#include <iterator>
#include <boost/test/unit_test.hpp>

#include <eosio/chain/controller.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <eosio/chain/abi_serializer.hpp>

#include <eosio/testing/tester.hpp>

#include <eosio/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/permutation.hpp>

#include <eosio.msig/eosio.msig.wast.hpp>
#include <eosio.msig/eosio.msig.abi.hpp>

using namespace eosio;
using namespace chain;
using tester = eosio::testing::tester;
using mvo = fc::mutable_variant_object;

BOOST_AUTO_TEST_SUITE(special_account_tests)

//Check special accounts exits in genesis
BOOST_FIXTURE_TEST_CASE(accounts_exists, tester)
{ try {

      tester test;
      chain::controller *control = test.control.get();
      chain::database &chain1_db = control->db();

      auto nobody = chain1_db.find<account_object, by_name>(config::null_account_name);
      BOOST_CHECK(nobody != nullptr);
      const auto& nobody_active_authority = chain1_db.get<permission_object, by_owner>(boost::make_tuple(config::null_account_name, config::active_name));
      BOOST_CHECK_EQUAL(nobody_active_authority.auth.threshold, 1);
      BOOST_CHECK_EQUAL(nobody_active_authority.auth.accounts.size(), 0);
      BOOST_CHECK_EQUAL(nobody_active_authority.auth.keys.size(), 0);

      const auto& nobody_owner_authority = chain1_db.get<permission_object, by_owner>(boost::make_tuple(config::null_account_name, config::owner_name));
      BOOST_CHECK_EQUAL(nobody_owner_authority.auth.threshold, 1);
      BOOST_CHECK_EQUAL(nobody_owner_authority.auth.accounts.size(), 0);
      BOOST_CHECK_EQUAL(nobody_owner_authority.auth.keys.size(), 0);

      auto producers = chain1_db.find<account_object, by_name>(config::producers_account_name);
      BOOST_CHECK(producers != nullptr);

      const auto& active_producers = control->head_block_state()->active_schedule;

      const auto& producers_active_authority = chain1_db.get<permission_object, by_owner>(boost::make_tuple(config::producers_account_name, config::active_name));
      auto expected_threshold = (active_producers.producers.size() * 2)/3 + 1;
      BOOST_CHECK_EQUAL(producers_active_authority.auth.threshold, expected_threshold);
      BOOST_CHECK_EQUAL(producers_active_authority.auth.accounts.size(), active_producers.producers.size());
      BOOST_CHECK_EQUAL(producers_active_authority.auth.keys.size(), 0);

      std::vector<account_name> active_auth;
      for(auto& apw : producers_active_authority.auth.accounts) {
         active_auth.emplace_back(apw.permission.actor);
      }

      std::vector<account_name> diff;
      for (int i = 0; i < std::max(active_auth.size(), active_producers.producers.size()); ++i) {
         account_name n1 = i < active_auth.size() ? active_auth[i] : (account_name)0;
         account_name n2 = i < active_producers.producers.size() ? active_producers.producers[i].producer_name : (account_name)0;
         if (n1 != n2) diff.push_back((uint64_t)n2 - (uint64_t)n1);
      }

      BOOST_CHECK_EQUAL(diff.size(), 0);

      const auto& producers_owner_authority = chain1_db.get<permission_object, by_owner>(boost::make_tuple(config::producers_account_name, config::owner_name));
      BOOST_CHECK_EQUAL(producers_owner_authority.auth.threshold, 1);
      BOOST_CHECK_EQUAL(producers_owner_authority.auth.accounts.size(), 0);
      BOOST_CHECK_EQUAL(producers_owner_authority.auth.keys.size(), 0);

      //TODO: Add checks on the other permissions of the producers account

} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(accounts_contracthost, tester)
{ try {

      auto push_action_wrap = [&]( const account_name& signer, const action_name &name, const variant_object &data) -> tester::action_result {
         return push_action(
               get_action(config::system_account_name, name, vector<permission_level>{{signer,config::active_name}}, data),
               uint64_t(signer)
         );
      };

      create_account("alice111");

      const vector<uint8_t> wasm = wast_to_wasm(eosio_msig_wast);
      auto abi = fc::json::from_string(eosio_msig_abi).template as<abi_def>();

      BOOST_REQUIRE_EQUAL(
            error("contract uploading is not allowed for this account"),
            push_action_wrap( N(alice111), N(setcode),
                            mvo()
                            ("account", "alice111")
                            ("vmtype",0)
                            ("vmversion", 0)
                            ("code", bytes(wasm.begin(), wasm.end()))
            )
      );
      BOOST_REQUIRE_EQUAL(
            error("contract uploading is not allowed for this account"),
            push_action_wrap( N(alice111), N(setabi),
                         mvo()
                         ("account", "alice111")
                         ("abi", fc::raw::pack(abi))
            )
      );

      BOOST_REQUIRE_EQUAL(
            success(),
            push_action_wrap( config::system_account_name, N(contracthost),
                              mvo()
                              ("account", "alice111")
                              ("contract_host", true)
            )
      );


      BOOST_REQUIRE_EQUAL(
            success(),
            push_action_wrap( N(alice111), N(setcode),
                              mvo()
                              ("account", "alice111")
                              ("vmtype",0)
                              ("vmversion", 0)
                              ("code", bytes(wasm.begin(), wasm.end()))
            )
      );
      BOOST_REQUIRE_EQUAL(
            success(),
            push_action_wrap( N(alice111), N(setabi),
                              mvo()
                              ("account", "alice111")
                              ("abi", fc::raw::pack(abi))
            )
      );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
