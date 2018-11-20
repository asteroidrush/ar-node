#!/usr/bin/env python3
import argparse
import json

from os.path import abspath
from time import sleep
import signal

from accounts import AccountsManager
from auth import AuthManager
from components import BootNode, Wallet, Cleos, Token
from contracts import ContractsManager
from process import ProcessManager


def read_configs():
    config_file = open(abspath('./boot_config.json'))
    return json.load(config_file)


def prepare():
    ProcessManager.init_log(open(args.log_path, 'a'))
    ProcessManager.run('killall keosd nodeos || true')
    ProcessManager.sleep(1.5)


configs = read_configs()
parser = argparse.ArgumentParser()

parser.add_argument('--public-key', metavar='', help="Boot Public Key",
                    default='EOS6DovkiCze69bSzptXRnth7crDP1J6XvaXu1hJMJfgWdDPC45Fy', dest="public_key")
parser.add_argument('--private-Key', metavar='', help="Boot Private Key",
                    default='5KfjdDqaKCiDpMern6mGmtL4HNzWiRxRSF5mZUg9uFDrfk3xYT1', dest="private_key")
parser.add_argument('--data-dir', metavar='', help="Path to data directory", default='')
parser.add_argument('--wallet-dir', metavar='', help="Path to wallet directory", default='./wallet/')
parser.add_argument('--genesis-json', metavar='', help="Path to genesis.json", default="./genesis.json")
parser.add_argument('--keosd', metavar='', help="Path to keosd binary",
                    default='../../build/programs/keosd/keosd --http-server-address=127.0.0.1:8020 '
                            '--http-alias=keosd:8020 --http-alias=localhost:8020'
                    )
parser.add_argument('--nodeos', metavar='', help="Path to nodeos binary",
                    default='../../build/programs/nodeos/nodeos '
                            '--http-alias=nodeosd:8000 --http-alias=127.0.0.1:8000 '
                            '--http-alias=localhost:8000 --http-server-address=0.0.0.0:8000 '
                            '--bnet-endpoint=0.0.0.0:8001 --p2p-listen-endpoint=0.0.0.0:8002'
                    )
parser.add_argument('--cleos', metavar='', help="Cleos command",
                    default='../../build/programs/cleos/cleos --url=http://127.0.0.1:8000 --wallet-url=http://127.0.0.1:8020')
parser.add_argument('--log-path', metavar='', help="Path to log file", default='./output.log')
parser.add_argument('--contracts-dir', metavar='', help="Path to contracts directory", default='../../build/contracts/')

args = parser.parse_args()

prepare()

node = BootNode(args.nodeos, args.data_dir, args.genesis_json)
node.start(args.public_key, args.private_key)

sleep(2)

cleos = Cleos(args.cleos)

wallet = Wallet(args.keosd, args.wallet_dir, cleos)
wallet.start()
wallet.import_key(args.private_key)

auth_manager = AuthManager(cleos)

auth_manager.set_account_permission('eosio', 'createaccnt',
                                        [
                                            {
                                                'pub': 'EOS7zFCW3qHBoMt6LEjUQGDsZv12fRyb7xNC9hN3nTxK9kix7CEec',
                                                'weight': 1
                                            }
                                        ],
                                        [
                                            {
                                                'name': 'eosio',
                                                'permission': 'active',
                                                'weight': 1
                                            }
                                        ]
                                    )
auth_manager.set_action_permission('eosio', 'eosio', 'newaccount', 'createaccnt')

accounts_manager = AccountsManager(wallet, cleos, configs['tokens'])
accounts_manager.create_system_accounts()


contracts_manager = ContractsManager(args.contracts_dir, accounts_manager, cleos)
contracts_manager.install_base_contracts()
contracts_manager.install_system_contract()

for data in configs['tokens'].values():
    token = Token(data['shortcut'], data['max_supply'], data['precision'], cleos)
    token.create(data['precision'])
    if data['supply']:
        token.issue(data['supply'], data['precision'])

accounts_manager.create_accounts(configs['accounts'])

if configs['enable_government']:
    auth_manager.resign(AccountsManager.government_account,
                        [account['name'] for account in configs['accounts'] if account['management']])
    auth_manager.resign('eosio', [AccountsManager.government_account])

for a in AccountsManager.system_accounts:
    auth_manager.resign(a, ['eosio'])

run = True


def stop(*args):
    global run
    print("Stopping...")
    run = False


signal.signal(signal.SIGINT, stop)
signal.signal(signal.SIGTERM, stop)

while run:
    sleep(1)

print("Complete")
