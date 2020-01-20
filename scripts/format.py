#! /usr/bin/python2
import os
import re
import sys
import fnmatch
import tempfile
import shutil
import datetime

copyright_str = '''
/*
 * Copyright (c) %s Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
''' % datetime.datetime.now().year

def is_header(path):
    return re.search('\.h$', path)

def is_test(path):
    _, basename = os.path.split(path)
    return basename.startswith('test_')

def is_lib(path):
    rootname = os.path.basename(os.path.dirname(os.path.dirname(os.path.abspath(path))))
    return rootname == 'library'

def make_guard(path):
    dirpath, basename = os.path.split(path)
    dirname = os.path.basename(dirpath)
    if is_lib(path):
        arr = ['roc', basename]
    elif not dirname.startswith('roc_') and dirname != 'roc':
        arr = [os.path.basename(os.path.dirname(dirpath)), dirname, basename]
    else:
        arr = [dirname, basename]
    return '_'.join(arr).replace('.', '_').upper() + '_'

def make_doxygen_path(path):
    path = '/'.join(path.split(os.sep))
    path = re.sub('^\.?/', '', path)
    return '@file %s' % path

def make_doxygen_brief(text):
    if not text.endswith('.'):
        text += '.'
    return '@brief %s' % text

def format_file(output, path):
    def fprint(s):
        output.write(s + '\n')

    with open(path) as fp:
        lines = fp.read().splitlines()

    has_copyright, has_doxygen, has_guard, is_autogen = False, False, False, False

    section = 'copyright'
    brief = 'TODO'
    pre = []

    if is_header(path):
        while re.match(r'^\s*$', lines[-1]):
            lines.pop()

        if re.match(r'^\s*#\s*endif.*$', lines[-1]):
            lines.pop()

        while re.match(r'^\s*$', lines[-1]):
            lines.pop()

        for line in lines:
            m = re.search(r'@brief\s+(.*)', line)
            if m:
                brief = m.group(1)
                break

    while lines:
        line = lines.pop(0)

        if section in ['copyright', 'doxygen', 'guard']:
            if re.match(r'^\s*$', line):
                continue

        if section == 'copyright':
            pre += [line]

            if re.match(r'^\s*/?\*', line):
                if re.match(r'^\s*/?\*.*AUTO-GENERATED.*', line):
                    is_autogen = True

                if re.match(r'^\s*/?\*\s*(Copyright|Mozilla)', line):
                    has_copyright = True

                if re.match(r'^\s*\*/', line):
                    if is_autogen or has_copyright:
                        for p in pre:
                            fprint(p)
                        fprint('')
                    else:
                        fprint(copyright_str.strip())
                        fprint('')

                        if not has_copyright:
                            for p in pre:
                                fprint(p)
                        fprint('')

                    section = 'doxygen'
            else:
                fprint(copyright_str.strip())
                fprint('')
                lines = pre + lines
                section = 'doxygen'
                continue

        if section == 'doxygen':
            if re.match(r'^\s*/?\*', line) or re.match(r'^\s*//', line):
                continue

            if re.match(r'^\s*//!', line):
                if not is_header(path) or is_test(path) or is_autogen:
                    continue

                if re.match(r'^\s*//!\s*@file', line):
                    fprint('//! %s' % make_doxygen_path(path))
                else:
                    fprint(line)

                has_doxygen = True
                continue
            else:
                if is_lib(path) or is_test(path) or is_autogen:
                    section = 'guard' if is_header(path) else 'body'
                else:
                    if not has_doxygen:
                        if is_header(path):
                            fprint('//! %s' % make_doxygen_path(path))
                            fprint('//! %s' % make_doxygen_brief(brief))
                            section = 'guard'
                        else:
                            section = 'body'

                    if is_header(path):
                        fprint('')

        if section == 'guard':
            m = re.match(r'#\s*(ifndef|define)', line)
            if m:
                has_guard = True
                fprint('#%s %s' % (m.group(1), make_guard(path)))
                continue
            else:
                if not has_guard:
                    fprint('#ifndef %s' % make_guard(path))
                    fprint('#define %s' % make_guard(path))
                fprint('')
                section = 'body'

        if section == 'body':
            fprint(line)

    if is_header(path):
        fprint('')
        fprint('#endif // %s' % make_guard(path))

def walk_dir(directory, patterns):
    for root, dirs, files in os.walk(directory):
        for basename in files:
            for pattern in patterns:
                if fnmatch.fnmatch(basename, pattern):
                    filename = os.path.join(root, basename)
                    yield filename
                    break

if len(sys.argv) > 1:
    os.chdir(sys.argv[1])

for path in walk_dir('.', ['*.h', '*.cpp']):
    with tempfile.NamedTemporaryFile('w+') as fp:
        format_file(fp, path)
        fp.flush()
        shutil.copy(fp.name, path)
