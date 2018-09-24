import os
import re

from scripts.boot.process import ProcessManager


class BootNode:

    def __init__(self, path, genesis):
        self.path = path
        self.genesis = genesis

    def start(self, pub_key, pvt_key):
        ProcessManager.background(self.path +
                                  ' -e --producer-name eosio --private-key \'["%s","%s"]\' '
                                  '--verbose-http-errors --contracts-console '
                                  '--genesis-json %s '
                                  '--max-transaction-time 1000 '
                                  '--plugin eosio::producer_plugin --plugin eosio::chain_api_plugin '
                                  '--plugin eosio::http_plugin' % (pub_key, pvt_key, os.path.abspath(self.genesis)))

class Cleos:

    def __init__(self, path):
        self.path = path

    def run(self, command):
        return ProcessManager.run(self.path + ' ' + command)

    def background(self, command):
        return ProcessManager.background(self.path + ' ' + command)

    def get_output(self, command):
        return ProcessManager.get_output(self.path + ' ' + command)

    def get_output_json(self, command):
        return ProcessManager.get_json_output(self.path + ' ' + command)

class Wallet:

    def __init__(self, path, wallet_dir, cleos):
        self.path = path
        self.wallet_dir = wallet_dir
        self.cleos = cleos

    def reset(self):
        ProcessManager.run('rm -rf ' + os.path.abspath(self.wallet_dir))
        ProcessManager.run('mkdir -p ' + os.path.abspath(self.wallet_dir))

    def start(self):
        self.reset()
        ProcessManager.background(
            self.path + ' --unlock-timeout %d --http-server-address 127.0.0.1:6666 --wallet-dir %s' % (
                999999999, os.path.abspath(self.wallet_dir)))
        ProcessManager.sleep(.4)
        self.cleos.run('wallet create --to-console')

    def create_keys(self):
        keys = self.cleos.get_output('create key --to-console')
        r = re.match('Private key: *([^ \n]*)\nPublic key: *([^ \n]*)', keys, re.DOTALL | re.MULTILINE)
        return {
            'pvt': r[1],
            'pub': r[2]
        }

    def import_key(self, pvt):
        self.cleos.run('wallet import --private-key %s' % pvt)

    @staticmethod
    def int_to_currency(value, symbol):
        return '%d.%04d %s' % (value // 10000, value % 10000, symbol)


class Token:

    def __init__(self, name, max_supply, cleos):
        self.name = name
        self.max_supply = max_supply
        self.cleos = cleos

    def create(self):
        self.cleos.run('push action  eosio.token create \'[ "eosio", "%s" ]\' -p eosio.token@active' % (
            Wallet.int_to_currency(self.max_supply, self.name)))

    def issue(self, supply):
        self.cleos.run('push action  eosio.token issue \'[ "eosio", "%s", "memo" ]\' -p eosio@active' % (
            Wallet.int_to_currency(self.max_supply * supply, self.name)))