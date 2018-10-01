#!/usr/bin/env python3

import argparse

from cmd.build import BuildCommand
from cmd.start import StartCommand
from cmd.stop import StopCommand
from cmd.status import StatusCommand

commands = {
    BuildCommand.name: BuildCommand(),
    StartCommand.name: StartCommand(),
    StopCommand.name: StopCommand(),
    StatusCommand.name: StatusCommand()
}

parser = argparse.ArgumentParser()

subparsers = parser.add_subparsers(dest="command")
subparsers.required = True

for command in commands.values():
    command.add_parser(subparsers)

args = parser.parse_args()

commands[args.command].exec(args)