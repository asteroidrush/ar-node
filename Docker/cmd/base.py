import warnings
import re

import docker


class DockerUtils:
    files_dir = './dockerfiles/%s'
    image_name = 'asteroid_rush/'

    @classmethod
    def get_dockerfile(cls, name):
        return cls.files_dir % (name)

    @classmethod
    def get_tag_name(cls, tag):
        old_tag = tag
        tag = re.sub(r'[^\w.-]', '_', tag)
        if tag != old_tag:
            warnings.warn("Attention: original tag has unacceptable characters. Tag was changed %s ===> %s" % (old_tag, tag))
        return tag

    @classmethod
    def get_image_name(cls, name, tag=''):
        if tag:
            tag = cls.get_tag_name(tag)
            tag = ':' + tag
        return cls.image_name + name + tag


class Command:
    name = None

    def __init__(self, *args, **kwargs):
        self.docker_client = docker.DockerClient(base_url='unix://var/run/docker.sock', version='auto')

    def add_parser(self):
        return NotImplemented

    def exec(self, args):
        return NotImplemented
