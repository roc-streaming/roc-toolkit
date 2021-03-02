import SCons.Script
import sys
import re
import os
import os.path
import shutil
import subprocess

def CommandOutput(env, command):
    try:
        with open(os.devnull, 'w') as null:
            proc = subprocess.Popen(command,
                                    shell=True,
                                    stdin=null,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT,
                                    env=env['ENV'])
            lines = [s.decode() for s in proc.stdout.readlines()]
            output = str(' '.join(lines).strip())
            proc.terminate()
            return output
    except:
        return None

def PythonExecutable(env):
    base = os.path.basename(sys.executable)
    path = env.Which(base)
    if path and path[0] == sys.executable:
        return base
    else:
        return sys.executable

def ClangDBWriter(env, tool, build_dir):
    return '%s scripts/build/clangdb.py "%s" "%s" "%s"' % (
        env.PythonExecutable(),
        env.Dir('#').path,
        env.Dir(build_dir).path,
        tool)

def ClangFormat(env, srcdir):
    return env.Action(
        '%s -i %s' % (env['CLANG_FORMAT'], ' '.join(map(str,
            env.GlobRecursive(
                srcdir, ['*.h', '*.cpp'],
                exclude=[
                    os.path.relpath(env.File('#'+s).srcnode().abspath)
                      for s in open(env.File('#.fmtignore').abspath).read().split()])
        ))),
        env.PrettyCommand('FMT', env.Dir(srcdir).path, 'yellow')
    )

def HeaderFormat(env, srcdir):
    return env.Action(
        '%s scripts/build/format.py %s' % (env.PythonExecutable(), env.Dir(srcdir).path),
        env.PrettyCommand('FMT', env.Dir(srcdir).path, 'yellow')
    )

def Doxygen(env, build_dir='', html_dir=None, config='', sources=[], werror=False):
    target = os.path.join(build_dir, '.done')

    dirs = [env.Dir(build_dir).path]
    if html_dir:
        dirs += [env.Dir(html_dir).path]

    env.Command(target, sources + [config], SCons.Action.CommandAction(
        '%s scripts/build/docfilt.py %s %s %s %s %s %s %s' % (
            env.PythonExecutable(),
            env.Dir('#').path,
            env.Dir(os.path.dirname(config)).path,
            ':'.join(dirs),
            env.File(target).path,
            int(werror or 0),
            env['DOXYGEN'],
            env.File(config).name),
        cmdstr = env.PrettyCommand('DOXYGEN', env.Dir(build_dir).path, 'purple')))

    return target

def Sphinx(env, output_type, build_dir, output_dir, source_dir, sources, werror=False):
    target = os.path.join(build_dir, '.done')

    env.Command(target, sources, SCons.Action.CommandAction(
        '%s scripts/build/docfilt.py %s %s %s %s %s %s -j %d -q -b %s -d %s %s %s' % (
            env.PythonExecutable(),
            env.Dir('#').path,
            env.Dir('#').path,
            env.Dir(output_dir).path,
            env.File(target).path,
            int(werror or 0),
            env['SPHINX_BUILD'],
            SCons.Script.GetOption('num_jobs'),
            output_type,
            env.Dir(build_dir).path,
            env.Dir(source_dir).path,
            env.Dir(output_dir).path),
        cmdstr = env.PrettyCommand('SPHINX', env.Dir(output_dir).path, 'purple')))

    return env.File(target)

def Ragel(env, source):
    if 'RAGEL' in env.Dictionary():
        ragel = env['RAGEL']

    else:
        ragel = 'ragel'

    if not isinstance(ragel, str):
        ragel = env.File(ragel).path

    source = env.File(source)

    target_name = os.path.splitext(os.path.basename(source.path))[0] + '.cpp'
    target = os.path.join(str(source.dir), target_name)

    env.Command(target, source, SCons.Action.CommandAction(
        '%s -o "%s" "%s"' % (
            ragel,
            os.path.join(os.path.dirname(source.path), target_name),
            source.srcnode().path),
        cmdstr = env.PrettyCommand('RAGEL', '$SOURCE', 'purple')))

    return [env.Object(target)]

