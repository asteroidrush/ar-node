from time import sleep


class ContractsManager:
    system_contracts = ["eosio.token", "eosio.msig"]

    def __init__(self, dir, cleos):
        self.cleos = cleos
        self.dir = dir

    def install(self, account_name, contract_name):
        self.cleos.run(('set contract %s ' + self.dir + '%s/') % (account_name, contract_name))

    def unlock_contract_uploading(self, account_name):
        self.cleos.run('set account contracthost %s 1' % account_name)

    def install_base_contracts(self):
        for contract_name in self.system_contracts:
            self.install(contract_name, contract_name)

    def install_system_contract(self):
        self.install('eosio', 'eosio.system')
        self.cleos.run('push action eosio setpriv \'["eosio.msig", 1]\' -p eosio@active -x 1000')

    def install_contracts(self, contracts):
        for contract_data in contracts:
            self.install(contract_data['account'], contract_data['name'])
