from datetime import datetime
from pathlib import Path
import re
import subprocess
import sys

COPYRIGHT_HEADER = """
/*
 * Copyright (c) {year} Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
""".lstrip()

DOXYGEN_HEADER = """
//! @file {path}
//! @brief TODO: short file description.
""".lstrip()

PRAGMA_HEADER = """
#pragma once
""".lstrip()

H_REGEXP = re.compile(r'''
    ^
    (?P<copyright> /\*.*?Copyright.*?\*/\n )?
    \n*
    (?P<span1> .*?\n )??
    \n*
    (?P<doxygen> //!\s*@file.*?\n //!\s*@brief.*?\n )?
    \n*
    (?P<span2> .*?\n )??
    \n*
    (?P<pragma> \#pragma\s+once\n | \#ifndef\s+ROC_.*?_H_\n \#define\s+ROC_.*?_H_\n )?
    \n*
    (?P<body> .* )
    $
''', re.DOTALL | re.VERBOSE)

CPP_REGEXP = re.compile(r'''
    ^
    (?P<copyright> /\*.*?Copyright.*?\*/\n )?
    \n*
    (?P<body> .* )?
    $
''', re.DOTALL | re.VERBOSE)

PUBLIC_API_DIR = 'src/public_api/include/'

def process_h(content, file_path, doxygen_path):
    m = H_REGEXP.match(content)

    if m.group('copyright') and (m.group('doxygen') or not doxygen_path) \
       and m.group('pragma'):
        return None

    result = ''

    if m.group('copyright'):
        result += m.group('copyright') + '\n'
    else:
        result += COPYRIGHT_HEADER.format(year=datetime.now().year) + '\n'

    if m.group('span1'):
        result += m.group('span1')

    if m.group('doxygen'):
        result += m.group('doxygen') + '\n'
    elif doxygen_path:
        result += DOXYGEN_HEADER.format(path=doxygen_path) + '\n'

    if m.group('span2'):
        result += m.group('span2')

    if m.group('pragma'):
        result += m.group('pragma') + '\n'
    else:
        result += PRAGMA_HEADER + '\n'

    if m.group('body'):
        result += m.group('body')

    return result

def process_cpp(content):
    m = CPP_REGEXP.match(content)

    if m.group('copyright'):
        return None

    result = ''

    if m.group('copyright'):
        result += m.group('copyright') + '\n'
    else:
        result += COPYRIGHT_HEADER.format(year=datetime.now().year) + '\n'

    if m.group('body'):
        result += m.group('body')

    return result

def process_file(file_path):
    file_path = Path(file_path).absolute().relative_to(Path.cwd()).as_posix()

    if file_path.startswith('src/internal_modules/'):
        doxygen_path = file_path.replace('src/internal_modules/', '')
    else:
        doxygen_path = None

    with open(file_path, 'r') as fp:
        content = fp.read()

    if file_path.endswith('.h') and not file_path.startswith(PUBLIC_API_DIR):
        new_content = process_h(content, file_path, doxygen_path)
    else:
        new_content = process_cpp(content)

    if new_content and new_content != content:
        with open(file_path, 'w') as fp:
            fp.write(new_content)

def clang_format(clang_format, files):
    command = [clang_format, '--Wno-error=unknown', '-i'] + files
    proc = subprocess.Popen(command, stderr=subprocess.PIPE, universal_newlines=True)
    skip_lines = 0

    for line in proc.stderr:
        if skip_lines:
            skip_lines -= 1
            continue

        if re.search(r": warning: unknown key 'SpaceInEmptyBraces'$", line.rstrip()):
            skip_lines = 2
            continue

        print(line, end='', file=sys.stderr)

    ret = proc.wait()
    if ret > 0:
        return ret
    if ret < 0:
        return 128 - ret
    return 0

if len(sys.argv) < 3:
    print('usage: formatter.py <clang-format> <file>...', file=sys.stderr)
    exit(1)

clang_format_exe = sys.argv[1]
source_files = sys.argv[2:]

ret = clang_format(clang_format_exe, source_files)
if ret:
    exit(ret)

for file_path in source_files:
    process_file(file_path)
