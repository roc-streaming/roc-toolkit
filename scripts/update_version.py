#! /usr/bin/env python3

import fileinput
import os
import os.path
import re
import subprocess
import sys

os.chdir(os.path.join(
    os.path.dirname(__file__), '..'))

version = subprocess.check_output([
    sys.executable, 'scripts/scons_helpers/parse-version.py']).decode().strip()

print(("Updating to version {}").format(version))

for line in fileinput.input(files=['rpm/roc-toolkit.spec'], inplace=True):
    m = re.match('^(Version:\s+).*$', line)
    if m:
        line = ("{}{}\n").format(m.group(1),version)
    print(line, end='')

print('Done.')
