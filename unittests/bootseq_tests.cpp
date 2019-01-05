#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>

#include <eosio.system/eosio.system.wast.hpp>
#include <eosio.system/eosio.system.abi.hpp>
// These contracts are still under dev
#include <eosio.token/eosio.token.wast.hpp>
#include <eosio.token/eosio.token.abi.hpp>
#include <eosio.msig/eosio.msig.wast.hpp>
#include <eosio.msig/eosio.msig.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <eosio/chain/asset.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

#define SPT_SYMBOL SY(4,SPT)
#define SPT_SYMBOL_NAME "SPT"

eosio::chain::asset spt_from_string(const std::string& s) {
   return eosio::chain::asset::from_string(s + " " SPT_SYMBOL_NAME);
}

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

using mvo = fc::mutable_variant_object;

struct genesis_account {
   account_name aname;
   uint64_t     initial_balance;
};

std::vector<genesis_account> test_genesis( {
  {N(b1),    100'000'000'0000ll},
  {N(whale4), 40'000'000'0000ll},
  {N(whale3), 30'000'000'0000ll},
  {N(whale2), 20'000'000'0000ll},
  {N(whale1), 10'000'000'0000ll},
  {N(proda),      1'000'000'0000ll},
  {N(prodb),      1'000'000'0000ll},
  {N(prodc),      1'000'000'0000ll},
  {N(prodd),      1'000'000'0000ll},
  {N(prode),      1'000'000'0000ll},
  {N(prodf),      1'000'000'0000ll},
  {N(prodg),      1'000'000'0000ll},
  {N(prodh),      1'000'000'0000ll},
  {N(prodi),      1'000'000'0000ll},
  {N(prodj),      1'000'000'0000ll},
  {N(prodk),      1'000'000'0000ll},
  {N(prodl),      1'000'000'0000ll},
  {N(prodm),      1'000'000'0000ll},
  {N(prodn),      1'000'000'0000ll},
  {N(prodo),      1'000'000'0000ll},
  {N(prodp),      1'000'000'0000ll},
  {N(prodq),      1'000'000'0000ll},
  {N(prodr),      1'000'000'0000ll},
  {N(prods),      1'000'000'0000ll},
  {N(prodt),      1'000'000'0000ll},
  {N(produ),      1'000'000'0000ll},
  {N(runnerup1),1'000'000'0000ll},
  {N(runnerup2),1'000'000'0000ll},
  {N(runnerup3),1'000'000'0000ll},
  {N(minow1),        100'0000ll},
  {N(minow2),          1'0000ll},
  {N(minow3),          1'0000ll},
  {N(masses),800'000'000'0000ll}
});

class bootseq_tester : public TESTER {
public:

   fc::variant get_global_state() {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(global), N(global) );
      if (data.empty()) std::cout << "\nData is empty\n" << std::endl;
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "eosio_global_state", data, abi_serializer_max_time );

   }

    auto set_ram(name account, int64_t ram) {
       auto r = base_tester::push_action(config::system_account_name, N(setaccntram), config::system_account_name, mvo()
             ("account", account)
             ("ram", ram)
       );
       produce_block();
       return r;
    }

    auto set_stake(name from, name receiver, int64_t stake, uint8_t transfer = 1) {
       auto r = base_tester::push_action(N(eosio.token), N(transfer), from, mvo()
                    ("from", from )
                    ("to", receiver)
                    ("quantity", asset(stake, symbol(CORE_SYMBOL)).to_string())
                    ("memo", "Stake initial value")
                    );
       produce_block();
       return r;
    }

    void create_currency( name contract, name manager, asset maxsupply, const private_key_type* signer = nullptr ) {
        auto act =  mutable_variant_object()
                ("issuer",       manager )
                ("maximum_supply", maxsupply );

        base_tester::push_action(contract, N(create), contract, act );
    }

    auto issue( name contract, name manager, name to, asset amount ) {
       auto r = base_tester::push_action( contract, N(issue), manager, mutable_variant_object()
                ("to",      to )
                ("quantity", amount )
                ("memo", "")
        );
        produce_block();
        return r;
    }

    auto claim_rewards( name owner ) {
       auto r = base_tester::push_action( config::system_account_name, N(claimrewards), owner, mvo()("owner",  owner ));
       produce_block();
       return r;
    }

    auto set_privileged( name account ) {
       auto r = base_tester::push_action(config::system_account_name, N(setpriv), config::system_account_name,  mvo()("account", account)("is_priv", 1));
       produce_block();
       return r;
    }

    auto register_producer(name producer) {
       auto r = base_tester::push_action(config::system_account_name, N(regproducer), producer, mvo()
                       ("producer",  name(producer))
                       ("producer_key", get_public_key( producer, "active" ) )
                       ("url", "" )
                       ("location", 0 )
                    );
       produce_block();
       return r;
    }


    auto undelegate_bandwidth( name from, name receiver, asset net, asset cpu ) {
       auto r = base_tester::push_action(config::system_account_name, N(undelegatebw), from, mvo()
                    ("from", from )
                    ("receiver", receiver)
                    ("unstake_net_quantity", net)
                    ("unstake_cpu_quantity", cpu)
                    );
       produce_block();
       return r;
    }

    asset get_balance( const account_name& act ) {
         return get_currency_balance(N(eosio.token), symbol(CORE_SYMBOL), act);
    }

    void set_code_abi(const account_name& account, const char* wast, const char* abi, const private_key_type* signer = nullptr) {
       wdump((account));
        set_code(account, wast, signer);
        set_abi(account, abi, signer);
        if (account == config::system_account_name) {
           const auto& accnt = control->db().get<account_object,by_name>( account );
           abi_def abi_definition;
           BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi_definition), true);
           abi_ser.set_abi(abi_definition, abi_serializer_max_time);
        }
        produce_blocks();
    }


    abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(bootseq_tests)

BOOST_FIXTURE_TEST_CASE( bootseq_test, bootseq_tester ) {
    try {

        // Create eosio.msig and eosio.token
        create_accounts({N(eosio.msig), N(eosio.token), N(eosio.vpay), N(eosio.bpay)});

        // Set code for the following accounts:
        //  - eosio (code: eosio.bios) (already set by tester constructor)
        //  - eosio.msig (code: eosio.msig)
        //  - eosio.token (code: eosio.token)
        set_code_abi(N(eosio.msig), eosio_msig_wast, eosio_msig_abi);//, &eosio_active_pk);
        set_code_abi(N(eosio.token), eosio_token_wast, eosio_token_abi); //, &eosio_active_pk);

        // Set privileged for eosio.msig and eosio.token
        set_privileged(N(eosio.msig));
        set_privileged(N(eosio.token));

        // Verify eosio.msig and eosio.token is privileged
        const auto& eosio_msig_acc = get<account_object, by_name>(N(eosio.msig));
        BOOST_TEST(eosio_msig_acc.privileged == true);
        const auto& eosio_token_acc = get<account_object, by_name>(N(eosio.token));
        BOOST_TEST(eosio_token_acc.privileged == true);

        // Set eosio.system to eosio
        set_code_abi(config::system_account_name, eosio_system_wast, eosio_system_abi);

        TESTER::push_action(config::system_account_name, updateauth::get_name(), config::system_account_name, mvo()
             ("account",     name(config::system_account_name).to_string())
             ("permission",     "createaccnt")
             ("parent", "active")
             ("auth", authority{1,{},{
                    permission_level_weight{{config::system_account_name, config::active_name}, 1}
              }
              }
             )
        );


        TESTER::push_action(config::system_account_name, linkauth::get_name(), config::system_account_name, mvo()
             ("account",     name(config::system_account_name).to_string())
             ("code",        name(config::system_account_name).to_string())
             ("type",        "newaccount")
             ("requirement", "createaccnt")
        );

        // Create SYS tokens in eosio.token, set its manager as eosio
        auto core_max_supply = core_from_string("10000000000.0000"); /// 1x larger than 1B initial tokens
        auto initial_supply = core_from_string("10000000000.0000"); /// 1x larger than 1B initial tokens
        create_currency(N(eosio.token), config::system_account_name, core_max_supply);
        // Issue the genesis supply of 1 billion SYS tokens to eosio.system
        issue(N(eosio.token), config::system_account_name, config::system_account_name, initial_supply);

        // Create SPT tokens in eosio.token, set its manager as eosio
        auto spt_max_supply = spt_from_string("10000000000.0000"); /// 1x larger than 1B initial tokens
        create_currency(N(eosio.token), config::system_account_name, spt_max_supply);

        auto actual = get_balance(config::system_account_name);
        BOOST_REQUIRE_EQUAL(initial_supply, actual);

        // Create genesis accounts
        for( const auto& a : test_genesis ) {
           create_account( a.aname, config::system_account_name, false, true, N(createaccnt) );
        }

        for( const auto& a : test_genesis ) {
           auto ib = a.initial_balance;
           auto ram = 6000;

           auto r = set_ram(a.aname, ram);
           BOOST_REQUIRE( !r->except_ptr );

           r = set_stake(config::system_account_name, a.aname, ib);
           BOOST_REQUIRE( !r->except_ptr );
        }

        auto producer_candidates = {
                N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ),
                N(runnerup1), N(runnerup2), N(runnerup3)
        };

        // Register producers
        for( auto pro : producer_candidates ) {
           register_producer(pro);
        }

        // Vote for producers
        auto votepro = [&]( account_name voter, vector<account_name> producers ) {
          std::sort( producers.begin(), producers.end() );
          base_tester::push_action(config::system_account_name, N(voteproducer), voter, mvo()
                                ("voter",  name(voter))
                                ("proxy", name(0) )
                                ("producers", producers)
                     );
        };
        votepro( N(b1), { N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                           N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                           N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ)} );
        votepro( N(whale1), {N(runnerup1), N(runnerup2), N(runnerup3)} );
        votepro( N(whale3), {N(proda), N(prodb), N(prodc), N(prodd), N(prode)} );

        // Total Stakes = b1 + whale2 + whale3 stake
        BOOST_TEST(get_global_state()["total_activated_stake"].as<int64_t>() == 140'000'000'0000);

        // No producers will be set, since the total activated stake is less than 150,000,000
        produce_blocks_for_n_rounds(2); // 2 rounds since new producer schedule is set when the first block of next round is irreversible
        auto active_schedule = control->head_block_state()->active_schedule;
        BOOST_TEST(active_schedule.producers.size() == 1);
        BOOST_TEST(active_schedule.producers.front().producer_name == "eosio");

        // Spend some time so the producer pay pool is filled by the inflation rate
        produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(30 * 24 * 3600)); // 30 days
        // Since the total activated stake is less than 150,000,000, it shouldn't be possible to claim rewards
        BOOST_REQUIRE_THROW(claim_rewards(N(runnerup1)), eosio_assert_message_exception);

        // This will increase the total vote stake by 40,000,000
        votepro( N(whale4), {N(prodq), N(prodr), N(prods), N(prodt), N(produ)} );
        BOOST_TEST(get_global_state()["total_activated_stake"].as<int64_t>() == 180'000'000'0000);

        // Since the total vote stake is more than 150,000,000, the new producer set will be set
        produce_blocks_for_n_rounds(2); // 2 rounds since new producer schedule is set when the first block of next round is irreversible
        active_schedule = control->head_block_state()->active_schedule;
        BOOST_REQUIRE(active_schedule.producers.size() == 21);
        BOOST_TEST(active_schedule.producers.at(0).producer_name == "proda");
        BOOST_TEST(active_schedule.producers.at(1).producer_name == "prodb");
        BOOST_TEST(active_schedule.producers.at(2).producer_name == "prodc");
        BOOST_TEST(active_schedule.producers.at(3).producer_name == "prodd");
        BOOST_TEST(active_schedule.producers.at(4).producer_name == "prode");
        BOOST_TEST(active_schedule.producers.at(5).producer_name == "prodf");
        BOOST_TEST(active_schedule.producers.at(6).producer_name == "prodg");
        BOOST_TEST(active_schedule.producers.at(7).producer_name == "prodh");
        BOOST_TEST(active_schedule.producers.at(8).producer_name == "prodi");
        BOOST_TEST(active_schedule.producers.at(9).producer_name == "prodj");
        BOOST_TEST(active_schedule.producers.at(10).producer_name == "prodk");
        BOOST_TEST(active_schedule.producers.at(11).producer_name == "prodl");
        BOOST_TEST(active_schedule.producers.at(12).producer_name == "prodm");
        BOOST_TEST(active_schedule.producers.at(13).producer_name == "prodn");
        BOOST_TEST(active_schedule.producers.at(14).producer_name == "prodo");
        BOOST_TEST(active_schedule.producers.at(15).producer_name == "prodp");
        BOOST_TEST(active_schedule.producers.at(16).producer_name == "prodq");
        BOOST_TEST(active_schedule.producers.at(17).producer_name == "prodr");
        BOOST_TEST(active_schedule.producers.at(18).producer_name == "prods");
        BOOST_TEST(active_schedule.producers.at(19).producer_name == "prodt");
        BOOST_TEST(active_schedule.producers.at(20).producer_name == "produ");

        // Spend some time so the producer pay pool is filled by the inflation rate
        produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(30 * 24 * 3600)); // 30 days
        // Since the total activated stake is larger than 150,000,000, pool should be filled reward should be bigger than zero
        claim_rewards(N(runnerup1));
        BOOST_TEST(get_balance(N(runnerup1)).get_amount() > 0);

        const auto first_june_2018 = fc::seconds(1527811200); // 2018-06-01
        const auto first_june_2028 = fc::seconds(1843430400); // 2028-06-01
        // Ensure that now is yet 10 years after 2018-06-01 yet
        BOOST_REQUIRE(control->head_block_time().time_since_epoch() < first_june_2028);

        return;
        produce_blocks(7000); /// produce blocks until virutal bandwidth can acomadate a small user
        wlog("minow" );
        votepro( N(minow1), {N(p1), N(p2)} );


// TODO: Complete this test
    } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
