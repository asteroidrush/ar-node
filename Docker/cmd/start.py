from cmd.base import Command


class StartCommand(Command):
    name = 'start'

    def add_parser(self, subparsers):
        parser = subparsers.add_parser("start", help='Starts node or keos')
        parser.add_argument('-e', '--environment', choices=['prod', 'test', 'dev'], help='Environment of node',
                            required=True)
        parser.add_argument('-t', '--tag', type=str, help='Tag in git repository', required=True)
        parser.add_argument('-c', '--component', choices=['boot', 'node', 'node-clean', 'keos'],
                            help='Component to start', default='node')

    def exec(self, args):
        pass
