#! /usr/bin/env python3

import datetime
import fileinput
import os
import os.path
import re
import subprocess
import sys
import textwrap

os.chdir(os.path.join(
    os.path.dirname(__file__), '..'))

def update_deb_copyright():
    in_path = 'docs/sphinx/about_project/authors.rst'
    out_path = 'debian/copyright'

    print(f'updating {out_path}')

    with open(in_path, 'r') as in_file:
        in_lines = in_file.readlines()

    with open(out_path, 'r') as out_file:
        out_content = out_file.readlines()

    updated_content = []

    for line in out_content:
        updated_content.append(line)
        if line.startswith('Copyright:'):
            break

    for line in in_lines:
        if line.startswith('* '):
            updated_content.append(line.replace('* ', '  '))

    for line in out_content:
        if line.startswith('License:'):
            updated_content.append(line)
            break

    with open(out_path, 'w') as out_file:
        out_file.writelines(updated_content)

def update_deb_changelog():
    in_path = 'docs/sphinx/development/changelog.rst'
    out_path = 'debian/changelog'

    print(f'updating {out_path}')

    in_file = open(in_path, 'r')
    out_file = open(out_path, 'w')

    def fprint(msg=''):
        print(msg, file=out_file)

    first = True

    for line in in_file.readlines():
        pad = re.sub(r'^(\s*).*$', r'\1', line.rstrip())
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
            line = re.sub(r'`([^` ]+)\s*<[^` >]+>`_+', r'\1', line)
            line = re.sub(r'``', '`', line)
            fprint('\n'.join(textwrap.wrap(
                line, width=80, initial_indent=('  '+pad), subsequent_indent=('    '+pad))))

        prev_line = line

    fprint()
    fprint(f' -- Roc Streaming authors <roc@freelists.org>  {prev_date}')
    fprint()

def update_rpm_spec():
    out_path = 'rpm/roc-toolkit.spec'

    print(f'updating {out_path}')

    version = subprocess.check_output([
        sys.executable, 'scripts/scons_helpers/parse-version.py']).decode().strip()

    for line in fileinput.input(files=[out_path], inplace=True):
        m = re.match('^(Version:\s+).*$', line)
        if m:
            line = m.group(1) + version + '\n'
        print(line, end='')

update_deb_copyright()
update_deb_changelog()
update_rpm_spec()

print('done')
