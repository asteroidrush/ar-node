import json

from math import floor

from process import ProcessManager


class AuthManager:
    def __init__(self, cleos):
        self.cleos = cleos

    def update_auth(self, account, permission, parent, controllers):
        self.cleos.run('push action eosio updateauth \'' + json.dumps({
            'account': account,
            'permission': permission,
            'parent': parent,
            'auth': {
                'threshold': floor(len(controllers) / 2) + 1, 'keys': [], 'waits': [],
                'accounts': [
                    {
                        'weight': 1,
                        'permission': {'actor': controller, 'permission': 'active'}
                    }
                    for controller in controllers
                ]
            }
        }) + '\' -p ' + account + '@' + permission)

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
        self.update_auth(name, 'owner', '', controllers)
        self.update_auth(name, 'active', 'owner', controllers)
        ProcessManager.sleep(1)
        self.cleos.run('get account ' + name)
