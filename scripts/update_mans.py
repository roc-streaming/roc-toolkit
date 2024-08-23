#! /usr/bin/env python3

import fileinput
import os
import os.path
import re
import subprocess

# read output of <tool> --help and produce list of lines
def read_help(tool):
    exe_file = f'bin/x86_64-pc-linux-gnu/{tool}'

    return subprocess.check_output(
        [exe_file, '--help']).decode().splitlines()

# reads list of lines, returns list of sections with options
def parse_options(inp):
    section_name = 'General options'
    section_opts = []
    last_opt = None

    for line in inp:
        if re.match(r'^[A-Za-z].+:$', line):
            if last_opt:
                section_opts.append(last_opt)
                last_opt = None
            if section_opts:
                yield (section_name, section_opts)
            section_name = line.rstrip(':')
            section_opts = []
        elif re.match(r'^\s+\-.+', line):
            if last_opt:
                section_opts.append(last_opt)
            last_opt = line.strip()
        else:
            if last_opt:
                last_opt = (last_opt + ' ' + line.strip()).strip()

    if last_opt:
        section_opts.append(last_opt)
    if section_opts:
        yield (section_name, section_opts)

# reads list of sections with options, produces list of formatted lines
def format_options(inp):
    first = True

    for section_name, section_opts in inp:
        if first:
            first = False
        else:
            yield ''
        yield section_name
        yield ''.join(['-']*len(section_name))
        yield ''

        maxlen = 0
        column1 = []
        column2 = []

        for opt in section_opts:
            m = re.match(r'^((\-\w,\s)?\-\-\S+)\s+(.+)$', opt)
            if m:
                maxlen = max(maxlen, len(m.group(1)))
                column1.append(m.group(1))
                column2.append(m.group(3))

        for c1, c2 in zip(column1, column2):
            yield ('{:<' + str(maxlen + 2) + '}{}').format(c1, c2)

# updates options section in .rst file
def update_rst(tool, new_opts):
    rst_file = f'docs/sphinx/manuals/{tool.replace("-","_")}.rst'
    print(f'Updating {rst_file}')

    in_options = False
    with fileinput.input(files=[rst_file], inplace=True) as fp:
        for line in fp:
            if re.match(r'^\.\. begin_options', line):
                in_options = True
                print(line)
            elif re.match(r'^\.\. end_options', line):
                in_options = False
            if in_options:
                if new_opts:
                    print('\n'.join(new_opts))
                    print()
                    new_opts = None
            else:
                print(line, end='')

os.chdir(os.path.join(
    os.path.dirname(__file__), '..'))

for tool in ['roc-send', 'roc-recv', 'roc-copy']:
    options = format_options(
        parse_options(
            read_help(tool)))
    update_rst(tool, options)
