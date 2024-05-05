import SCons.SConf
import hashlib
import os
import os.path
import re
import textwrap

# Python 2 compatibility.
try:
    from shlex import quote
except:
    from pipes import quote

def _run_prog(context, src, suffix):
    # Workaround for a SCons bug.
    # RunProg uses a global incrementing counter for temporary .c file names. The
    # file name depends on the number of invocations of that function, but not on
    # the file contents. When the user subsequently invokes scons with different
    # options, the sequence of file contents passed to RunProg may vary. However,
    # RunProg may incorrectly use cached results from a previous run saved for
    # different file contents but the same invocation number. To prevent this, we
    # monkey patch its global counter with a hashsum of the file contents.
    # The workaround is needed only for older versions of SCons, where
    # _ac_build_counter was an integer.
    try:
        if type(SCons.SConf._ac_build_counter) is int:
            SCons.SConf._ac_build_counter = int(hashlib.md5(src.encode()).hexdigest(), 16)
    except:
        pass
    return context.RunProg(src, suffix)

def _get_llvm_dir(version):
    def macos_dirs():
        return [
        '/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin',
        '/Library/Developer/CommandLineTools/usr/bin',
        ]

    def linux_dirs():
        suffixes = []
        for n in [3, 2, 1]:
            v = '.'.join(map(str, version[:n]))
            suffixes += [
                '-' + v,
                '/' + v,
            ]
        suffixes += ['']
        ret = []
        for s in suffixes:
            ret.append('/usr/lib/llvm{}/bin'.format(s))
        return ret

    for llvmdir in macos_dirs() + linux_dirs():
        if os.path.isdir(llvmdir):
            return llvmdir

# Gets `PKG_CONFIG_PATH` from environment `env`, tries to append `add_prefix`/lib/pkgconfig, and
# returns the result.
# Tolerant to absense of any or both of those two components. If both are absent, returns an
# empty string.
# Doesn't perform quoting.
def _compose_pkg_config_path(env, add_prefix):
    pkg_config_path = env.get('PKG_CONFIG_PATH', '')
    if add_prefix:
        # set user-defined $(add_prefix)/lib/pkgconfig path for pkg-config if needed (lowest
        # priority); otherwise use original PKG_CONFIG_PATH is propagated explicitly
        add_pkg_config_path = os.path.join(add_prefix, 'lib', 'pkgconfig')
        if os.path.isdir(add_pkg_config_path):
            if pkg_config_path:
                pkg_config_path += ':'
            pkg_config_path += add_pkg_config_path
    return pkg_config_path

# Workaround for brew / pkg-config weirdeness
def _fix_brew_libpath(libpath):
    if not '/Cellar/' in libpath:
        return libpath # not in brew (quick check)

    if libpath.endswith('/lib'):
        return libpath # looks good

    brew_prefix = env.GetCommandOutput('brew --prefix')
    if not brew_prefix:
        return libpath # can't help

    if not libpath.startswith(brew_prefix+'/Cellar/'):
        return libpath # not in brew

    if not os.path.isdir(libpath+'/lib'):
        return libpath # can't help

    return libpath+'/lib'

def CheckLibWithHeaderExt(context, libs, headers, language, expr='1', run=True):
    if not isinstance(headers, list):
        headers = [headers]

    if not isinstance(libs, list):
        libs = [libs]

    name = libs[0]
    libs = [l for l in libs if not l in context.env['LIBS']]

    suffix = '.{}'.format(language.lower())
    includes = '\n'.join(['#include <{}>'.format(h) for h in ['stdio.h'] + headers])
    src = textwrap.dedent("""
        {}

        int main() {{
            printf("%d\\n", (int)({}));
            return 0;
        }}
    """).format(includes, expr)

    context.Message("Checking for {} library {} ... ".format(language.upper(), name))

    if run:
        err, out = _run_prog(context, src, suffix)
        if out.strip() == '0':
            err = True
    else:
        err = context.CompileProg(src, suffix)

    if not err:
        context.Result('yes')
        context.env.Append(LIBS=libs)
        return True
    else:
        context.Result('no')
        return False