def GenGetOpt(env, source, ver):
    if 'GENGETOPT' in env.Dictionary():
        gengetopt = env['GENGETOPT']

    else:
        gengetopt = 'gengetopt'

    if not isinstance(gengetopt, str):
        gengetopt = env.File(gengetopt).path

    source = env.File(source)
    source_name = os.path.splitext(os.path.basename(source.path))[0]
    target = [
        os.path.join(str(source.dir), source_name + '.c'),
        os.path.join(str(source.dir), source_name + '.h'),
    ]

    env.Command(target, source, SCons.Action.CommandAction(
        '%s -i "%s" -F "%s" --output-dir "%s" --set-version "%s"' % (
            gengetopt,
            source.srcnode().path,
            source_name,
            os.path.dirname(source.path),
            ver),
        cmdstr = env.PrettyCommand('GGO', '$SOURCE', 'purple')))

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

def MaybeStripLibrary(env, dst, src, is_debug):
    def copy(target, source, env):
        shutil.copy(source[0].path, target[0].path)

    actions =  [
        env.Action(copy, env.PrettyCommand('CP', src[0].path, 'yellow'))
    ]
    if 'STRIP' in env.Dictionary() and not is_debug:
        actions += [
            SCons.Action.CommandAction('$STRIP $STRIPFLAGS $TARGET',
                cmdstr=env.PrettyCommand('STRIP', '$TARGET', 'red')),
        ]

    return env.Command(dst, src, actions)

def SymlinkLibrary(env, src):
    def symlink(target, source, env):
        os.symlink(os.path.relpath(source[0].path, os.path.dirname(target[0].path)),
                   target[0].path)

    path = src.abspath
    ret = []

    while True:
        m = re.match(r'^(.+)\.[0-9]+(\.[a-z]+)?$', path)
        if not m:
            break

        path = m.group(1) + (m.group(2) or '')

        dst = env.File(path)
        ret += [dst]

        env.Command(dst, src, env.Action(
            symlink, env.PrettyCommand('SYMLINK', dst.path, 'yellow', 'ln(%s)' % dst.path)))

    return ret

def FixupLibrary(env, path):
    if not env.Which('install_name_tool'):
        return []

    return [SCons.Action.CommandAction(
        'install_name_tool -id "%s" "%s"' % (path, path),
        cmdstr = env.PrettyCommand('FIXUP', path, 'yellow'))]

def DeleteFile(env, path):
    path = env.File(path).path

    def rmfile(target, source, env):
        if os.path.exists(path):
            os.remove(path)

    return env.Action(rmfile, env.PrettyCommand('RM', path, 'red', 'rm(%s)' % path))

def DeleteDir(env, path):
    path = env.Dir(path).path

    def rmtree(target, source, env):
        if os.path.exists(path):
            shutil.rmtree(path)

    return env.Action(rmtree, env.PrettyCommand('RM', path, 'red', 'rm(%s)' % path))

def Artifact(env, dst, src):
    def noop(target, source, env):
        pass

    target = env.File(dst)

    env.Command(dst, src, env.Action(noop, env.PrettyCommand(
            'CHECK', target.path, 'purple', 'art(%s)' % target.path)))

    env.Precious(dst)
    env.Requires(dst, src)

    return target

def init(env):
    env.AddMethod(CommandOutput, 'CommandOutput')
    env.AddMethod(PythonExecutable, 'PythonExecutable')
    env.AddMethod(ClangDBWriter, 'ClangDBWriter')
    env.AddMethod(ClangFormat, 'ClangFormat')
    env.AddMethod(HeaderFormat, 'HeaderFormat')
    env.AddMethod(Doxygen, 'Doxygen')
    env.AddMethod(Sphinx, 'Sphinx')
    env.AddMethod(Ragel, 'Ragel')
    env.AddMethod(GenGetOpt, 'GenGetOpt')
    env.AddMethod(MaybeStripLibrary, 'MaybeStripLibrary')
    env.AddMethod(SymlinkLibrary, 'SymlinkLibrary')
    env.AddMethod(FixupLibrary, 'FixupLibrary')
    env.AddMethod(DeleteFile, 'DeleteFile')
    env.AddMethod(DeleteDir, 'DeleteDir')
    env.AddMethod(Artifact, 'Artifact')
