/**
 *  @file api_tests.cpp
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <boost/test/unit_test.hpp>
#pragma GCC diagnostic pop

#include <eosio/testing/tester.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/resource_limits.hpp>

#include <fc/exception/exception.hpp>
#include <fc/variant_object.hpp>

#include "eosio_system_tester.hpp"

#include <test_ram_limit/test_ram_limit.abi.hpp>
#include <test_ram_limit/test_ram_limit.wast.hpp>

#define DISABLE_EOSLIB_SERIALIZE
#include <test_api/test_api_common.hpp>

/*
 * register test suite `ram_tests`
 */
BOOST_AUTO_TEST_SUITE(ram_tests)



/*************************************************************************************
 * ram_tests test case
 *************************************************************************************/
BOOST_FIXTURE_TEST_CASE(ram_tests, eosio_system::eosio_system_tester) { try {

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
