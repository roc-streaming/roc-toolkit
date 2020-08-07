from __future__ import print_function

import subprocess
import os
import os.path
import sys
import re
import errno

try:
    project_dir = os.path.abspath(sys.argv[1])
    working_dir = os.path.abspath(sys.argv[2])
    output_dirs = sys.argv[3].split(':')
    touch_file = sys.argv[4]
    werror = int(sys.argv[5])
    command = sys.argv[6:]
except:
   print(
    "usage: doc.py ROOT_DIR WORKING_DIR OUTPUT_DIRS TOUCH_FILE WERROR COMMAND [ARGS...]",
        file=sys.stderr)
   exit(1)

for output_dir in output_dirs:
    try:
        os.makedirs(output_dir)
    except OSError:
        e = sys.exc_info()[1]
        if e.errno != errno.EEXIST:
            print("error: unable to create '%s' directory" % output_dir, file=sys.stderr)
            print(str(e), file=sys.stderr)

try:
    os.remove(touch_file)
except:
    pass

proc = subprocess.Popen(command, cwd=working_dir, stderr=subprocess.PIPE)
err = False

for line in proc.stderr:
    line = line.decode('utf-8')

    if 'RemovedInSphinx30Warning' in line:
        continue

    line = re.sub('WARNING:', 'warning:', line, flags=re.I)
    line = re.sub('ERROR:', 'error:', line, flags=re.I)

    m = re.match('^(warning:\s*|error:\s*)?([^:.]+[.][^:.]+)(:.*)', line)
    if m:
        line = (m.group(1) or '') + os.path.relpath(m.group(2), project_dir) + m.group(3)

    if werror and re.search('warning:', line):
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
    open(touch_file, 'w').close()
except:
    e = sys.exc_info()[1]
    print("error: unable to touch '%s' file" % touch_file, file=sys.stderr)
    print(str(e), file=sys.stderr)

exit(0)
