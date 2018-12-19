import json

from math import floor

from process import ProcessManager


class AuthManager:
    def __init__(self, cleos):
        self.cleos = cleos

    def update_auth(self, account, permission, parent, auth):
        self.cleos.run('push action eosio updateauth \'' + json.dumps({
            'account': account,
            'permission': permission,
            'parent': parent,
            'auth': auth
        }) + '\' -p ' + account + '@' + permission)

    def update_key_auth(self, name, active, owner):
        self.update_auth(name, 'owner', '', {
            'threshold': 1, "waits":[], "accounts": [],
            'keys': [{'key': owner, 'weight': 1}]
        })
        self.update_auth(name, 'active', 'owner', {
            'threshold': 1, "waits":[], "accounts": [],
            'keys': [{'key': active, 'weight': 1}]
        })

    def update_multisig_auth(self, account, permission, parent, controllers):
        self.update_auth(account, permission, parent, {
            'threshold': floor(len(controllers) / 2) + 1, 'keys': [], 'waits': [],
            'accounts': [
                {
                    'weight': 1,
                    'permission': {'actor': controller, 'permission': 'active'}
                }
                for controller in controllers
            ]
        })

    def set_account_permission(self, account, permission, keys, accounts):
        self.cleos.run(
            (
                'set account permission %s %s \'' + json.dumps(
                    {
                        "threshold": 1,
                        "keys":
                        [
                            {"key": key['pub'], "weight": key['weight']}
                            for key in keys
                        ],
                        "accounts":
                        [
                            {
                                "permission":
                                {
                                    "actor": account['name'],
                                    "permission": account['permission']
                                },
                                "weight": account['weight']
                            } for account in accounts
                        ]
                    }
                ) + '\''
            ) % (account, permission)
        )

    def set_action_permission(self, account, contract_account, action, permission):
        self.cleos.run(
            'set action permission %s %s %s %s ' % (account, contract_account, action, permission)
        )


    def resign(self, name, controllers):
        self.update_multisig_auth(name, 'owner', '', controllers)
        self.update_multisig_auth(name, 'active', 'owner', controllers)
        ProcessManager.sleep(1)
        self.cleos.run('get account ' + name)