def CheckProg(context, prog):
    context.Message("Checking for executable {} ... ".format(prog))

    path = context.env.Which(prog)
    if path:
        context.Result(path[0])
        return True
    else:
        context.Result('not found')
        return False

def CheckCanRunProgs(context):
    context.Message("Checking whether we can run compiled executables ... ")

    src = textwrap.dedent("""
        int main() {
            return 0;
        }
    """)

    err, out = _run_prog(context, src, '.c')

    if not err:
        context.Result('yes')
        return True
    else:
        context.Result('no')
        return False

def CheckCompilerOptionSupported(context, opt, language):
    context.Message("Checking whether {} compiler supports {} ... ".format(language.upper(), opt))

    ext = '.' + language.lower()
    src = "int main() { return 0; }"

    orig_env = context.env

    context.env = orig_env.DeepClone()
    context.env.Append(**{language.upper()+'FLAGS': [opt]})

    err = context.CompileProg(src, ext)

    context.env = orig_env

    if not err:
        context.Result('yes')
        return True
    else:
        context.Result('no')
        return False

def FindProgram(context, var, tool):
    if tool:
        context.Message("Searching {} executable ... ".format(var))

        tool_path = context.env.Which(tool)
        if tool_path:
            context.env[var] = tool_path[0]
        else:
            context.env[var] = tool

        context.Result(context.env[var])

    return True

def FindTool(context, var, toolchains, commands,
             compiler_dir=None, prepend_path=[], required=True):
    env = context.env

    context.Message("Searching {} executable ... ".format(var))

    if env.HasArgument(var):
        context.Result(env[var])
        return True

    toolchains = list(toolchains)

    for tc in list(toolchains):
        if '-pc-' in tc:
            toolchains += [tc.replace('-pc-', '-')]

        if 'android' in tc:
            m = re.search(r'((android(eabi)?)\d+)$', tc)
            if m:
                tc = tc.replace(m.group(1), m.group(2))
                toolchains += [tc]
            toolchains += [tc.replace('armv7a', 'arm')]

    if '' in toolchains:
        toolchains.remove('')
        toolchains.append('')

    if compiler_dir:
        toolchains.append(compiler_dir)

    found = False

    for tool_prefix in toolchains:
        for tool_cmd, tool_ver in commands:
            if isinstance(tool_cmd, list):
                tool_name = tool_cmd[0]
                tool_flags = tool_cmd[1:]
            else:
                tool_name = tool_cmd
                tool_flags = []

            if not tool_prefix:
                tool = tool_name
            elif tool_prefix == compiler_dir:
                tool = os.path.join(compiler_dir, tool_name)
            else:
                tool = '{}-{}'.format(tool_prefix, tool_name)

            if tool_ver:
                search_versions = [
                    tool_ver[:3],
                    tool_ver[:2],
                    tool_ver[:1],
                ]

                default_ver = env.ParseCompilerVersion(tool)

                if default_ver and default_ver[:len(tool_ver)] == tool_ver:
                    search_versions += [default_ver]

                for ver in reversed(sorted(set(search_versions))):
                    versioned_tool = '{}-{}'.format(tool, '.'.join(map(str, ver)))
                    if env.Which(versioned_tool, prepend_path):
                        tool = versioned_tool
                        break

            tool_path = env.Which(tool, prepend_path)
            if not tool_path:
                continue

            if tool_ver:
                actual_ver = env.ParseCompilerVersion(tool_path[0])
                if actual_ver:
                    actual_ver = actual_ver[:len(tool_ver)]

                actual_ver_short = actual_ver if len(actual_ver)<=2 else actual_ver[:2]
                tool_ver_short = tool_ver if len(tool_ver)<=2 else tool_ver[:2]

                if actual_ver_short != tool_ver_short:
                    env.Die(
                        ("problem detecting {}: "
                         "found '{}', which reports version {}, but expected version {}").format(
                            var,
                            tool_path[0],
                            '.'.join(map(str, actual_ver)) if actual_ver else '<unknown>',
                            '.'.join(map(str, tool_ver))))

            env[var] = tool_path[0]
            if tool_flags:
                env[var + 'FLAGS'] = ' '.join(tool_flags)

            found = True
            break

        if found:
            break

    if not found:
        if required:
            env.Die("can't find {} executable".format(var))
        else:
            env[var] = None
            context.Result('not found')
            return False

    message = env[var]
    realpath = os.path.realpath(env[var])
    if realpath != env[var]:
        message += ' ({})'.format(realpath)

    context.Result(message)
    return True

