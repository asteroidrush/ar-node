from components import Wallet
from contracts import ContractsManager


class AccountManager:

    SIZES = {
        'mb': 1024*1024,
        'kb': 1024
    }

    def __init__(self, cleos, contracts_manager, tokens_info, temp_public_key):
        self.cleos = cleos
        self.contracts_manager = contracts_manager
        self.tokens_info = tokens_info

        # necessary for centralized management during boot sequence
        self.temp_public_key = temp_public_key

    def create(self, name, pub):
        self.cleos.run('create account eosio %s %s' % (name, pub))

    def create_staked(self, name, tokens, ram="default", net="default", cpu="default", contract_host=False):
        self.cleos.run('system newaccount eosio %s %s -p eosio@createaccnt' % (name, self.temp_public_key) )
        for token_name, amount in tokens.items():
            token_data = self.tokens_info[token_name]
            self.cleos.run('transfer eosio %s "%s"' % (name, Wallet.int_to_currency(amount, token_data['shortcut'], token_data['precision'])))

        if ram != "default":
            multiplier = self.SIZES[ram[-2:]]
            size = int(ram[:-2])
            self.cleos.run("set account ram %s %d -p eosio@active" % (name, size * multiplier))

        if net == 'default':
            net = 1

        if cpu == 'default':
            cpu = 1

        if net > 1 or cpu > 1:
            self.cleos.run("set account bandwidth %s %d %d -p eosio@active" % (name, net, cpu))

        if contract_host:
            self.contracts_manager.unlock_contract_uploading(name)

        self.cleos.run("get account %s" % name)


class AccountsManager:

    government_account = 'eosio.gov'

    system_accounts = [
        'eosio.bpay',
        'eosio.names',
        'eosio.saving',
        'eosio.upay',
        government_account
    ] + ContractsManager.system_contracts

    def __init__(self, wallet, cleos, contracts_manager, auth_manager, tokens_info, contracthost_temp_key):
        self.wallet = wallet
        self.account_manager = AccountManager(cleos, contracts_manager, tokens_info, contracthost_temp_key)
        self.auth_manager = auth_manager

    def create_system_account(self, name):
        keys = self.wallet.create_keys()
        self.wallet.import_key(keys['pvt'])
        self.account_manager.create(name, keys['pub'])
        self.account_manager.contracts_manager.unlock_contract_uploading(name)

    def create_system_accounts(self):
        for account_name in self.system_accounts:
            self.create_system_account(account_name)

    def create_accounts(self, accounts):
        for account in accounts:
            self.account_manager.create_staked(account['name'], account['tokens'],
                                               account['ram'], account['net'], account['cpu'], account['contract_host'])
            permissions = account.get('permissions')
            if not permissions:
                continue

            for perm in permissions:
                self.auth_manager.set_account_permission(account['name'], perm['name'], perm['keys'], perm['accounts'])
                for action in perm['actions']:
                    self.auth_manager.set_action_permission(account['name'], account['name'], action, perm['name'])
