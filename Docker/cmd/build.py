from time import sleep
import configparser
import datetime

import docker

from cmd.base import Command, DockerUtils


class BuildCommand(Command):
    name = 'build'

    def add_parser(self, subparsers):
        parser = subparsers.add_parser("build", help='Build image')
        parser.add_argument('-e', '--environment', choices=['prod', 'test', 'dev'], required=True,
                            help='Environment for resulting image')
        parser.add_argument('-t', '--tag', type=str, help='Tag in git repository', required=True)
        parser.add_argument('-f', '--force', action='store_true', help='Force building even image already exists')

    def get_credentials(self):
        credentials = configparser.ConfigParser()
        credentials.read("credentials.ini")
        return credentials['repository']

    def image_exists(self, tag):
        try:
            self.docker_client.images.get(tag)
            return True
        except docker.errors.ImageNotFound:
            return False

    def build(self, *args, **kwargs):
        need_build = kwargs.pop('rebuild') or not self.image_exists(kwargs['tag'])

        if not need_build:
            print('Image "%s" already exists, omit image building...' % kwargs['tag'])
            return

        print("Start building \"%s\"" % kwargs['tag'])
        st = datetime.datetime.now()
        self.docker_client.images.build(*args, **kwargs)
        et = datetime.datetime.now()
        print("Complete \"%s\". Elapsed %s" % (kwargs['tag'], et - st))

    def exec(self, args):
        credentials = self.get_credentials()

        self.build(path='.', dockerfile=DockerUtils.get_dockerfile('Dockerfile.Builder'),
                   tag=DockerUtils.get_image_name('builder'), rebuild=False)

        self.build(path='.', dockerfile=DockerUtils.get_dockerfile('Dockerfile.Base'),
                   tag=DockerUtils.get_image_name('base', args.tag),
                   buildargs={
                       'branch': args.tag,
                       'login': credentials['login'],
                       'password': credentials['password'],
                       'environment': args.environment
                   }, rebuild=args.force)

        version = DockerUtils.get_tag_name(args.tag)
        self.build(path='.', dockerfile=DockerUtils.get_dockerfile('Dockerfile.Boot'),
                   tag=DockerUtils.get_image_name('boot', args.tag),
                   buildargs={
                       'version': version
                   }, rebuild=args.force)

        self.build(path='.', dockerfile=DockerUtils.get_dockerfile('Dockerfile.Node'),
                   tag=DockerUtils.get_image_name('node', args.tag),
                   buildargs={
                       'version': version
                   }, rebuild=args.force)

        self.build(path='.', dockerfile=DockerUtils.get_dockerfile('Dockerfile.Keos'),
                   tag=DockerUtils.get_image_name('keos', args.tag),
                   buildargs={
                       'version': version
                   }, rebuild=args.force)
