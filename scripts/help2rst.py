#! /usr/bin/python2
import fileinput
import re

def read_lines():
    for line in fileinput.input():
        yield line.strip()

def filter_options(inp):
    for line in inp:
        if re.match(r'^\s*\-', line):
            yield line

def concat_options(inp):
    full_line = None
    for line in inp:
        if line.startswith('-') or line == '':
            if full_line:
                yield full_line
            full_line = line
        else:
            if full_line:
                full_line += ' ' + line
    yield full_line

def format_options(inp):
    maxlen = 0

    col1 = []
    col2 = []

    for line in inp:
        m = re.match(r'^((\-\w,\s)?\-\-\S+)\s+(.+)$', line)
        if m:
            maxlen = max(maxlen, len(m.group(1)))
            col1.append(m.group(1))
            col2.append(m.group(3))

    for c1, c2 in zip(col1, col2):
        print(('%-'+str(maxlen+2)+'s%s') % (c1, c2))

format_options(
    filter_options(
        concat_options(
            read_lines())))
