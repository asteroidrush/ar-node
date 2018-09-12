import json

from scripts.boot.process import ProcessManager


class AuthManager:

    def __init__(self, cleos):
        self.cleos = cleos

    def update_auth(self, account, permission, parent, controller):
        self.cleos.run('push action eosio updateauth \'' + json.dumps({
            'account': account,
            'permission': permission,
            'parent': parent,
            'auth': {
                'threshold': 1, 'keys': [], 'waits': [],
                'accounts': [{
                    'weight': 1,
                    'permission': {'actor': controller, 'permission': 'active'}
                }]
            }
        }) + '\' -p ' + account + '@' + permission)


    def resign(self, name, controller):
        self.update_auth(name, 'owner', '', controller)
        self.update_auth(name, 'active', 'owner', controller)
        ProcessManager.sleep(1)
        self.cleos.run('get account ' + name)