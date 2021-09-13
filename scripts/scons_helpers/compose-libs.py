from __future__ import print_function

import os.path
import re
import shutil
import subprocess
import sys
import tempfile

try:
    from shlex import quote
except:
    from pipes import quote

def execute_command(cmd):
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out = proc.stdout.read()
    code = proc.wait()
    if code != 0:
        print(cmd, file=sys.stderr)
        print(out, file=sys.stderr)
        print("command exited with code %s" % code)
        exit(1)

def get_var_value(s):
    return s.split('=', 2)[1]

if len(sys.argv) < 3:
    print("usage: compose-libs.py DST_LIB SRC_LIBS... VARS...",
          file=sys.stderr)
    exit(1)

ar_exe = 'ar'

dst_lib = os.path.abspath(sys.argv[1])
src_libs = []

for arg in sys.argv[2:]:
    if arg.startswith('AR='):
        ar_exe = get_var_value(arg)
    else:
        src_libs.append(os.path.abspath(arg))

try:
    if src_libs:
        shutil.copy(src_libs[0], dst_lib)

    temp_dir = tempfile.mkdtemp()
    os.chdir(temp_dir)

    for src_lib in src_libs[1:]:
        lib_name = os.path.splitext(os.path.basename(src_lib))[0]

        execute_command('%s x %s' % (quote(ar_exe), quote(src_lib)))

        for obj in list(os.listdir('.')):
            prefixed_obj = '_%s_%s' % (lib_name, obj)
            os.rename(obj, prefixed_obj)

            execute_command('%s r %s %s' % (
                quote(ar_exe), quote(dst_lib), quote(prefixed_obj)))

            os.remove(prefixed_obj)

except:
    os.remove(dst_lib)
    raise

finally:
    shutil.rmtree(temp_dir)