def FindClangFormat(context):
    env = context.env

    context.Message("Searching for clang-format ... ")

    if env.HasArgument('CLANG_FORMAT'):
        context.Result(env['CLANG_FORMAT'])
        return True

    min_ver = 10
    max_ver = 99

    def _checkver(exe):
        ver_str = env.ParseToolVersion('{exe} --version'.format(exe=exe))
        try:
            ver = tuple(map(int, ver_str.split('.')))
            return (min_ver, 0, 0) <= ver <= (max_ver, 99, 99)
        except:
            return False

    clang_format = env.Which('clang-format')
    if clang_format and _checkver(clang_format[0]):
        env['CLANG_FORMAT'] = clang_format[0]
        context.Result(env['CLANG_FORMAT'])
        return True

    for ver in range(min_ver,max_ver+1):
        clang_format = env.Which('clang-format-{}'.format(ver))
        if clang_format and _checkver(clang_format[0]):
            env['CLANG_FORMAT'] = clang_format[0]
            context.Result(env['CLANG_FORMAT'])
            return True

        llvmdir = _get_llvm_dir((ver,))
        if llvmdir:
            clang_format = os.path.join(llvmdir, 'clang-format')
            if _checkver(clang_format):
                env['CLANG_FORMAT'] = clang_format
                context.Result(env['CLANG_FORMAT'])
                return True

    env.Die("can't find clang-format >= {} and <= {}".format(min_ver, max_ver))

def FindLLVMDir(context, version):
    context.Message(
        "Searching PATH for llvm {} ... ".format('.'.join(map(str, version))))

    llvmdir = _get_llvm_dir(version)
    if llvmdir:
        context.env['ENV']['PATH'] += ':' + llvmdir
        context.Result(llvmdir)
        return True

    context.Result('not found')
    return True

def FindLibDir(context, prefix, host):
    context.Message("Searching for system library directory ... ")

    def _libdirs(host):
        dirs = ['lib/' + host]
        if 'x86_64-pc-linux-gnu' == host:
            dirs += ['lib/x86_64-linux-gnu']
        if 'x86_64' in host:
            dirs += ['lib64']
        dirs += ['lib']
        return dirs

    for d in _libdirs(host):
        libdir = os.path.join(prefix, d)
        if os.path.isdir(libdir):
            break

    context.env['ROC_SYSTEM_LIBDIR'] = libdir
    context.Result(libdir)
    return True

def FindConfigGuess(context):
    context.Message('Searching CONFIG_GUESS script ... ')

    if context.env.HasArgument('CONFIG_GUESS'):
        context.Result(context.env['CONFIG_GUESS'])
        return True

    prefixes = [
        '/usr',
        '/usr/local',
        '/usr/local/Cellar',
    ]

    dirs = [
        'share/gnuconfig',
        'share/misc',
        'share/automake-*',
        'automake/*/share/automake-*',
        'share/libtool/build-aux',
        'libtool/*/share/libtool/build-aux',
        'lib/php/build',
        'lib/php/*/build',
    ]

    for p in prefixes:
        for d in dirs:
            for f in context.env.Glob(os.path.join(p, d, 'config.guess')):
                path = str(f)
                if not os.access(path, os.X_OK):
                    continue

                context.env['CONFIG_GUESS'] = path
                context.Result(path)

                return True

    context.Result('not found')
    return False

