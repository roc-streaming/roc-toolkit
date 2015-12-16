from __future__ import print_function

import subprocess
import os
import os.path
import sys
import re
import errno

if len(sys.argv) < 5:
   print("usage: doxygen.py ROOT_DIR OUTPUT_DIR TOUCH_FILE DOXYGEN [DOXYGEN_ARGS...]",
         file=sys.stderr)
   exit(1)

project_dir = os.path.abspath(sys.argv[1])
output_dir = sys.argv[2]
touch_file = sys.argv[3]
doxygen_cmd = sys.argv[4:]

try:
    os.makedirs(output_dir)
except OSError, e:
    if e.errno != errno.EEXIST:
        print("error: unable to create '%s' directory" % output_dir, file=sys.stderr)
        print(str(e), file=sys.stderr)

try:
    os.remove(touch_file)
except:
    pass

doxygen = subprocess.Popen(doxygen_cmd, stderr=subprocess.PIPE)

for line in doxygen.stderr:
   m = re.match('^([^:]+)(:.*)', line)
   if m:
      line = os.path.relpath(m.group(1), project_dir) + m.group(2)

   print(line, file=sys.stderr)

doxygen.wait()

if doxygen.returncode == 0:
    try:
        open(touch_file, 'w').close()
    except Exception, e:
        print("error: unable to touch '%s' file" % touch_file, file=sys.stderr)
        print(str(e), file=sys.stderr)

exit(doxygen.returncode)
