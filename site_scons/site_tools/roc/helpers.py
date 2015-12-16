from __future__ import print_function

import sys
import os
import os.path
import fnmatch
import copy
import re
import subprocess

import SCons.Script

def DeleteDir(env, path):
    path = env.Dir(path).path
    command = 'rm -rf %s' % path
    return env.Action(command, env.Pretty('RM', path, 'red'))

def Die(env, fmt, *args):
    print('error: ' + (fmt % args).strip() + '\n', file=sys.stderr)
    SCons.Script.Exit(1)

def RecursiveGlob(env, dirs, patterns, exclude=[]):
    if not isinstance(dirs, list):
        dirs = [dirs]

    if not isinstance(patterns, list):
        patterns = [patterns]

    if not isinstance(exclude, list):
        exclude = [exclude]

    matches = []

    for pattern in patterns:
        for root in dirs:
            for root, dirnames, filenames in os.walk(env.Dir(root).srcnode().abspath):
                for filename in fnmatch.filter(filenames, pattern):
                    cwd = env.Dir('.').srcnode().abspath

                    abspath = os.path.join(root, filename)
                    relpath = os.path.relpath(abspath, cwd)

                    for ex in exclude:
                        if fnmatch.fnmatch(relpath, ex):
                            break
                        if fnmatch.fnmatch(os.path.basename(relpath), ex):
                            break
                    else:
                        matches.append(env.File(relpath))

    return matches

def Which(env, prog):
    result = []
    exts = filter(None, os.environ.get('PATHEXT', '').split(os.pathsep))
    path = os.environ.get('PATH', None)
    if path is None:
        return []
    for p in os.environ.get('PATH', '').split(os.pathsep):
        p = os.path.join(p, prog)
        if os.access(p, os.X_OK):
            result.append(p)
        for e in exts:
            pext = p + e
            if os.access(pext, os.X_OK):
                result.append(pext)
    return result

def Python(env):
    base = os.path.basename(sys.executable)
    path = env.Which(base)
    if path and path[0] == sys.executable:
        return base
    else:
        return sys.executable

def CompilerVersion(env, compiler):
    proc = subprocess.Popen([compiler, '--version'],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    line = proc.stdout.readline()
    m = re.search('([0-9]\.[0-9.]+)', line)
    if not m:
        return (0)
    return tuple(map(int, m.group(1).split('.')))

def ClangDB(env, build_dir, pattern, compiler):
    return '%s %s/wrappers/clangdb.py %s %s "%s" %s' % (
        env.Python(),
        env.Dir(os.path.dirname(__file__)).path,
        env.Dir('#').path,
        env.Dir(build_dir).path,
        pattern,
        compiler)

def Doxygen(env, output_dir, sources):
    target = os.path.join(env.Dir(output_dir).path, '.done')

    if 'DOXYGEN' in env.Dictionary():
        doxygen = env['DOXYGEN']
    else:
        doxygen = 'doxygen'

    env.Command(target, sources, SCons.Action.CommandAction(
        '%s %s/wrappers/doxygen.py %s %s %s %s' % (
            env.Python(),
            env.Dir(os.path.dirname(__file__)).path,
            env.Dir('#').path,
            output_dir,
            target,
            doxygen),
        cmdstr = env.Pretty('DOXYGEN', output_dir, 'purple')))

    return target

def GenGetOpt(env, source, ver):
    source = env.File(source)
    source_name = os.path.splitext(os.path.basename(source.path))[0]
    target = [
        os.path.join(str(source.dir), source_name + '.c'),
        os.path.join(str(source.dir), source_name + '.h'),
    ]

    env.Command(target, source, SCons.Action.CommandAction(
        'gengetopt -F %s --output-dir=%s --set-version=%s < %s' % (
            source_name,
            os.path.dirname(source.path),
            ver,
            source.srcnode().path),
        cmdstr = env.Pretty('GGO', '$SOURCE', 'purple')))

    return [env.Object(target[0])]

def Init(env):
    env.AddMethod(DeleteDir, 'DeleteDir')
    env.AddMethod(Die, 'Die')
    env.AddMethod(RecursiveGlob, 'RecursiveGlob')
    env.AddMethod(Which, 'Which')
    env.AddMethod(Python, 'Python')
    env.AddMethod(CompilerVersion, 'CompilerVersion')
    env.AddMethod(ClangDB, 'ClangDB')
    env.AddMethod(Doxygen, 'Doxygen')
    env.AddMethod(GenGetOpt, 'GenGetOpt')
