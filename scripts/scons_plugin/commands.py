import SCons.Script
import os
import os.path
import re
import shutil
import sys

try:
    from shlex import quote
except:
    from pipes import quote

def ClangFormat(env, src_dir):
    exclude_files = [
        os.path.relpath(env.File('#'+s).srcnode().abspath)
            for s in open(env.File('#.fmtignore').abspath).read().split()
        ]

    files = env.GlobRecursive(
        src_dir, ['*.h', '*.cpp'],
        exclude=exclude_files)

    return env.Action(
        '{clang_format} -i {files}'.format(
            clang_format=quote(env['CLANG_FORMAT']),
            files=' '.join(map(str, files))),
        env.PrettyCommand('FMT', env.Dir(src_dir).path, 'yellow'))

def HeaderFormat(env, src_dir):
    return env.Action(
        '{python} scripts/scons_helpers/format-header.py {src_dir}'.format(
            python=quote(env.GetPythonExecutable()),
            src_dir=quote(env.Dir(src_dir).path)),
        env.PrettyCommand('FMT', env.Dir(src_dir).path, 'yellow'))

def SelfTest(env):
    return env.Action(
        '{python} scripts/scons_helpers/build-3rdparty.py --self-test'.format(
            python=quote(env.GetPythonExecutable())),
        env.PrettyCommand('TEST', 'build-3rdparty.py', 'green'))

def Doxygen(env, build_dir='', html_dir=None, config='', sources=[], werror=False):
    target = os.path.join(build_dir, 'commit')

    dirs = [env.Dir(build_dir).path]
    if html_dir:
        dirs += [env.Dir(html_dir).path]

    cmd = [
        quote(env.GetPythonExecutable()), 'scripts/scons_helpers/docfilt.py',
        '--root-dir', quote(env.Dir('#').path),
        '--work-dir', quote(env.Dir(os.path.dirname(config)).path),
        '--out-dirs', ' '.join(map(quote, dirs)),
        '--touch-file', quote(env.File(target).path),
        ]

    if werror:
        cmd += ['--werror']

    cmd += [
        '--',
        quote(env['DOXYGEN']),
        quote(env.File(config).name),
        ]

    env.Command(target, sources + [config], SCons.Action.CommandAction(
        ' '.join(cmd),
        cmdstr = env.PrettyCommand('DOXYGEN', env.Dir(build_dir).path, 'purple')))

    return target

def Sphinx(env, output_type, build_dir, output_dir, source_dir, sources, werror=False):
    target = os.path.join(build_dir, 'commit')

    cmd = [
        quote(env.GetPythonExecutable()), 'scripts/scons_helpers/docfilt.py',
        '--root-dir', quote(env.Dir('#').path),
        '--work-dir', quote(env.Dir('#').path),
        '--out-dirs', quote(env.Dir(output_dir).path),
        '--touch-file', quote(env.File(target).path),
        ]

    if werror:
        cmd += ['--werror']

    cmd += [
        '--',
        quote(env['SPHINX_BUILD']),
        '-j', str(SCons.Script.GetOption('num_jobs')),
        '-q',
        '-b', output_type,
        '-d', quote(env.Dir(build_dir).path),
        quote(env.Dir(source_dir).path),
        quote(env.Dir(output_dir).path),
        ]

    env.Command(target, sources, SCons.Action.CommandAction(
        ' '.join(cmd),
        cmdstr = env.PrettyCommand('SPHINX', env.Dir(output_dir).path, 'purple')))

    return env.File(target)

def Ragel(env, source, target=None):
    if 'RAGEL' in env.Dictionary():
        ragel = env['RAGEL']
    else:
        ragel = 'ragel'

    if not isinstance(ragel, str):
        ragel = env.File(ragel).path

    rl_file = env.File(source)

    cpp_file = env.File(os.path.join(
        str(source.dir),
        os.path.splitext(os.path.basename(source.path))[0] + '.cpp'))

    env.Command(cpp_file, rl_file, SCons.Action.CommandAction(
        '{ragel} -o {target} {source}'.format(
            ragel=quote(ragel),
            target=quote(cpp_file.path),
            source=quote(rl_file.srcnode().path)),
        cmdstr = env.PrettyCommand('RAGEL', '$SOURCE', 'purple')))

    return [env.Object(target=target, source=cpp_file)]

def GenGetOpt(env, source, version):
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
        ('{gengetopt} -i {source} -F {source_name} --output-dir {output_dir}'
         ' --set-version {version}').format(
            gengetopt=quote(gengetopt),
            source=quote(source.srcnode().path),
            source_name=quote(source_name),
            output_dir=quote(os.path.dirname(source.path)),
            version=quote(version)),
        cmdstr = env.PrettyCommand('GGO', '$SOURCE', 'purple')))

    return env.Object(target[0])

def SupportsRelocatableObject(env):
    if not env.get('LD', None):
        return False

    out = env.GetCommandOutput('{} -V'.format(env['LD']))
    return 'GNU' in out

def RelocatableObject(env, dst, src_list):
    dst += env['OBJSUFFIX']

    action = SCons.Action.CommandAction(
        '$LD -r $SOURCES -o $TARGET',
        cmdstr=env.PrettyCommand('LD', env.File(dst).path, 'red'))

    return env.Command(dst, src_list, [action])

