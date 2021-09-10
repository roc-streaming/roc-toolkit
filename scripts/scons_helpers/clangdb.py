from __future__ import print_function

import sys
import os
import os.path
import fnmatch
import json
import shlex

try:
    import fcntl

    def flock(fp):
        fcntl.flock(fp.fileno(), fcntl.LOCK_EX)

    def funlock(fp):
        fcntl.flock(fp.fileno(), fcntl.LOCK_UN)
except:
    import msvcrt

    def flock(fp):
        fp.seek(0)
        msvcrt.locking(fp.fileno(), msvcrt.LK_LOCK, 1)

    def funlock(fp):
        fp.seek(0)
        msvcrt.locking(fp.fileno(), msvcrt.LK_UNLCK, 1)

if len(sys.argv) < 5:
   print("usage: clangdb.py ROOT_DIR BUILD_DIR COMPILER COMPILER_ARGS...",
         file=sys.stderr)
   exit(1)

root_dir = os.path.abspath(sys.argv[1])
build_dir = os.path.abspath(sys.argv[2])
compiler = sys.argv[3]
compiler_args = sys.argv[4:]

source_file = None

for arg in compiler_args:
    for pattern in ['*.c', '*.cpp']:
        if fnmatch.fnmatch(arg, pattern):
            source_file = os.path.join(root_dir, arg)
            break
    if source_file:
        break

if source_file:
    db_path = os.path.join(build_dir, "compile_commands.json")

    cmd = {
        "command": "%s %s" % (compiler, ' '.join(compiler_args)),
        "directory": root_dir,
        "file": source_file,
    }

    try:
        with open(db_path, 'a+') as fp:
            try:
                flock(fp)

                try:
                    fp.seek(0)
                    db = json.loads(fp.read())
                    db[:]
                except:
                    db = []

                for index, item in enumerate(db):
                    if item["file"] == source_file:
                        db[index] = cmd
                        break
                else:
                    db.append(cmd)

                fp.seek(0)
                fp.truncate()
                fp.write(json.dumps(db, indent=2))
            finally:
                funlock(fp)
    except:
        e = sys.exc_info()[1]
        print("error: unable to write clangdb to %s" % db_path, file=sys.stderr)
        print(str(e), file=sys.stderr)

cmd = shlex.split(compiler) + compiler_args
os.execvp(cmd[0], cmd)
