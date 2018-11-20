from components import Wallet


class AccountManager:

    SIZES = {
        'mb': 1024*1024,
        'kb': 1024
    }

    def __init__(self, cleos, tokens_info):
        self.cleos = cleos
        self.tokens_info = tokens_info

    def create(self, name, pub):
        self.cleos.run('create account eosio %s %s' % (name, pub))

    def create_staked(self, name, pub, tokens, ram="default", net="default", cpu="default"):
        self.cleos.run('system newaccount eosio %s %s -p eosio@createaccnt' % (name, pub) )
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

        self.cleos.run("get account %s" % name)


class AccountsManager:

    government_account = 'eosio.gov'

    system_accounts = [
        'eosio.bpay',
        'eosio.names',
        'eosio.saving',
        'eosio.upay',
        government_account
    ]

    def __init__(self, wallet, cleos, tokens_info):
        self.wallet = wallet
        self.account_manager = AccountManager(cleos, tokens_info)

    def create_system_account(self, name):
        keys = self.wallet.create_keys()
        self.wallet.import_key(keys['pvt'])
        self.account_manager.create(name, keys['pub'])

    def create_system_accounts(self):
        for account_name in self.system_accounts:
            self.create_system_account(account_name)


    def create_accounts(self, accounts):
        for account in accounts:
            self.account_manager.create_staked(account['name'], account['pub'], account['tokens'], account['ram'], account['net'], account['cpu'])