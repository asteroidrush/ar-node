import os
import re

from math import pow

from process import ProcessManager


class BootNode:

    def __init__(self, path, data_dir, genesis):
        self.path = path
        self.data_dir = data_dir
        self.genesis = genesis

    def start(self, pub_key, pvt_key):
        data_dir_str =  ' --data-dir=%s ' % self.data_dir if self.data_dir else ' '
        ProcessManager.background(self.path + data_dir_str +
                                  '-e --producer-name eosio --signature-provider "%s=KEY:%s" '
                                  '--verbose-http-errors --contracts-console '
                                  '--genesis-json %s --delete-all-blocks '
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
            self.path + ' --unlock-timeout %d --wallet-dir %s' % (
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
    def int_to_currency(value, symbol, precision):
        return ('%d.%0'+ str(precision) +'d %s') % (value // pow(10, precision), value % pow(10, precision), symbol)


class Token:

    def __init__(self, name, max_supply, precision, cleos):
        self.name = name
        self.max_supply = max_supply
        self.cleos = cleos
        self.precision = precision

    def create(self, precision):
        self.cleos.run('push action  eosio.token create \'[ "eosio", "%s" ]\' -p eosio.token@active' % (
            Wallet.int_to_currency(self.max_supply, self.name, self.precision)))

    def issue(self, supply, precision):
        self.cleos.run('push action  eosio.token issue \'[ "eosio", "%s", "memo" ]\' -p eosio@active' % (
            Wallet.int_to_currency(self.max_supply * supply, self.name, self.precision)))