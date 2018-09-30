from cmd.base import Command


class StopCommand(Command):
    name = 'stop'

    def add_parser(self, subparsers):
        parser = subparsers.add_parser("stop", help='Stops node or keos')
        parser.add_argument('-e', '--environment', choices=['prod', 'test', 'dev'], help='Environment of node',
                            required=True)
        parser.add_argument('-c', '--component', choices=['boot', 'node', 'node-clean', 'keos'],
                            help='Component to stop', default='node')

    def exec(self, args):
        pass