def SupportsLocalizedObject(env):
    if not env.get('OBJCOPY', None):
        return False

    out = env.GetCommandOutput('{} -V'.format(env['OBJCOPY']))
    return 'GNU' in out

def LocalizedObject(env, dst, src):
    dst += env['OBJSUFFIX']

    action = SCons.Action.CommandAction(
        '$OBJCOPY --localize-hidden --strip-unneeded $SOURCE $TARGET',
        cmdstr=env.PrettyCommand('OBJCOPY', env.File(dst).path, 'red'))

    return env.Command(dst, src, [action])

def SupportsStripSharedLibrary(env):
    return env.get('STRIP', None) is not None

def StripSharedLibrary(env, dst, src):
    def _copy(target, source, env):
        shutil.copy(source[0].path, target[0].path)

    actions =  [
        env.Action(
            _copy,
            env.PrettyCommand('CP', env.File(dst).path, 'yellow'),
            ),
        SCons.Action.CommandAction(
            '$STRIP $STRIPFLAGS $TARGET',
            cmdstr=env.PrettyCommand('STRIP', '$TARGET', 'red'),
            ),
    ]

    return env.Command(dst, src, actions)

def SymlinkLibrary(env, src):
    def _symlink(target, source, env):
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
            _symlink, env.PrettyCommand('LN', dst.path, 'yellow', 'ln({})'.format(dst.path))))

    return ret

def NeedsFixupSharedLibrary(env):
    return env.get('INSTALL_NAME_TOOL', None) is not None

def FixupSharedLibrary(env, path):
    return [
        SCons.Action.CommandAction(
            '$INSTALL_NAME_TOOL -id {0} {0}'.format(quote(path)),
            cmdstr=env.PrettyCommand('FIXUP', path, 'yellow')),
            ]

def ComposeStaticLibraries(env, dst_lib, src_libs):
    dst_lib = env['LIBPREFIX'] + dst_lib + env['LIBSUFFIX']

    cmd = [
        quote(env.GetPythonExecutable()), 'scripts/scons_helpers/compose-libs.py',
        '--out', quote(env.File(dst_lib).path),
        '--in', ' '.join([quote(env.File(lib).path) for lib in src_libs]),
        ]

    if env['ROC_PLATFORM'] == 'darwin' and env['ROC_MACOS_ARCH']:
        cmd += ['--arch', ' '.join(env['ROC_MACOS_ARCH'])]

    cmd += ['--tools']
    for tool in ['AR', 'OBJCOPY', 'LIPO']:
        if env.get(tool, None):
            cmd += [
                '{}={}'.format(tool, quote(env[tool])),
                ]

    action = SCons.Action.CommandAction(
        ' '.join(cmd),
        cmdstr=env.PrettyCommand('COMPOSE', env.File(dst_lib).path, 'red'))

    return env.Command(dst_lib, [src_libs[0]], [action])

def DeleteFile(env, path):
    path = env.File(path).path

    def rmfile(target, source, env):
        if os.path.exists(path):
            os.remove(path)

    return env.Action(rmfile, env.PrettyCommand('RM', path, 'red', 'rm({})'.format(path)))

def DeleteDir(env, path):
    path = env.Dir(path).path

    def rmtree(target, source, env):
        if os.path.exists(path):
            shutil.rmtree(path)

    return env.Action(rmtree, env.PrettyCommand('RM', path, 'red', 'rm({})'.format(path)))

def Artifact(env, dst, src):
    def noop(target, source, env):
        pass

    target = env.File(dst)

    env.Command(dst, src, env.Action(noop, env.PrettyCommand(
        'ART', target.path, 'yellow', 'art({})'.format(target.path))))

    env.Precious(dst)
    env.Requires(dst, src)

    return target

def init(env):
    env.AddMethod(ClangFormat, 'ClangFormat')
    env.AddMethod(HeaderFormat, 'HeaderFormat')
    env.AddMethod(SelfTest, 'SelfTest')
    env.AddMethod(Doxygen, 'Doxygen')
    env.AddMethod(Sphinx, 'Sphinx')
    env.AddMethod(Ragel, 'Ragel')
    env.AddMethod(GenGetOpt, 'GenGetOpt')
    env.AddMethod(SupportsRelocatableObject, 'SupportsRelocatableObject')
    env.AddMethod(RelocatableObject, 'RelocatableObject')
    env.AddMethod(SupportsLocalizedObject, 'SupportsLocalizedObject')
    env.AddMethod(LocalizedObject, 'LocalizedObject')
    env.AddMethod(SupportsStripSharedLibrary, 'SupportsStripSharedLibrary')
    env.AddMethod(StripSharedLibrary, 'StripSharedLibrary')
    env.AddMethod(SymlinkLibrary, 'SymlinkLibrary')
    env.AddMethod(NeedsFixupSharedLibrary, 'NeedsFixupSharedLibrary')
    env.AddMethod(FixupSharedLibrary, 'FixupSharedLibrary')
    env.AddMethod(ComposeStaticLibraries, 'ComposeStaticLibraries')
    env.AddMethod(DeleteFile, 'DeleteFile')
    env.AddMethod(DeleteDir, 'DeleteDir')
    env.AddMethod(Artifact, 'Artifact')
