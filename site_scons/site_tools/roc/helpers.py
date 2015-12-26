from __future__ import print_function

import sys
import os
import os.path
import shutil
import fnmatch
import copy
import re
import subprocess

import SCons.Script

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
    try:
        proc = subprocess.Popen([compiler, '--version'],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
    except:
        return None

    while 1:
        line = proc.stdout.readline()
        if not line:
            break

        m = re.search('([0-9]\.[0-9.]+)', line)
        if m:
            return tuple(map(int, m.group(1).split('.')))

    return None

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

    if not env.Which(doxygen):
        env.Die("doxygen not found in PATH (looked for `%s')" % doxygen)

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
    if 'GENGETOPT' in env.Dictionary():
        gengetopt = env['GENGETOPT']

    else:
        gengetopt = 'gengetopt'

    if isinstance(gengetopt, str):
        if not env.Which(gengetopt):
            env.Die("gengetopt not found in PATH (looked for `%s')" % gengetopt)
    else:
        gengetopt = env.File(gengetopt).path

    source = env.File(source)
    source_name = os.path.splitext(os.path.basename(source.path))[0]
    target = [
        os.path.join(str(source.dir), source_name + '.c'),
        os.path.join(str(source.dir), source_name + '.h'),
    ]

    env.Command(target, source, SCons.Action.CommandAction(
        '%s -i %s -F %s --output-dir=%s --set-version=%s' % (
            gengetopt,
            source.srcnode().path,
            source_name,
            os.path.dirname(source.path),
            ver),
        cmdstr = env.Pretty('GGO', '$SOURCE', 'purple')))

    ret = env.Object(target[0])

    # Workaround to avoid rebuilding returned object file twice.
    #
    # What happens here:
    #  1. When invoked first time, SCons generates .c from .ggo. When
    #     SCons scans .c files for #includes, .c doesn't exist yet,
    #     thus it's not scanned.
    #  2. When invoked second time, SCons scans genarated .c file and
    #     notes that is depends on generated .h file. Since it's new
    #     dependency for this file, SCons rebuilds it.
    #
    # We force implicit dependency .c -> .h, so that it's present even
    # if SCons didn't scan .c file yet, and no new dependencies will be
    # introduced on next invocation.
    ret[0].add_to_implicit([env.File(target[1])])

    return ret

def ThridParty(env, toolchain, name, includes=[]):
    if not os.path.exists(os.path.join('3rdparty', name, 'commit')):
        if env.Execute(
            SCons.Action.CommandAction(
                '%s scripts/3rdparty.py 3rdparty "%s" %s' % (
                    env.Python(),
                    toolchain,
                    name),
                cmdstr = env.Pretty('GET', name, 'yellow'))):
            env.Die("can't make `%s', see `3rdparty/%s/build.log' for details" % (
                name, name))

    if not includes:
        includes = ['']

    for s in includes:
        env.Prepend(CPPPATH=[
            '#3rdparty/%s/include/%s' % (name, s)
        ])

    for lib in env.RecursiveGlob('#3rdparty/%s/lib' % name, 'lib*'):
        env.Append(LIBS=[env.File(lib)])

def DeleteDir(env, path):
    path = env.Dir(path).path
    def rmtree(target, source, env):
        if os.path.exists(path):
            shutil.rmtree(path)
    return env.Action(rmtree, env.Pretty('RM', path, 'red', 'rm(%s)' % path))

def TryParseConfig(env, cmd):
    if 'PKG_CONFIG' in env.Dictionary():
        pkg_config = env['PKG_CONFIG']
    elif env.Which('pkg-config'):
        pkg_config = 'pkg-config'
    else:
        return False

    try:
        env.ParseConfig('%s %s' % (pkg_config, cmd))
        return True
    except:
        return False

def CheckLibWithHeaderExpr(context, libs, headers, language, expr):
    if not isinstance(libs, list):
        libs = [libs]

    if not isinstance(headers, list):
        headers = [headers]

    suffix = '.%s' % language
    includes = '\n'.join(['#include <%s>' % h for h in ['stdio.h'] + headers])
    src = """
%s

int main() {
    printf("%%d\\n", (int)(%s));
    return 0;
}
""" % (includes, expr)

    context.Message("Checking for %s library %s... " % (
        language.upper(), libs[0]))

    err, out = context.RunProg(src, suffix)

    if not err and out.strip() != '0':
        context.Result('yes')
        return True
    else:
        context.Result('no')
        return False

def CheckProg(context, prog):
    context.Message("Checking for executable %s... " % prog)

    if context.env.Which(prog):
        context.Result('yes')
        return True
    else:
        context.Result('no')
        return False

def Init(env):
    env.AddMethod(Die, 'Die')
    env.AddMethod(RecursiveGlob, 'RecursiveGlob')
    env.AddMethod(Which, 'Which')
    env.AddMethod(Python, 'Python')
    env.AddMethod(CompilerVersion, 'CompilerVersion')
    env.AddMethod(ClangDB, 'ClangDB')
    env.AddMethod(Doxygen, 'Doxygen')
    env.AddMethod(GenGetOpt, 'GenGetOpt')
    env.AddMethod(ThridParty, 'ThridParty')
    env.AddMethod(DeleteDir, 'DeleteDir')
    env.AddMethod(TryParseConfig, 'TryParseConfig')
    env.CustomTests = {
        'CheckLibWithHeaderExpr': CheckLibWithHeaderExpr,
        'CheckProg': CheckProg,
    }
