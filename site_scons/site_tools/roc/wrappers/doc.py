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
    command = sys.argv[5:]
except:
   print(
     "usage: werror.py ROOT_DIR OUTPUT_DIR TOUCH_FILE WERROR COMMAND [COMMAND_ARGS...]",
         file=sys.stderr)
   exit(1)

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

proc = subprocess.Popen(command, stderr=subprocess.PIPE)
err = False

for line in proc.stderr:
   m = re.match('^([^:]+)(:.*)', line)
   if m:
      line = os.path.relpath(m.group(1), project_dir) + m.group(2)

   if werror and re.search('warning:', line, flags=re.I):
       line = re.sub('warning:', 'error:', line, flags=re.I)
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
