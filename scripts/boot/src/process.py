import signal
import subprocess
import sys
import time

import json


class ProcessManager:
    log_file = None

    @classmethod
    def init_log(cls, log_file):
        cls.log_file = log_file

    @classmethod
    def write_to_log(cls, msg):
        if cls.log_file:
            cls.log_file.write(msg)

    @classmethod
    def run(cls, args):
        print('configure.py:', args)
        cls.write_to_log(args + '\n')
        if subprocess.call(args, shell=True):
            print('configure.py: exiting because of error')
            sys.exit(1)

    @classmethod
    def retry(cls, args):
        while True:
            print('configure.py:', args)
            cls.write_to_log(args + '\n')
            if subprocess.call(args, shell=True):
                print('*** Retry')
            else:
                break

    @classmethod
    def background(cls, args):
        print('configure.py:', args)
        cls.write_to_log(args + '\n')
        return subprocess.Popen(args, shell=True)

    @classmethod
    def get_output(cls, args):
        print('configure.py:', args)
        cls.write_to_log(args + '\n')
        proc = subprocess.Popen(args, shell=True, stdout=subprocess.PIPE)
        return proc.communicate()[0].decode('utf-8')

    @classmethod
    def get_json_output(cls, args):
        print('configure.py:', args)
        cls.write_to_log(args + '\n')
        proc = subprocess.Popen(args, shell=True, stdout=subprocess.PIPE)
        return json.loads(proc.communicate()[0])

    @classmethod
    def sleep(cls, t):
        print('sleep', t, '...')
        time.sleep(t)
        print('resume')

    @classmethod
    def lock_process(cls):
        run = True

        def stop(*args):
            global run
            print("Stopping...")
            run = False

        signal.signal(signal.SIGINT, stop)
        signal.signal(signal.SIGTERM, stop)

        while cls.run:
            time.sleep(1)
