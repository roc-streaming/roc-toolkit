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
        print("command exited with code {}".format(code))
        exit(1)

def is_gnu_tool(tool):
    try:
        out = subprocess.check_output([tool, '-V'], stderr=subprocess.STDOUT)
        try:
            out = out.decode()
        except:
            pass
        try:
            out = str(out)
        except:
            pass
        return 'GNU' in out
    except:
        return False

def get_var_value(s):
    return s.split('=', 2)[1]

if len(sys.argv) < 3:
    print("usage: compose-libs.py DST_LIB SRC_LIBS... VARS...",
          file=sys.stderr)
    exit(1)

ar_exe = 'ar'
objcopy_exe = 'objcopy'

dst_lib = os.path.abspath(sys.argv[1])
src_libs = []

for arg in sys.argv[2:]:
    if arg.startswith('AR='):
        ar_exe = get_var_value(arg)
    elif arg.startswith('OBJCOPY='):
        objcopy_exe = get_var_value(arg)
    else:
        src_libs.append(os.path.abspath(arg))

is_gnu_objcopy = is_gnu_tool(objcopy_exe)

try:
    if src_libs:
        shutil.copy(src_libs[0], dst_lib)

    temp_dir = tempfile.mkdtemp()
    os.chdir(temp_dir)

    for src_lib in src_libs[1:]:
        lib_name = os.path.splitext(os.path.basename(src_lib))[0]

        # unpack objects fron static lib
        execute_command('{ar_exe} x {src_lib}'.format(
            ar_exe=quote(ar_exe), src_lib=quote(src_lib)))

        for obj in list(os.listdir('.')):
            new_obj = '_{lib_name}_{obj}'.format(lib_name=lib_name, obj=obj)

            # transform each object:
            #  - prefix object name with lib name to prevent conflicts
            #  - localize hidden symbols to unexport them from resulting static lib
            if is_gnu_objcopy:
                execute_command(
                    '{objcopy_exe} --localize-hidden --strip-unneeded {obj} {new_obj}'.format(
                        objcopy_exe=quote(objcopy_exe), obj=obj, new_obj=new_obj))
            else:
                os.rename(obj, new_obj)

            # add transformed object to resulting static lib
            execute_command('{ar_exe} r {dst_lib} {new_obj}'.format(
                ar_exe=quote(ar_exe), dst_lib=quote(dst_lib), new_obj=quote(new_obj)))

            os.remove(new_obj)

except:
    os.remove(dst_lib)
    raise

finally:
    shutil.rmtree(temp_dir)
