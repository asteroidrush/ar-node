import os
import re

from os.path import exists

import docker

from cmd.base import Command


class StatusCommand(Command):
    name = 'status'
    components = ['nodeos-boot', 'nodeos-clean',  'nodeos',  'keos']
    environments = ['prod', 'test', 'dev']

    def add_parser(self, subparsers):
        parser = subparsers.add_parser("status", help='Show information about services')
        parser.add_argument('-e', '--environment', choices=['prod', 'test', 'dev'], default=None,
                            help='Environment of container')

    def get_version(self, env):
        env_file_path = os.path.join(env, '.env')
        if not exists(env_file_path):
            return None

        with open(env_file_path, 'r') as f:
            version = f.read()
            res = re.search('VERSION=(\w+)', version)
            if res:
                return res.group(1)
            return None

    def _print_component_status(self, name, status):
        print("%-15s| %s" % (name, status))

    def _print_components_header(self, env):
        print("\n")
        self._print_delimeter()
        print("%s environment: " % env.capitalize())
        self._print_delimeter()
        self._print_component_status("Name", "Status")
        self._print_delimeter()

    def _print_delimeter(self):
        print('-'*30)

    def display_components_status(self, env):
        self._print_components_header(env)
        for component in self.components:
            try:
                container_info = self.docker_api.client.containers.get(
                    '%s_%s_1' % (env, component))
                status = container_info.status
            except docker.errors.NotFound:
                status = "not found"

            self._print_component_status(component, status)
        self._print_delimeter()

    def exec(self, args):
        if args.environment:
            self.display_components_status(args.environment)
            print("\n")
            return

        for env in self.environments:
            self.display_components_status(env)
        print("\n")
