from __future__ import print_function

import subprocess
import os
import os.path
import sys
import re
import errno

try:
    project_dir = os.path.abspath(sys.argv[1])
    output_dir = sys.argv[2]
    touch_file = sys.argv[3]
    werror = int(sys.argv[4])
    doxygen_cmd = sys.argv[5:]
except:
   print(
     "usage: doxygen.py ROOT_DIR OUTPUT_DIR TOUCH_FILE WERROR DOXYGEN [DOXYGEN_ARGS...]",
         file=sys.stderr)
   exit(1)

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
err = False

for line in doxygen.stderr:
   m = re.match('^([^:]+)(:.*)', line)
   if m:
      line = os.path.relpath(m.group(1), project_dir) + m.group(2)

   if werror and 'warning:' in line:
       line = line.replace('warning:', 'error:')
       err = True
   elif 'error:' in line:
       err = True

   print(line, file=sys.stderr)

doxygen.wait()

if doxygen.returncode != 0:
    exit(doxygen.returncode)

if err:
    exit(1)

try:
    open(touch_file, 'w').close()
except Exception, e:
    print("error: unable to touch '%s' file" % touch_file, file=sys.stderr)
    print(str(e), file=sys.stderr)

exit(0)
