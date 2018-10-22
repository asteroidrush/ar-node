from components import Wallet


class AccountManager:

    def __init__(self, cleos, boot_configs):
        self.cleos = cleos
        self.boot_configs = boot_configs

    def create(self, name, pub):
        self.cleos.run('create account eosio %s %s' % (name, pub))

    def create_staked(self, name, pub, stake):
        system_token = self.boot_configs['tokens']['system']
        self.cleos.run('system newaccount eosio %s %s -p eosio@createaccnt' % (name, pub) )
        self.cleos.run('transfer eosio %s "%s"' % (name, Wallet.int_to_currency(stake, system_token['name'], system_token['precision'])))
        self.cleos.run("get account %s" % name)


class AccountsManager:

    gateway_account = 'eosio.gate'
    government_account = 'eosio.gov'

    system_accounts = [
        'eosio.bpay',
        'eosio.names',
        'eosio.saving',
        'eosio.upay',
        government_account
    ]

    def __init__(self, wallet, cleos, boot_configs):
        self.wallet = wallet
        self.account_manager = AccountManager(cleos, boot_configs)
        self.boot_configs = boot_configs

    def create_system_account(self, name):
        keys = self.wallet.create_keys()
        self.wallet.import_key(keys['pvt'])
        self.account_manager.create(name, keys['pub'])

    def create_system_accounts(self):
        for account_name in self.system_accounts:
            self.create_system_account(account_name)

    def create_gateway_account(self, public_key):
        self.account_manager.create(self.gateway_account, public_key)


    def create_management_accounts(self):
        token = self.boot_configs['tokens']['system']
        supply = token['max_supply'] * token['supply']
        for account in self.boot_configs['accounts']:
            account_stake = account['stake'] * supply
            self.account_manager.create_staked(account['name'], account['pub'], account_stake)