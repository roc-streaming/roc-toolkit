import SCons.SConf
import re
import os
import os.path
import hashlib

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
    SCons.SConf._ac_build_counter = int(hashlib.md5(src.encode()).hexdigest(), 16)
    return context.RunProg(src, suffix)

def CheckLibWithHeaderExt(context, libs, headers, language, expr='1', run=True):
    if not isinstance(headers, list):
        headers = [headers]

    if not isinstance(libs, list):
        libs = [libs]

    name = libs[0]
    libs = [l for l in libs if not l in context.env['LIBS']]

    suffix = '.%s' % language.lower()
    includes = '\n'.join(['#include <%s>' % h for h in ['stdio.h'] + headers])
    src = """
%s

int main() {
    printf("%%d\\n", (int)(%s));
    return 0;
}
""" % (includes, expr)

    context.Message("Checking for %s library %s... " % (
        language.upper(), name))

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
    context.Message("Checking for executable %s... " % prog)

    path = context.env.Which(prog)
    if path:
        context.Result(path[0])
        return True
    else:
        context.Result('not found')
        return False

def CheckCanRunProgs(context):
    context.Message("Checking whether we can run compiled executables... ")

    src = """
int main() {
    return 0;
}
"""

    err, out = _run_prog(context, src, '.c')

    if not err:
        context.Result('yes')
        return True
    else:
        context.Result('no')
        return False

def FindTool(context, var, toolchain, version, commands, prepend_path=[]):
    env = context.env

    context.Message("Searching %s executable... " % var)

    if env.HasArg(var):
        context.Result(env[var])
        return True

    toolchain_list = [toolchain]

    if 'android' in toolchain:
        m = re.search('((android(eabi)?)\d+)$', toolchain)
        if m:
            toolchain_list += [toolchain.replace(m.group(1), m.group(2))]
        toolchain_list += [toolchain_list[-1].replace('armv7a', 'arm')]

    found = False

    for tool_prefix in toolchain_list:
        for tool_cmd in commands:
            if isinstance(tool_cmd, list):
                tool_name = tool_cmd[0]
                tool_flags = tool_cmd[1:]
            else:
                tool_name = tool_cmd
                tool_flags = []

            if not tool_prefix:
                tool = tool_name
            else:
                tool = '%s-%s' % (tool_prefix, tool_name)

            if version:
                search_versions = [
                    version[:3],
                    version[:2],
                    version[:1],
                ]

                default_ver = env.ParseCompilerVersion(tool)

                if default_ver and default_ver[:len(version)] == version:
                    search_versions += [default_ver]

                for ver in reversed(sorted(set(search_versions))):
                    versioned_tool = '%s-%s' % (tool, '.'.join(map(str, ver)))
                    if env.Which(versioned_tool, prepend_path):
                        tool = versioned_tool
                        break

            tool_path = env.Which(tool, prepend_path)
            if tool_path:
                env[var] = tool_path[0]
                if tool_flags:
                    env['%sFLAGS' % var] = ' '.join(tool_flags)
                found = True
                break

        if found:
            break

    if not found:
        env.Die("can't detect %s: looked for any of: %s" % (
            var,
            ', '.join([' '.join(c) if isinstance(c, list) else c for c in commands])))

    if version:
        actual_ver = env.ParseCompilerVersion(env[var])
        if actual_ver:
            actual_ver = actual_ver[:len(version)]

        if actual_ver != version:
            env.Die(
                "can't detect %s: '%s' not found in PATH, '%s' version is %s" % (
                    var,
                    '%s-%s' % (tool, '.'.join(map(str, version))),
                    env[var],
                    '.'.join(map(str, actual_ver))))

    message = env[var]
    realpath = os.path.realpath(env[var])
    if realpath != env[var]:
        message += ' (%s)' % realpath

    context.Result(message)
    return True

def FindLLVMDir(context, version):
    context.Message(
        "Searching PATH for llvm %s... " % '.'.join(map(str, version)))

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
            ret.append('/usr/lib/llvm%s/bin' % s)
        return ret

    for llvmdir in macos_dirs() + linux_dirs():
        if os.path.isdir(llvmdir):
            context.env['ENV']['PATH'] += ':' + llvmdir
            context.Result(llvmdir)
            return True

    context.Result('not found')
    return True

def _libdirs(host):
    dirs = ['lib/' + host]
    if 'x86_64-pc-linux-gnu' == host:
        dirs += ['lib/x86_64-linux-gnu']
    if 'x86_64' in host:
        dirs += ['lib64']
    dirs += ['lib']
    return dirs

def _isprefix(prefix, subdir):
    prefix = os.path.abspath(prefix)
    subdir = os.path.abspath(subdir)
    return subdir.startswith(prefix + os.sep)

