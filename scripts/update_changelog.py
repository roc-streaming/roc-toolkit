#! /usr/bin/env python3

import datetime
import os
import os.path
import re
import sys
import textwrap

os.chdir(os.path.join(
    os.path.dirname(__file__), '..'))

in_path = 'docs/sphinx/development/changelog.rst'
out_path = 'debian/changelog'

print(f'Parsing {in_path}')

in_file = open(in_path)
out_file = open(out_path, 'w')

def fprint(msg=''):
    print(msg, file=out_file)

first = True

for line in in_file.readlines():
    line = line.strip()

    if line.startswith('==='):
        if not first:
            fprint()
            fprint(f' -- Roc Streaming authors <roc@freelists.org>  {prev_date}')
            fprint()
        first = False

        m = re.match('^Version\s+(\S+)\s+\((.*)\)$', prev_line)
        version, date = m.group(1), m.group(2)
        prev_date = datetime.datetime.strptime(date, "%b %d, %Y").\
            replace(tzinfo=datetime.timezone.utc).\
            strftime("%a, %d %b %Y %H:%M:%S %z")
        fprint(f'roc-toolkit ({version}) unstable; urgency=low')

    if line.startswith('---'):
        fprint()
        fprint(f'  [ {prev_line} ]')

    if line.startswith('* '):
        fprint('\n'.join(textwrap.wrap(
            line, width=80, initial_indent='  ', subsequent_indent='    ')))

    prev_line = line

fprint()
fprint(f' -- Roc Streaming authors <roc@freelists.org>  {prev_date}')
fprint()

print(f'Updated {out_path}')
