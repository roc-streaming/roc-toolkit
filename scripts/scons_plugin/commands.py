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

def ClangFormat(env, srcdir):
    return env.Action(
        '{} -i {}'.format(
            env['CLANG_FORMAT'],
            ' '.join(map(str,
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
        '{python_cmd} scripts/scons_helpers/format-header.py {src_dir}'.format(
            python_cmd=env.GetPythonExecutable(),
            src_dir=env.Dir(srcdir).path),
        env.PrettyCommand('FMT', env.Dir(srcdir).path, 'yellow')
    )

def Doxygen(env, build_dir='', html_dir=None, config='', sources=[], werror=False):
    target = os.path.join(build_dir, '.done')

    dirs = [env.Dir(build_dir).path]
    if html_dir:
        dirs += [env.Dir(html_dir).path]

    env.Command(target, sources + [config], SCons.Action.CommandAction(
        ('{python_cmd} scripts/scons_helpers/docfilt.py {proj_dir} {work_dir} {out_dirs} '
         '{touch_file} {werror} {cmd} {arg}').format(
            python_cmd=env.GetPythonExecutable(),
            proj_dir=env.Dir('#').path,
            work_dir=env.Dir(os.path.dirname(config)).path,
            out_dirs=':'.join(dirs),
            touch_file=env.File(target).path,
            werror=int(werror or 0),
            cmd=env['DOXYGEN'],
            arg=env.File(config).name),
        cmdstr = env.PrettyCommand('DOXYGEN', env.Dir(build_dir).path, 'purple')))

    return target

def Sphinx(env, output_type, build_dir, output_dir, source_dir, sources, werror=False):
    target = os.path.join(build_dir, '.done')

    env.Command(target, sources, SCons.Action.CommandAction(
        ('{python_cmd} scripts/scons_helpers/docfilt.py {proj_dir} {work_dir} {out_dirs} '
         '{touch_file} {werror} {cmd} -j {njobs} -q -b {builder} -d {cache_dir} {src_dir} '
         '{out_dir}').format(
            python_cmd=env.GetPythonExecutable(),
            proj_dir=env.Dir('#').path,
            work_dir=env.Dir('#').path,
            out_dirs=env.Dir(output_dir).path,
            touch_file=env.File(target).path,
            werror=int(werror or 0),
            cmd=env['SPHINX_BUILD'],
            njobs=SCons.Script.GetOption('num_jobs'),
            builder=output_type,
            cache_dir=env.Dir(build_dir).path,
            src_dir=env.Dir(source_dir).path,
            out_dir=env.Dir(output_dir).path),
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
        '{ragel_cmd} -o "{target}" "{source}"'.format(
            ragel_cmd=ragel,
            target=os.path.join(os.path.dirname(source.path), target_name),
            source=source.srcnode().path),
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
        ('{gengetopt_cmd} -i "{src}" -F "{file_name}" --output-dir "{out_dir}" '
         '--set-version "{ver}"').format(
            gengetopt_cmd=gengetopt,
            src=source.srcnode().path,
            file_name=source_name,
            out_dir=os.path.dirname(source.path),
            ver=ver),
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
    def copy(target, source, env):
        shutil.copy(source[0].path, target[0].path)

    actions =  [
        env.Action(
            copy,
            env.PrettyCommand('CP', env.File(dst).path, 'yellow'),
            ),
        SCons.Action.CommandAction(
            '$STRIP $STRIPFLAGS $TARGET',
            cmdstr=env.PrettyCommand('STRIP', '$TARGET', 'red'),
            ),
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
            symlink, env.PrettyCommand('LN', dst.path, 'yellow', 'ln({})'.format(dst.path))))

    return ret

def NeedsFixupSharedLibrary(env):
    return env.get('INSTALL_NAME_TOOL', None) is not None

def FixupSharedLibrary(env, path):
    return [
        SCons.Action.CommandAction(
            '$INSTALL_NAME_TOOL -id "{0}" "{0}"'.format(path),
            cmdstr=env.PrettyCommand('FIXUP', path, 'yellow')),
            ]

def ComposeStaticLibraries(env, dst_lib, src_libs):
    dst_lib = env['LIBPREFIX'] + dst_lib + env['LIBSUFFIX']

    action = SCons.Action.CommandAction(
        '{python_cmd} scripts/scons_helpers/compose-libs.py '
        '{dst_lib} {src_libs} AR={ar_exe}'.format(
            python_cmd=env.GetPythonExecutable(),
            dst_lib=quote(env.File(dst_lib).path),
            src_libs=' '.join([quote(env.File(lib).path) for lib in src_libs]),
            ar_exe=quote(env['AR'])),
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
