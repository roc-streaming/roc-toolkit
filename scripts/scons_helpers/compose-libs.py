from __future__ import print_function

import argparse
import glob
import os.path
import shutil
import subprocess
import sys
import tempfile

try:
    from shlex import quote
except:
    from pipes import quote

VERBOSE = False

def path_dirname(path):
    return os.path.dirname(path)

def path_basename(path):
    return os.path.splitext(os.path.basename(path))[0]

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

def execute_command(cmd):
    if VERBOSE:
        print(cmd, file=sys.stderr)
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out = proc.stdout.read().decode()
    code = proc.wait()
    if code != 0:
        print(cmd, file=sys.stderr)
        print(out, file=sys.stderr)
        print("command exited with code {}".format(code))
        exit(1)

parser = argparse.ArgumentParser(description='compose multiple static libs into one')

parser.add_argument('--out', dest='output', type=str, required=True,
                    help='output library')

parser.add_argument('--in', dest='inputs', type=str, nargs='+', required=True,
                    help='input libraries')

parser.add_argument('--arch', dest='arch', type=str, nargs='*',
                    help='binary achitecture(s)')

parser.add_argument('--tools', dest='tools', type=str, nargs='*',
                    help='paths to tools')

parser.add_argument('--verbose', dest='verbose', action='store_true',
                    help='enable verbose output')

args = parser.parse_args()

VERBOSE = args.verbose

dst_lib = os.path.abspath(args.output)
src_libs = [os.path.abspath(arg) for arg in args.inputs]

archs = args.arch or [None]
is_fat = len(archs) > 1

tools = dict()
for e in args.tools:
    k, v = e.split('=', 1)
    tools[k] = v

for exe in ['ar', 'objcopy', 'lipo']:
    if not tools.get(exe.upper(), None):
        tools[exe.upper()] = exe

have_gnu_objcopy = is_gnu_tool(tools['OBJCOPY'])

try:
    temp_dir = tempfile.mkdtemp()
    os.chdir(temp_dir)

    # remove output lib(s)
    for path in glob.glob(dst_lib+'*'):
        os.remove(path)

    for arch in archs:
        dst_arch_lib = '{}/{}.{}.a'.format(
            path_dirname(dst_lib),
            path_basename(dst_lib),
            arch)

        for path in glob.glob(dst_arch_lib+'*'):
            os.remove(path)

    for src_lib in src_libs:
        for arch in archs:
            basename = path_basename(src_lib)

            # unpack object files for given arch from input lib
            if is_fat:
                arch_lib = '{}.{}.a'.format(basename, arch)

                execute_command(
                    '{lipo} -extract_family {arch} -output {arch_lib} {src_lib}'.format(
                        lipo=quote(tools['LIPO']),
                        arch=quote(arch),
                        arch_lib=quote(arch_lib),
                        src_lib=quote(src_lib)))

                execute_command(
                    '{ar} x {arch_lib}'.format(
                        ar=quote(tools['AR']),
                        arch_lib=quote(arch_lib)))

                os.remove(arch_lib)
            else:
                execute_command(
                    '{ar} x {src_lib}'.format(
                        ar=quote(tools['AR']),
                        src_lib=quote(src_lib)))

            for obj in list(glob.glob('*.o')):
                new_obj = '{}-{}'.format(basename, obj)

                # transform object file
                if have_gnu_objcopy:
                    execute_command(
                        '{objcopy} --localize-hidden --strip-unneeded {obj} {new_obj}'.format(
                            objcopy=quote(tools['OBJCOPY']),
                            obj=quote(obj),
                            new_obj=quote(new_obj)))
                    os.remove(obj)
                else:
                    os.rename(obj, new_obj)

                # add object file to resulting static lib for given arch
                if is_fat:
                    dst_arch_lib = '{}/{}.{}.a'.format(
                        path_dirname(dst_lib),
                        path_basename(dst_lib),
                        arch)

                    # quick mode (disable checks, disable symbol index)
                    execute_command(
                        '{ar} qS {dst_arch_lib} {new_obj}'.format(
                            ar=quote(tools['AR']),
                            dst_arch_lib=quote(dst_arch_lib),
                            new_obj=quote(new_obj)))
                else:
                    execute_command(
                        '{ar} r {dst_lib} {new_obj}'.format(
                            ar=quote(tools['AR']),
                            dst_lib=quote(dst_lib),
                            new_obj=quote(new_obj)))

                os.remove(new_obj)

    # compose per-arch static libs into one
    if is_fat:
        dst_arch_libs = [
            '{}/{}.{}.a'.format(
                    path_dirname(dst_lib),
                    path_basename(dst_lib),
                    arch)
            for arch in archs]

        # build symbol index
        for dst_arch_lib in dst_arch_libs:
            execute_command(
                '{ar} s {dst_arch_lib}'.format(
                    ar=quote(tools['AR']),
                    dst_arch_lib=quote(dst_arch_lib)))

        execute_command(
              '{lipo} -create -output {dst_lib} {dst_arch_libs}'.format(
                  lipo=quote(tools['LIPO']),
                  dst_lib=quote(dst_lib),
                  dst_arch_libs=' '.join(map(quote, dst_arch_libs))))

except:
    if os.path.isfile(dst_lib):
        os.remove(dst_lib)
    raise

finally:
    if os.path.isdir(temp_dir):
        shutil.rmtree(temp_dir)
