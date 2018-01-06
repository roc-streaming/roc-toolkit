from __future__ import print_function

import sys
import os
import threading
import subprocess

try:
    timeout = int(sys.argv[1])
    cmd = sys.argv[2:]
    if len(cmd) == 0:
        raise ValueError()
except:
   print("usage: timeout.py TIMEOUT COMMAND...", file=sys.stderr)
   exit(1)

proc = subprocess.Popen(cmd)

def trap():
    print("timeout of %ss expired, aborting" % timeout, file=sys.stderr)
    try:
        proc.terminate()
    except:
        pass

timer = threading.Timer(timeout, trap)
timer.start()

try:
    ret = proc.wait()
    if ret >= 0:
        os._exit(ret)
    else:
        os.kill(os.getpid(), -ret)
except:
    proc.terminate()
    os._exit(127)