def FindPkgConfig(context, toolchain):
    env = context.env

    context.Message('Searching PKG_CONFIG ... ')

    if env.HasArgument('PKG_CONFIG'):
        context.Result(env['PKG_CONFIG'])
        return True

    # https://autotools.io/pkgconfig/cross-compiling.html
    # http://tiny.cc/lh6upz
    if toolchain:
        pkg_config_cmd = toolchain + '-pkg-config'
        if env.Which(pkg_config_cmd):
            env['PKG_CONFIG'] = env.Which(pkg_config_cmd)[0]
            context.Result(env['PKG_CONFIG'])
            return True

        if 'PKG_CONFIG_PATH' in os.environ \
            or 'PKG_CONFIG_LIBDIR' in os.environ \
            or 'PKG_CONFIG_SYSROOT_DIR' in os.environ:
            if env.Which('pkg-config'):
                env['PKG_CONFIG'] = env.Which('pkg-config')[0]
                context.Result(env['PKG_CONFIG'])
                return True

        context.Result('not found')
        return False

    if env.Which('pkg-config'):
        env['PKG_CONFIG'] = env.Which('pkg-config')[0]
        context.Result(env['PKG_CONFIG'])
        return True

    context.Result('not found')
    return False

def FindPkgConfigPath(context, prefix):
    if not prefix.endswith(os.sep):
        prefix += os.sep

    env = context.env

    context.Message("Searching PKG_CONFIG_PATH ...")

    if env.HasArgument('PKG_CONFIG_PATH'):
        context.Result(env['PKG_CONFIG_PATH'])
        return True

    pkg_config = env.get('PKG_CONFIG', None)
    if pkg_config:
        pkg_config_path = env.GetCommandOutput(
            '{pkg_config_cmd} --variable pc_path pkg-config'.format(
                pkg_config_cmd=quote(pkg_config)))
        if pkg_config_path:
            env['PKG_CONFIG_PATH'] = pkg_config_path
            context.Result(env['PKG_CONFIG_PATH'])
            return True

    # https://linux.die.net/man/1/pkg-config the default is libdir/pkgconfig
    env['PKG_CONFIG_PATH'] = os.path.join(env['ROC_SYSTEM_LIBDIR'], 'pkgconfig')
    context.Result(env['PKG_CONFIG_PATH'])
    return True

def AddPkgConfigDependency(context, package, flags,
                           add_prefix=None, exclude_from_pc=False, exclude_libs=[]):
    context.Message("Searching pkg-config package {} ...".format(package))

    env = context.env

    pkg_config = env.get('PKG_CONFIG', None)
    if not pkg_config:
        context.Result('pkg-config not available')
        return False

    cmd = []
    pkg_config_path = _compose_pkg_config_path(env, add_prefix)
    if pkg_config_path:
        cmd = ['env', 'PKG_CONFIG_PATH={}'.format(pkg_config_path)]

    cmd += [pkg_config, package, '--silence-errors'] + flags.split()
    try:
        old_libs = env['LIBS'][:]
        old_dirs = env['LIBPATH'][:]

        env.ParseConfig(cmd)

        new_libs = env['LIBS'][:]
        new_dirs = env['LIBPATH'][:]

        for lib in exclude_libs:
            if lib not in old_libs and lib in new_libs:
                env['LIBS'].remove(lib)

        for n, libpath in enumerate(new_dirs):
            if not libpath in old_dirs:
                env['LIBPATH'][n] = _fix_brew_libpath(libpath)

        if not exclude_from_pc:
            if '_DEPS_PCFILES' not in env.Dictionary():
                env['_DEPS_PCFILES'] = []
            env.AppendUnique(_DEPS_PCFILES=[package])
    except:
        context.Result('not found')
        return False

    context.Result('yes')
    return True

def init(env):
    env.CustomTests = {
        'CheckLibWithHeaderExt': CheckLibWithHeaderExt,
        'CheckProg': CheckProg,
        'CheckCanRunProgs': CheckCanRunProgs,
        'CheckCompilerOptionSupported': CheckCompilerOptionSupported,
        'FindProgram': FindProgram,
        'FindTool': FindTool,
        'FindClangFormat': FindClangFormat,
        'FindLLVMDir': FindLLVMDir,
        'FindLibDir': FindLibDir,
        'FindConfigGuess': FindConfigGuess,
        'FindPkgConfig': FindPkgConfig,
        'FindPkgConfigPath': FindPkgConfigPath,
        'AddPkgConfigDependency': AddPkgConfigDependency,
    }
