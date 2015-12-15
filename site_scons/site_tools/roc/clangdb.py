from __future__ import print_function

import sys
import os
import os.path
import json
import fnmatch
import fcntl

project_dir = os.path.abspath(sys.argv[1])
build_dir = os.path.abspath(sys.argv[2])
pattern = sys.argv[3]
compiler = sys.argv[4]
compiler_args = sys.argv[5:]

source_file = None

for arg in compiler_args:
    if fnmatch.fnmatch(arg, pattern):
        source_file = os.path.join(project_dir, arg)
        break

if source_file:
    db_path = os.path.join(build_dir, "compile_commands.json")

    cmd = {
        "command": "%s %s" % (compiler, ' '.join(compiler_args)),
        "directory": project_dir,
        "file": source_file,
    }

    try:
        with open(db_path, 'a+') as fp:
            fcntl.flock(fp.fileno(), fcntl.LOCK_EX)

            try:
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
    except Exception, e:
        print("error: unable to write clangdb to %s" % db_path, file=sys.stderr)
        print(str(e), file=sys.stderr)

os.execvp(compiler, [compiler] + compiler_args)
