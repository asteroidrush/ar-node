#!/usr/bin/env python3
import argparse
import json

from os.path import abspath
from time import sleep

from src.accounts import AccountsManager
from src.auth import AuthManager
from src.components import BootNode, Wallet, Cleos, Token
from src.contracts import ContractsManager
from src.process import ProcessManager


'''================= Read configurations ================='''

configs = json.load(open(abspath('./boot_config.json')))
parser = argparse.ArgumentParser()

parser.add_argument('--public-key', metavar='', help="Boot Public Key",
                    default='EOS6DovkiCze69bSzptXRnth7crDP1J6XvaXu1hJMJfgWdDPC45Fy', dest="public_key")
parser.add_argument('--private-Key', metavar='', help="Boot Private Key",
                    default='5KfjdDqaKCiDpMern6mGmtL4HNzWiRxRSF5mZUg9uFDrfk3xYT1', dest="private_key")
parser.add_argument('--faucet-public-key', metavar='', help="Faucet public key",
                    default='EOS7zFCW3qHBoMt6LEjUQGDsZv12fRyb7xNC9hN3nTxK9kix7CEec', dest="faucet_public_key")
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

'''================= Clear running instances ================='''

ProcessManager.init_log(open(args.log_path, 'a'))
ProcessManager.run('killall keosd nodeos || true')
ProcessManager.sleep(1.5)


'''================= Initialize base components ================='''

node = BootNode(args.nodeos, args.data_dir, args.genesis_json)
node.start(args.public_key, args.private_key)

sleep(2)

cleos = Cleos(args.cleos)

wallet = Wallet(args.keosd, args.wallet_dir, cleos)
wallet.start()
wallet.import_key(args.private_key)

'''================= Blockchain initialization ================='''

auth_manager = AuthManager(cleos)

auth_manager.set_account_permission('eosio', 'createaccnt',
                                    [
                                        {
                                            'pub': args.faucet_public_key,
                                            'weight': 1
                                        }
                                    ],
                                    [
                                        {
                                            'permission': {
                                                'actor': 'eosio',
                                                'permission': 'active'
                                            },
                                            'weight': 1
                                        }
                                    ]
                                    )
auth_manager.set_action_permission('eosio', 'eosio', 'newaccount', 'createaccnt')

contracts_manager = ContractsManager(args.contracts_dir, cleos)
accounts_manager = AccountsManager(wallet, cleos, contracts_manager, configs['tokens'], args.public_key)

accounts_manager.create_system_accounts()

contracts_manager.unlock_contract_uploading("eosio")
contracts_manager.install_base_contracts()
contracts_manager.install_system_contract()

for data in configs['tokens'].values():
    token = Token(data['shortcut'], data['max_supply'], data['precision'], cleos)
    token.create()
    if data['supply']:
        token.issue(data['supply'])

accounts_manager.create_accounts(configs['accounts'])

contracts_manager.install_contracts(configs['contracts'])

# Setup parameters blockchain parameters

params_mapping = {
    'max_ram': 'setmaxram',
    'max_accounts': 'setmaxaccounts',
    'payment_bucket_per_year': 'setpaymentbucketperyear'
}


for key, value in configs['params'].items():
    if value != 'default':
        cleos.run("system %s %s" % (params_mapping[key], value))


# All configs were applied, now we can setup real permissions
for account in configs['accounts']:

    permissions = account.get('permissions')

    if not permissions:
        auth_manager.update_key_auth(account['name'], account['pub'], account['pub'])
        continue

    if account.get('pub'):
        raise Exception("You can't set both pub and permissions fields")

    for perm in permissions:
        auth_manager.set_account_permission(account['name'], perm['name'], perm['keys'], perm['accounts'])
        for action in perm['actions']:
            auth_manager.set_action_permission(account['name'], action['code'], action['name'], perm['name'])

for a in AccountsManager.system_accounts:
    auth_manager.resign(a, ['eosio'])

if configs['enable_government']:
    auth_manager.resign(AccountsManager.government_account,
                        [account['name'] for account in configs['accounts'] if account['management']])
    auth_manager.resign('eosio', [AccountsManager.government_account])


'''================= Lock current process ================='''

ProcessManager.lock_process()

print("Complete")
