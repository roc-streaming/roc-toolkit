import SCons.Script
import sys
import re
import os.path
import shutil

def PythonExecutable(env):
    base = os.path.basename(sys.executable)
    path = env.Which(base)
    if path and path[0] == sys.executable:
        return base
    else:
        return sys.executable

def ClangDBWriter(env, tool, build_dir):
    return '%s scripts/wrappers/clangdb.py "%s" "%s" "%s"' % (
        env.PythonExecutable(),
        env.Dir('#').path,
        env.Dir(build_dir).path,
        tool)

def Doxygen(env, build_dir='', html_dir=None, config='', sources=[], werror=False):
    target = os.path.join(env.Dir(build_dir).path, '.done')

    if not env.Which(env['DOXYGEN']):
        env.Die("doxygen not found in PATH (looked for '%s')" % env['DOXYGEN'])

    dirs = [env.Dir(build_dir).path]
    if html_dir:
        dirs += [env.Dir(html_dir).path]

    env.Command(target, sources + [config], SCons.Action.CommandAction(
        '%s scripts/wrappers/doc.py %s %s %s %s %s %s %s' % (
            env.PythonExecutable(),
            env.Dir('#').path,
            env.Dir(os.path.dirname(config)).path,
            ':'.join(dirs),
            target,
            int(werror or 0),
            env['DOXYGEN'],
            env.File(config).name),
        cmdstr = env.PrettyCommand('DOXYGEN', build_dir, 'purple')))

    return target

def Sphinx(env, output_type, build_dir, output_dir, source_dir, sources, werror=False):
    target = os.path.join(env.Dir(build_dir).path, source_dir, '.done.'+output_type)

    if not env.Which(env['SPHINX_BUILD']):
        env.Die("sphinx-build not found in PATH (looked for '%s')" % env['SPHINX_BUILD'])

    env.Command(target, sources, SCons.Action.CommandAction(
        '%s scripts/wrappers/doc.py %s %s %s %s %s %s -q -b %s -d %s %s %s' % (
            env.PythonExecutable(),
            env.Dir('#').path,
            env.Dir('#').path,
            output_dir,
            target,
            int(werror or 0),
            env['SPHINX_BUILD'],
            output_type,
            env.Dir(os.path.join(build_dir, source_dir)).path,
            env.Dir(source_dir).path,
            env.Dir(output_dir).path),
        cmdstr = env.PrettyCommand('SPHINX', output_dir, 'purple')))

    return env.File(target)

def GenGetOpt(env, source, ver):
    if 'GENGETOPT' in env.Dictionary():
        gengetopt = env['GENGETOPT']

    else:
        gengetopt = 'gengetopt'

    if isinstance(gengetopt, str):
        if not env.Which(gengetopt):
            env.Die("gengetopt not found in PATH (looked for '%s')" % gengetopt)
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
            symlink, env.PrettyCommand('LN', dst.path, 'yellow', 'ln(%s)' % dst.path)))

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
            'ART', target.path, 'purple', 'art(%s)' % target.path)))

    env.Precious(dst)
    env.Requires(dst, src)

    return target

def init(env):
    env.AddMethod(PythonExecutable, 'PythonExecutable')
    env.AddMethod(ClangDBWriter, 'ClangDBWriter')
    env.AddMethod(Doxygen, 'Doxygen')
    env.AddMethod(Sphinx, 'Sphinx')
    env.AddMethod(GenGetOpt, 'GenGetOpt')
    env.AddMethod(SymlinkLibrary, 'SymlinkLibrary')
    env.AddMethod(FixupLibrary, 'FixupLibrary')
    env.AddMethod(DeleteFile, 'DeleteFile')
    env.AddMethod(DeleteDir, 'DeleteDir')
    env.AddMethod(Artifact, 'Artifact')