def FindLibDir(context, prefix, host):
    context.Message("Searching for system library directory... ")

    for d in _libdirs(host):
        libdir = os.path.join(prefix, d)
        if os.path.isdir(libdir):
            break

    context.env['ROC_SYSTEM_LIBDIR'] = libdir
    context.Result(libdir)
    return True

def FindPulseDir(context, prefix, build, host, version):
    context.Message("Searching for PulseAudio modules directory... ")

    if build == host:
        pa_ver = context.env.CommandOutput('pulseaudio --version')
        m = re.search(r'([0-9.]+)', pa_ver or '')
        if m and m.group(1) == version:
            pa_conf = context.env.CommandOutput('pulseaudio --dump-conf')
            if pa_conf:
                for line in pa_conf.splitlines():
                    m = re.match(r'^\s*dl-search-path\s*=\s*(.*)$', line)
                    if m:
                        pa_dir = m.group(1)
                        if _isprefix(prefix, pa_dir):
                            context.env['ROC_PULSE_MODULEDIR'] = pa_dir
                            context.Result(pa_dir)
                            return True

    for d in _libdirs(host):
        pa_dir = os.path.join(prefix, d, 'pulse-'+version, 'modules')
        if os.path.isdir(pa_dir):
            context.env['ROC_PULSE_MODULEDIR'] = pa_dir
            context.Result(pa_dir)
            return True

    for d in _libdirs(host):
        libdir = os.path.join(prefix, d)
        if os.path.isdir(libdir):
            break

    pa_dir = os.path.join(libdir, 'pulse-'+version, 'modules')

    context.env['ROC_PULSE_MODULEDIR'] = pa_dir
    context.Result(pa_dir)
    return True

def FindConfigGuess(context):
    context.Message('Searching CONFIG_GUESS script... ')

    if context.env.HasArg('CONFIG_GUESS'):
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

    context.Message('Searching PKG_CONFIG... ')

    if env.HasArg('PKG_CONFIG'):
        context.Result(env['PKG_CONFIG'])
        return True

    # https://autotools.io/pkgconfig/cross-compiling.html
    # http://tiny.cc/lh6upz
    if toolchain:
        if env.Which(toolchain + '-pkg-config'):
            env['PKG_CONFIG'] = toolchain + '-pkg-config'
            context.Result(env['PKG_CONFIG'])
            return True

        if 'PKG_CONFIG_PATH' in os.environ \
            or 'PKG_CONFIG_LIBDIR' in os.environ \
            or 'PKG_CONFIG_SYSROOT_DIR' in os.environ:
            if env.Which('pkg-config'):
                env['PKG_CONFIG'] = 'pkg-config'
                context.Result(env['PKG_CONFIG'])
                return True

        context.Result('not found')
        return False

    if env.Which('pkg-config'):
        env['PKG_CONFIG'] = 'pkg-config'
        context.Result(env['PKG_CONFIG'])
        return True

    context.Result('not found')
    return False

def FindPkgConfigPath(context):
    env = context.env

    context.Message("Searching PKG_CONFIG_PATH...")

    if env.HasArg('PKG_CONFIG_PATH'):
        context.Result(env['PKG_CONFIG_PATH'])
        return True

    # https://linux.die.net/man/1/pkg-config the default is libdir/pkgconfig
    env['PKG_CONFIG_PATH'] = os.path.join(env['ROC_SYSTEM_LIBDIR'], 'pkgconfig')

    pkg_config = env.get('PKG_CONFIG', None)
    if pkg_config:
        pkg_config_paths = env.CommandOutput(
            '%s --variable pc_path pkg-config' % quote(pkg_config))
        try:
            for path in pkg_config_paths.split(':'):
                if os.path.isdir(path):
                    env['PKG_CONFIG_PATH'] = path
                    break
        except:
            pass

    context.Result(env['PKG_CONFIG_PATH'])
    return True

def AddPkgConfigDependency(context, package, flags):
    env = context.env
    if 'PKG_CONFIG_DEPS' not in env.Dictionary():
        env['PKG_CONFIG_DEPS'] = []

    pkg_config = env.get('PKG_CONFIG', None)
    if not pkg_config:
        return False

    cmd = '%s %s %s' % (quote(pkg_config), package, flags)
    try:
        if env.ParseConfig(cmd):
            env.AppendUnique(PKG_CONFIG_DEPS=[package])
            return True
    except:
        pass

    return False

def init(env):
    env.CustomTests = {
        'CheckLibWithHeaderExt': CheckLibWithHeaderExt,
        'CheckProg': CheckProg,
        'CheckCanRunProgs': CheckCanRunProgs,
        'FindTool': FindTool,
        'FindLLVMDir': FindLLVMDir,
        'FindLibDir': FindLibDir,
        'FindPulseDir': FindPulseDir,
        'FindConfigGuess': FindConfigGuess,
        'FindPkgConfig': FindPkgConfig,
        'FindPkgConfigPath': FindPkgConfigPath,
        'AddPkgConfigDependency': AddPkgConfigDependency,
    }
