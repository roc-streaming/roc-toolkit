from __future__ import print_function

import argparse
import errno
import os
import os.path
import re
import subprocess
import sys

parser = argparse.ArgumentParser(description='filter sphinx and doxygen console output')

parser.add_argument('--root-dir', dest='root_dir', type=str, required=True,
                    help='project directory')

parser.add_argument('--work-dir', dest='work_dir', type=str, required=True,
                    help='working directory')

parser.add_argument('--out-dirs', dest='out_dirs', type=str, nargs='+', required=True,
                    help='output directories')

parser.add_argument('--touch-file', dest='touch_file', type=str, required=True,
                    help='file to touch after everything is done')

parser.add_argument('--werror', dest='werror', action='store_true',
                    help='convert warnings to errors')

parser.add_argument('command', nargs=argparse.REMAINDER,
                    help='command to be executed')

if '--' not in sys.argv or sys.argv.index('--') == len(sys.argv)-1:
    parser.print_usage()
    print("error: command is required", file=sys.stderr)
    exit(1)

options = sys.argv[1:sys.argv.index('--')]
command = sys.argv[sys.argv.index('--')+1:]

args = parser.parse_args(options)

project_dir = os.path.abspath(args.root_dir)
working_dir = os.path.abspath(args.work_dir)

for out_dir in args.out_dirs:
    try:
        os.makedirs(out_dir)
    except OSError:
        e = sys.exc_info()[1]
        if e.errno != errno.EEXIST:
            print("error: unable to create '{}' directory".format(out_dir), file=sys.stderr)
            print(str(e), file=sys.stderr)
            exit(1)

try:
    os.remove(args.touch_file)
except:
    pass

proc = subprocess.Popen(command, cwd=working_dir, stderr=subprocess.PIPE)
err = False
was_warn = False

for line in proc.stderr:
    line = line.decode('utf-8')

    if not line.strip() and was_warn:
        continue

    if 'RemovedInSphinx30Warning' in line:
        was_warn = True
        continue

    if 'warnings.warn' in line:
        was_warn = True
        continue

    if 'warning: Detected potential recursive class relation' in line:
        was_warn = True
        continue

    was_warn = False

    line = re.sub('WARNING:', 'warning:', line, flags=re.I)
    line = re.sub('ERROR:', 'error:', line, flags=re.I)

    m = re.match(r'^(warning:\s*|error:\s*)?([^:.]+[.][^:.]+)(:.*)', line)
    if m:
        line = (m.group(1) or '') + os.path.relpath(m.group(2), project_dir) + m.group(3)

    if args.werror and re.search('warning:', line):
        line = re.sub('warning:', 'error:', line)
        err = True
    elif 'error:' in line:
        err = True

    print(line, file=sys.stderr)

ret = proc.wait()

if ret > 0:
    exit(ret)
elif ret < 0:
    os.kill(os.getpid(), -ret)

if err:
    exit(127)

try:
    open(args.touch_file, 'w').close()
except:
    e = sys.exc_info()[1]
    print("error: unable to touch '{}' file".format(args.touch_file), file=sys.stderr)
    print(str(e), file=sys.stderr)
    exit(1)

exit(0)
