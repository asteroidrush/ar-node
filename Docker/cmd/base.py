import warnings
import re

import docker


class DockerApi:
    files_dir = './dockerfiles/%s'
    image_name = 'asteroid_rush/'

    def __init__(self):
        self.client = docker.DockerClient(base_url='unix://var/run/docker.sock', version='auto')

    def get_dockerfile(self, name):
        return self.files_dir % (name)

    def get_tag_name(self, tag):
        old_tag = tag
        tag = re.sub(r'[^\w.-]', '_', tag)
        if tag != old_tag:
            warnings.warn("Attention: original tag has unacceptable characters. Tag was changed %s ===> %s" % (old_tag, tag))
        return tag

    def get_image_name(self, name, tag=''):
        if tag:
            tag = self.get_tag_name(tag)
            tag = ':' + tag
        return self.image_name + name + tag

    def image_exists(self, tag):
        try:
            self.client.images.get(tag)
            return True
        except docker.errors.ImageNotFound:
            return False


class Command:
    name = None

    def __init__(self, *args, **kwargs):
        self.docker_api = DockerApi()

    def add_parser(self, subparsers):
        return NotImplemented

    def exec(self, args):
        return NotImplemented
