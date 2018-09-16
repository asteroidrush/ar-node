#!/usr/bin/env python3
import argparse
import json

from os.path import abspath

from scripts.boot.accounts import AccountsManager
from scripts.boot.auth import AuthManager
from scripts.boot.components import BootNode, Wallet, Cleos, Token
from scripts.boot.contracts import ContractsManager
from scripts.boot.process import ProcessManager


def read_configs():
    config_file = open(abspath('./boot_config.json'))
    return json.load(config_file)



def prepare():
    ProcessManager.init_log(open(args.log_path, 'a'))
    ProcessManager.run('killall keosd nodeos || true')
    ProcessManager.sleep(1.5)


configs = read_configs()
parser = argparse.ArgumentParser()

build_dir = configs['build_dir']
parser.add_argument('--public-key', metavar='', help="Boot Public Key",
                    default='EOS6DovkiCze69bSzptXRnth7crDP1J6XvaXu1hJMJfgWdDPC45Fy', dest="public_key")
parser.add_argument('--private-Key', metavar='', help="Boot Private Key",
                    default='5KfjdDqaKCiDpMern6mGmtL4HNzWiRxRSF5mZUg9uFDrfk3xYT1', dest="private_key")
parser.add_argument('--wallet-dir', metavar='', help="Path to wallet directory", default='./wallet/')
parser.add_argument('--genesis', metavar='', help="Path to genesis.json", default="./genesis.json")
parser.add_argument('--keosd', metavar='', help="Path to keosd binary", default=build_dir + 'programs/keosd/keosd')
parser.add_argument('--nodeos', metavar='', help="Path to nodeos binary", default=build_dir + 'programs/nodeos/nodeos')
parser.add_argument('--cleos', metavar='', help="Cleos command",
                    default=build_dir + 'programs/cleos/cleos --wallet-url http://127.0.0.1:6666 ')
parser.add_argument('--log-path', metavar='', help="Path to log file", default='./output.log')
parser.add_argument('--contracts-dir', metavar='', help="Path to contracts directory", default=build_dir + 'contracts/')

args = parser.parse_args()

prepare()

node = BootNode(args.nodeos, args.genesis)
node.start(args.public_key, args.private_key)

cleos = Cleos(args.cleos)

wallet = Wallet(args.keosd, args.wallet_dir, cleos)
wallet.start()
wallet.import_key(args.private_key)

accounts_manager = AccountsManager(wallet, cleos, configs)
accounts_manager.create_system_accounts()

contracts_manager = ContractsManager(args.contracts_dir, accounts_manager, cleos)
contracts_manager.install_base_contracts()

for data in [configs['system_token'], configs['support_token']]:
    token = Token(data['name'], data['max_supply'], cleos)
    token.create()
    if data['supply']:
        token.issue(data['supply'])


contracts_manager.install_system_contract()

accounts_manager.create_management_accounts()

auth_manager = AuthManager(cleos)

auth_manager.resign(AccountsManager.government_account, [account['name'] for account in configs['accounts'] if account['management']])
auth_manager.resign('eosio', [AccountsManager.government_account])
for a in AccountsManager.system_accounts:
    auth_manager.resign(a, ['eosio'])
