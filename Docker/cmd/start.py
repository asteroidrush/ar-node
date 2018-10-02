import os
import re
from sys import stderr

import subprocess
from cmd.base import Command
from os.path import exists


class StartCommand(Command):
    name = 'start'

    def add_parser(self, subparsers):
        parser = subparsers.add_parser("start", help='Starts nodeos or keos')
        parser.add_argument('-e', '--environment', choices=['prod', 'test', 'dev'], help='Environment of node',
                            required=True)
        parser.add_argument('-t', '--tag', type=str, help='Tag in git repository', default=None)
        parser.add_argument('-c', '--component', choices=['nodeos-boot', 'nodeos', 'nodeos-clean', 'keos'],
                            help='Component to start', default='nodeos')

    def check_all_images_exists(self, tag):
        images = ['base', 'boot', 'node', 'keos']
        all_exists = True

        for im_name in images:
            full_im_name = self.docker_api.get_image_name(im_name, tag)
            if not self.docker_api.image_exists(full_im_name):
                print("Image \"%s\" doest not exists" % full_im_name, file=stderr)
                all_exists = False

        return all_exists

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

    def set_version(self, env, version):
        with open(os.path.join(env, '.env'), 'w') as f:
            f.write('VERSION=%s' % version)

    def exec(self, args):
        tag = args.tag

        if not tag:
            tag = self.get_version(args.environment)
            if not tag:
                raise Exception("Tag is undefined, please provide -t/--tag parameter to start component")

        version = self.docker_api.get_tag_name(tag)
        if not self.check_all_images_exists(version):
            print("Some images does not exists, please build that before launching", file=stderr)
            return

        if args.tag:
            self.set_version(args.environment, version)
        subprocess.call([
            "docker-compose",
            "up", args.component
        ], cwd=os.path.join(args.environment, ''))
