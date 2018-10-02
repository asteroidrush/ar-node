import os
import subprocess
from sys import stderr

from cmd.base import Command


class StopCommand(Command):
    name = 'stop'

    def add_parser(self, subparsers):
        parser = subparsers.add_parser("stop", help='Stops node or keos')
        parser.add_argument('-e', '--environment', choices=['prod', 'test', 'dev'], help='Environment of node',
                            required=True)
        parser.add_argument('-c', '--component', choices=['nodeos-boot', 'nodeos', 'nodeos-clean', 'keos'],
                            help='Component to stop', default='nodeos')

    def exec(self, args):
        subprocess.call([
            "docker-compose",
            "stop", "-t 10", args.component
        ], cwd=os.path.join(args.environment, ''))
