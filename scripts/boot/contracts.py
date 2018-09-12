class ContractsManager:
    system_contracts = ["eosio.token", "eosio.msig"]

    def __init__(self, dir, accounts_manager, cleos):
        self.cleos = cleos
        self.accounts_manager = accounts_manager
        self.dir = dir

    def install(self, account_name, contract_name):
        self.cleos.run(('set contract %s ' + self.dir + '%s/') % (account_name, contract_name))

    def install_base_contracts(self):
        for contract_name in self.system_contracts:
            self.accounts_manager.create_system_account(contract_name)
            self.install(contract_name, contract_name)

    def install_system_contract(self):
        self.install('eosio', 'eosio.system')
        self.cleos.run('push action eosio setpriv \'["eosio.msig", 1]\' -p eosio@active -x 1000')
