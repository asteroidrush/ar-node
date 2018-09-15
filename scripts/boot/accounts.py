from scripts.boot.components import Wallet


class AccountManager:

    def __init__(self, cleos, boot_configs):
        self.cleos = cleos
        self.boot_configs = boot_configs

    def create(self, name, pub):
        self.cleos.run('create account eosio %s %s' % (name, pub))

    def create_staked(self, name, pub, stake):
        token_name = self.boot_configs['system_token']['name']
        self.cleos.run('system newaccount eosio %s %s' % (name, pub) )
        self.cleos.run('transfer eosio %s "%s"' % (name, Wallet.int_to_currency(stake, token_name)))
        self.cleos.run("get account %s" % name)


class AccountsManager:

    system_accounts = [
        'eosio.bpay',
        'eosio.names',
        'eosio.saving',
        'eosio.upay'
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


    def create_management_accounts(self):
        token = self.boot_configs['system_token']
        supply = token['max_supply'] * token['supply']
        for account in self.boot_configs['accounts']:
            account_stake = account['stake'] * supply
            self.account_manager.create_staked(account['name'], account['pub'], account_stake)