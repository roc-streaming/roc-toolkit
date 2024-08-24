import SCons
import os
import os.path
import platform
import re
import sys

# supported platform names
supported_platforms = [
    'linux',
    'unix',
    'darwin',
    'android',
]

# supported compiler names (without version)
supported_compilers = [
    'clang',
    'gcc',
    'cc',
]

# supported sanitizers
supported_sanitizers = [
    'undefined',
    'address',
]

# supported macOS target architectures
supported_macos_archs = [
    'all', # for universal binaries, same as manually listing all archs
    'x86_64',
    'arm64',
]

# default installation prefix
if platform.system() == 'Darwin':
    default_prefix = '/usr/local'
else:
    default_prefix = '/usr'

AddOption('--prefix',
          dest='prefix',
          action='store',
          type='string',
          default=default_prefix,
          help="installation prefix, '{}' by default".format(default_prefix))

AddOption('--bindir',
          dest='bindir',
          action='store',
          type='string',
          default=os.path.join(GetOption('prefix'), 'bin'),
          help=("path to the binary installation directory"
                " (where to install Roc command-line tools),"
                " '<prefix>/bin' by default"))

AddOption('--libdir',
          dest='libdir',
          action='store',
          type='string',
          help=("path to the library installation directory"
                " (where to install Roc library),"
                " auto-detected by default"))

AddOption('--incdir',
          dest='incdir',
          action='store',
          type='string',
          default=os.path.join(GetOption('prefix'), 'include'),
          help=("path to the headers installation directory"
                " (where to install Roc headers),"
                " '<prefix>/include' by default"))

AddOption('--mandir',
          dest='mandir',
          action='store',
          type='string',
          default=os.path.join(GetOption('prefix'), 'share/man/man1'),
          help=("path to the manuals installation directory"
                " (where to install Roc manual pages),"
                " '<prefix>/share/man/man1' by default"))

AddOption('--build',
          dest='build',
          action='store',
          type='string',
          help=("system name where Roc is being compiled,"
                " e.g. 'x86_64-pc-linux-gnu',"
                " auto-detected by default"))

AddOption('--host',
          dest='host',
          action='store',
          type='string',
          help=("system name where Roc will run,"
                " e.g. 'arm-linux-gnueabihf',"
                " auto-detected by default"))

AddOption('--platform',
          dest='platform',
          action='store',
          choices=([''] + supported_platforms),
          help=("platform name where Roc will run,"
                " supported values: empty (detect from host), {}".format(
                ', '.join(["'{}'".format(s) for s in supported_platforms]))))

AddOption('--compiler',
          dest='compiler',
          action='store',
          type='string',
          help=("compiler name and optional version, e.g. 'gcc-4.9',"
                " supported names: empty (detect what available), {}".format(
                ', '.join(["'{}'".format(s) for s in supported_compilers]))))

AddOption('--compiler-launcher',
          dest='compiler_launcher',
          action='store',
          type='string',
          help=("optional launching tool for c and c++ compilers,"
              " e.g. 'ccache'"))

AddOption('--sanitizers',
          dest='sanitizers',
          action='store',
          type='string',
          help=("list of gcc/clang sanitizers,"
                " supported names: empty (no sanitizers), 'all', " +
                ', '.join(["'{}'".format(s) for s in supported_sanitizers])))

AddOption('--enable-debug',
          dest='enable_debug',
          action='store_true',
          help='enable debug build for Roc')

AddOption('--enable-debug-3rdparty',
          dest='enable_debug_3rdparty',
          action='store_true',
          help='enable debug build for 3rdparty libraries')

AddOption('--enable-werror',
          dest='enable_werror',
          action='store_true',
          help='treat warnings as errors')

AddOption('--enable-static',
          dest='enable_static',
          action='store_true',
          help='enable building static library')

AddOption('--disable-shared',
          dest='disable_shared',
          action='store_true',
          help='disable building shared library')

AddOption('--disable-tools',
          dest='disable_tools',
          action='store_true',
          help='disable tools building')

AddOption('--enable-tests',
          dest='enable_tests',
          action='store_true',
          help='enable tests building and running (requires CppUTest)')

AddOption('--enable-benchmarks',
          dest='enable_benchmarks',
          action='store_true',
          help='enable bechmarks building and running (requires Google Benchmark)')

AddOption('--enable-examples',
          dest='enable_examples',
          action='store_true',
          help='enable examples building')

AddOption('--enable-doxygen',
          dest='enable_doxygen',
          action='store_true',
          help='enable Doxygen documentation generation')

AddOption('--enable-sphinx',
          dest='enable_sphinx',
          action='store_true',
          help='enable Sphinx documentation generation')

AddOption('--disable-c11',
          dest='disable_c11',
          action='store_true',
          help='disable C11 support')

AddOption('--disable-soversion',
          dest='disable_soversion',
          action='store_true',
          help=("don't write version into the shared library"
                " and don't create version symlinks"))

AddOption('--disable-openfec',
          dest='disable_openfec',
          action='store_true',
          help='disable OpenFEC support required for FEC codes')

AddOption('--disable-speexdsp',
          dest='disable_speexdsp',
          action='store_true',
          help='disable SpeexDSP support for resampling')

AddOption('--disable-sox',
          dest='disable_sox',
          action='store_true',
          help='disable SoX support in tools')

AddOption('--disable-sndfile',
          dest='disable_sndfile',
          action='store_true',
          help='disable sndfile support in tools')

AddOption('--disable-openssl',
          dest='disable_openssl',
          action='store_true',
          help='disable OpenSSL support required for DTLS and SRTP')

AddOption('--disable-libunwind',
          dest='disable_libunwind',
          action='store_true',
          help='disable libunwind support required for printing backtrace')

AddOption('--disable-alsa',
          dest='disable_alsa',
          action='store_true',
          help='disable ALSA support in tools')

AddOption('--disable-pulseaudio',
          dest='disable_pulseaudio',
          action='store_true',
          help='disable PulseAudio support in tools')

AddOption('--with-openfec-includes',
          dest='with_openfec_includes',
          action='store',
          type='string',
          help=("path to the directory with OpenFEC headers (it should contain"
                " lib_common and lib_stable subdirectories)"))

AddOption('--with-includes',
          dest='with_includes',
          action='append',
          type='string',
          help=("additional include search path, may be used multiple times"))

AddOption('--with-libraries',
          dest='with_libraries',
          action='append',
          type='string',
          help=("additional library search path, may be used multiple times"))

AddOption('--macos-platform',
          dest='macos_platform',
          action='store',
          type='string',
          help=("macOS target platform, e.g. 10.12,"
                " (default is current OS version)"))

AddOption('--macos-arch',
          dest='macos_arch',
          action='store',
          type='string',
          help=("macOS target architecture(s),"
                " comma-separated list, supported values: {}".format(
                    ', '.join(["'{}'".format(s) for s in supported_macos_archs])) +
                " (default is current OS arch, pass multiple values"
                " or 'all' for univeral binaries)"))

AddOption('--build-3rdparty',
          dest='build_3rdparty',
          action='store',
          type='string',
          help=("download and build specified 3rdparty libraries,"
                " comma-separated list of library names and optional"
                " versions, e.g. 'libuv:1.4.2,openfec'"))

AddOption('--override-targets',
          dest='override_targets',
          action='store',
          type='string',
          help=("override targets to use,"
                " pass a comma-separated list of target names,"
                " e.g. 'pc,posix,posix_ext,gnu,libuv,openfec,...'"))

# configure even in dry run mode
SCons.SConf.dryrun = 0

# when we cross-compile on macOS to Android using clang, we should use
# GNU-like clang options, but SCons incorrectly sets up Apple-like
# clang options; here we prevent this behavior by forcing 'posix' platform
scons_platform = Environment(ENV=os.environ)['PLATFORM']
for opt in ['host', 'platform']:
    if 'android' in (GetOption(opt) or ''):
        scons_platform = 'posix'

env = Environment(
    ENV=os.environ,
    platform=scons_platform,
    toolpath=[os.path.join(Dir('#').abspath, 'scripts')],
    tools=[
        'default',
        'scons_plugin', # our plugin in scripts/
        ])

# performance tuning
env.Decider('MD5-timestamp')
env.SetOption('implicit_cache', 1)

# provide absolute path to force single sconsign file
# per-directory sconsign files seems to be buggy with generated sources
# create separate sconsign file for each python version, since different
# python versions can use different pickle protocols and switching from
# a higher version to a lower one would cause exception
env.SConsignFile(os.path.join(
    env.Dir('#').abspath, '.sconsign{}{}.dblite'.format(*sys.version_info[0:2])))

# we always use -fPIC, so object files built for static and shared
# libraries are no different
env['STATIC_AND_SHARED_OBJECTS_ARE_THE_SAME'] = 1

# fill vars from environment (CC=gcc scons ...) and arguments (scons CC=gcc ...)
for var in ['CXX', 'CC', 'LD', 'AR', 'RANLIB', 'LIPO', 'INSTALL_NAME_TOOL',
            'RAGEL', 'GENGETOPT',
            'PKG_CONFIG', 'PKG_CONFIG_PATH', 'CONFIG_GUESS',
            'CLANG_FORMAT']:
    env.OverrideFromArgument(var)

env.OverrideFromArgument('CXXLD', names=['CXXLD', 'CXX'])
env.OverrideFromArgument('CCLD', names=['CCLD', 'LD', 'CC'])

env.OverrideFromArgument('STRIP', default='strip')
env.OverrideFromArgument('OBJCOPY', default='objcopy')

env.OverrideFromArgument('DOXYGEN', default='doxygen')
env.OverrideFromArgument('SPHINX_BUILD', default='sphinx-build')
env.OverrideFromArgument('BREATHE_APIDOC', default='breathe-apidoc')

env.OverrideFromArgument('DESTDIR')

env.PrependFromArgument('CPPFLAGS')
env.PrependFromArgument('CXXFLAGS')
env.PrependFromArgument('CFLAGS')
env.PrependFromArgument('LINKFLAGS', names=['LINKFLAGS', 'LDFLAGS'])
env.PrependFromArgument('STRIPFLAGS')

# ensure that these vars always exist
env.Append(CXXFLAGS=[])
env.Append(CPPDEFINES=[])
env.Append(CPPPATH=[])
env.Append(LIBPATH=[])
env.Append(LIBS=[])
env.Append(STRIPFLAGS=[])

# handle --with-*
if GetOption('with_includes'):
    env.Append(CPPPATH=GetOption('with_includes'))

if GetOption('with_libraries'):
    env.Append(LIBPATH=GetOption('with_libraries'))

# handle --help
if GetOption('help'):
    Return()

# clean* targets
clean_build = []
for p in ['bin', 'build/src']:
    for d in env.GlobDirs(p):
        clean_build += [env.DeleteDir(d)]

clean_doc = []
for p in ['build/docs', 'docs/html']:
    for d in env.GlobDirs(p):
        clean_doc += [env.DeleteDir(d)]

clean_all = []
for p in ['bin',
          'build',
          'dist',
          'docs/html',
          'compile_commands.json',
          'config.log',
          '.sconsign*.dblite',
          '.sconf_temp',
          'debian/.debhelper',
          'debian/tmp',
          'debian/libroc',
          'debian/libroc-dev',
          'debian/roc',
          'debian/*.substvars',
          'debian/*.debhelper.log',
          'debian/debhelper-*',
          'debian/files',
          ]:
    for d in env.GlobDirs(p):
        clean_all += [env.DeleteDir(d)]
    for f in env.GlobFiles(p):
        clean_all += [env.DeleteFile(f)]

env.AlwaysBuild(env.Alias('clean', [], clean_all))
env.AlwaysBuild(env.Alias('cleanbuild', [], clean_build))
env.AlwaysBuild(env.Alias('cleandocs', [], clean_doc))

if set(COMMAND_LINE_TARGETS).intersection(['clean', 'cleanbuild', 'cleandocs']) or \
  env.GetOption('clean'):
    if set(COMMAND_LINE_TARGETS) - set(['clean', 'cleanbuild', 'cleandocs']):
        env.Die("combining 'clean*' targets with other targets is not allowed")
    if env.GetOption('clean'):
        if clean_all:
            env.Execute(clean_all)
        else:
            print("scons: Nothing to be done for `clean'.")
    Return()

# fmt target
if 'fmt' in COMMAND_LINE_TARGETS:
    conf = Configure(env, custom_tests=env.CustomTests)
    conf.FindClangFormat()
    env = conf.Finish()

    fmt_actions = []

    fmt_actions.append(env.ClangFormat('#src'))

    fmt_actions.append(env.HeaderFormat('#src/internal_modules'))
    fmt_actions.append(env.HeaderFormat('#src/public_api/src'))
    fmt_actions.append(env.HeaderFormat('#src/tests'))
    fmt_actions.append(env.HeaderFormat('#src/tools'))

    env.AlwaysBuild(
        env.Alias('fmt', [], fmt_actions))

# build documentation
doc_env = env.DeepClone()
doc_env.SConscript('docs/SConscript',
                   duplicate=0, exports='doc_env')

# run scons self-test
env.AlwaysBuild(env.Alias('selftest', [], [
    env.SelfTest(),
]))

# exit early if there is nothing to build
non_build_targets = ['fmt', 'docs', 'sphinx', 'doxygen', 'selftest']
if set(COMMAND_LINE_TARGETS) \
  and set(COMMAND_LINE_TARGETS).intersection(non_build_targets) == set(COMMAND_LINE_TARGETS):
    Return()

# meta-information about the build, used to generate env parameters
meta = type('meta', (), {
    field: '' for field in ('build host toolchain platform variant thirdparty_variant '
                            'compiler compiler_ver '
                            'c11_support gnu_toolchain').split()})

# toolchain triple of the local system (where we're building), e.g. x86_64-pc-linux-gnu
meta.build = GetOption('build')

# toolchain triple of the target system (where we will run), e.g. aarch64-linux-gnu
meta.host = GetOption('host') or ''

# toolchain prefix for compiler, linker, and other tools, e.g. aarch64-linux-gnu
meta.toolchain = GetOption('host') or ''

# name of the target platform, e.g. 'linux'; see supported_platforms
meta.platform = GetOption('platform') or ''

# build variant, i.e. 'debug' or 'release'
meta.variant = 'debug' if GetOption('enable_debug') else 'release'

# build variant for third-parties
meta.thirdparty_variant = 'debug' if GetOption('enable_debug_3rdparty') else 'release'

# compiler name, e.g. 'gcc', and version tuple, e.g. (4, 9)
if GetOption('compiler'):
    meta.compiler = GetOption('compiler')
    if '-' in meta.compiler:
        meta.compiler, meta.compiler_ver = meta.compiler.split('-')
        meta.compiler_ver = tuple(map(int, meta.compiler_ver.split('.')))
else:
    if env.HasArgument('CXX'):
        meta.compiler = env.ParseCompilerType(env['CXX'])
    elif meta.toolchain:
        if env.Which('{}-clang++'.format(meta.toolchain)):
            meta.compiler = 'clang'
        elif env.Which('{}-g++'.format(meta.toolchain)):
            meta.compiler = 'gcc'
        elif env.Which('{}-c++'.format(meta.toolchain)):
            meta.compiler = 'cc'
    else:
        if env.Which('clang++'):
            meta.compiler = 'clang'
        elif env.Which('g++'):
            meta.compiler = 'gcc'
        elif env.Which('c++'):
            meta.compiler = 'cc'

if not meta.compiler:
    env.Die("can't detect compiler name, please specify '--compiler=<name>' manually, "
            "should be one of: {}", ', '.join(supported_compilers))

if not meta.compiler in supported_compilers:
    env.Die("unknown --compiler '{}', expected one of: {}",
            meta.compiler, ', '.join(supported_compilers))

if not meta.compiler_ver:
    if meta.toolchain:
        meta.compiler_ver = env.ParseCompilerVersion('{}-{}'.format(meta.toolchain, meta.compiler))
    else:
        meta.compiler_ver = env.ParseCompilerVersion(meta.compiler)

if not meta.compiler_ver:
    if meta.compiler not in ['cc']:
        env.Die("can't detect compiler version for compiler '{}'",
                '-'.join([s for s in [meta.toolchain, meta.compiler] if s]))

conf = Configure(env, custom_tests=env.CustomTests)

if meta.compiler == 'clang':
    conf.FindLLVMDir(meta.compiler_ver)

if meta.compiler == 'clang':
    conf.FindTool('CXX', [meta.toolchain], [('clang++', meta.compiler_ver)])
elif meta.compiler == 'gcc':
    conf.FindTool('CXX', [meta.toolchain], [('g++', meta.compiler_ver)])
elif meta.compiler == 'cc':
    conf.FindTool('CXX', [meta.toolchain], [('c++', meta.compiler_ver)])

full_compiler_ver = env.ParseCompilerVersion(conf.env['CXX'])
if full_compiler_ver:
    meta.compiler_ver = full_compiler_ver

if not meta.build:
    for local_compiler in ['/usr/bin/gcc', '/usr/bin/clang']:
        meta.build = env.ParseCompilerTarget(local_compiler)
        if meta.build:
            break

if not meta.build and not meta.host:
    if conf.CheckCanRunProgs():
        meta.build = env.ParseCompilerTarget(conf.env['CXX'])

if not meta.build:
    if conf.FindConfigGuess():
        meta.build = env.ParseConfigGuess(conf.env['CONFIG_GUESS'])

if not meta.build:
    env.Die(("can't detect system type, please specify '--build=<type>' manually, "
             "e.g. '--build=x86_64-pc-linux-gnu'"))

if not meta.host:
    meta.host = env.ParseCompilerTarget(conf.env['CXX'])

if not meta.host:
    meta.host = meta.build

if not meta.toolchain and meta.build != meta.host:
    meta.toolchain = meta.host

if not meta.toolchain and meta.build == meta.host:
    meta.build = meta.host = env.ParseMacosHost(
        meta.host, GetOption('macos_platform'), GetOption('macos_arch'))

if not meta.platform:
    if 'android' in meta.host:
        meta.platform = 'android'
    elif 'linux' in meta.host:
        meta.platform = 'linux'
    elif 'darwin' in meta.host:
        meta.platform = 'darwin'
    elif 'gnu' in meta.host:
        meta.platform = 'unix'

if not meta.platform and meta.host == meta.build:
    if os.name == 'posix':
        meta.platform = 'unix'

if not meta.platform:
    env.Die("can't detect platform name, please specify '--platform=<name>' manually, "
            "should be one of: {}", ', '.join(supported_platforms))

if meta.platform not in supported_platforms:
    env.Die(("unknown --platform '{}', expected on of: {}"),
                meta.platform, ', '.join(supported_platforms))

allowed_toolchains = [meta.toolchain]
if meta.toolchain != '' and meta.build == meta.host:
    allowed_toolchains += ['']

if meta.compiler == 'clang':
    conf.FindTool('CXX', allowed_toolchains, [('clang++', meta.compiler_ver)])
    conf.FindTool('CC', allowed_toolchains, [('clang', meta.compiler_ver)])
    conf.FindTool('CXXLD', allowed_toolchains, [('clang++', meta.compiler_ver)])
    conf.FindTool('CCLD', allowed_toolchains, [('clang', meta.compiler_ver)])
    conf.FindTool('LD', allowed_toolchains, [('ld', None)], required=False)

    compiler_dir = env.ParseCompilerDirectory(conf.env['CXX'])
    if compiler_dir:
        prepend_path = [compiler_dir]
    else:
        prepend_path = []

    conf.FindTool('AR', allowed_toolchains,
                  [('llvm-ar', meta.compiler_ver), ('llvm-ar', None), ('ar', None)],
                  compiler_dir=compiler_dir,
                  prepend_path=prepend_path)

    conf.FindTool('RANLIB', allowed_toolchains,
                  [('llvm-ranlib', meta.compiler_ver), ('llvm-ranlib', None), ('ranlib', None)],
                  compiler_dir=compiler_dir,
                  prepend_path=prepend_path)

    conf.FindTool('STRIP', allowed_toolchains,
                  [('llvm-strip', meta.compiler_ver), ('llvm-strip', None), ('strip', None)],
                  compiler_dir=compiler_dir,
                  prepend_path=prepend_path)

    conf.FindTool('OBJCOPY', allowed_toolchains,
                  [('llvm-objcopy', meta.compiler_ver),
                   ('llvm-objcopy', None),
                   ('objcopy', None)],
                  compiler_dir=compiler_dir,
                  prepend_path=prepend_path,
                  required=False)

elif meta.compiler == 'gcc':
    conf.FindTool('CXX', allowed_toolchains, [('g++', meta.compiler_ver)])
    conf.FindTool('CC', allowed_toolchains, [('gcc', meta.compiler_ver)])
    conf.FindTool('CXXLD', allowed_toolchains, [('g++', meta.compiler_ver)])
    conf.FindTool('CCLD', allowed_toolchains, [('gcc', meta.compiler_ver)])
    conf.FindTool('LD', allowed_toolchains, [('ld', None)], required=False)
    conf.FindTool('AR', allowed_toolchains, [('ar', None)])
    conf.FindTool('RANLIB', allowed_toolchains, [('ranlib', None)])
    conf.FindTool('STRIP', allowed_toolchains, [('strip', None)])
    conf.FindTool('OBJCOPY', allowed_toolchains, [('objcopy', None)], required=False)

elif meta.compiler == 'cc':
    conf.FindTool('CXX', allowed_toolchains, [('c++', meta.compiler_ver)])
    conf.FindTool('CC', allowed_toolchains, [('cc', meta.compiler_ver)])
    conf.FindTool('CXXLD', allowed_toolchains, [('c++', meta.compiler_ver)])
    conf.FindTool('CCLD', allowed_toolchains, [('cc', meta.compiler_ver)])
    conf.FindTool('AR', allowed_toolchains, [('ar', None)])
    conf.FindTool('RANLIB', allowed_toolchains, [('ranlib', None)])
    conf.FindTool('STRIP', allowed_toolchains, [('strip', None)])

conf.env['LINK'] = env['CXXLD']
conf.env['SHLINK'] = env['CXXLD']

if GetOption('compiler_launcher'):
    conf.FindProgram('COMPILER_LAUNCHER', GetOption('compiler_launcher'))

if meta.platform == 'darwin':
    conf.FindTool('LIPO', [''], [('lipo', None)], required=False)
    conf.FindTool('INSTALL_NAME_TOOL', [''], [('install_name_tool', None)], required=False)

meta.c11_support = False
if not GetOption('disable_c11'):
    if meta.compiler == 'gcc':
        meta.c11_support = meta.compiler_ver[:2] >= (4, 9)
    elif meta.compiler == 'clang' and meta.platform == 'darwin':
        meta.c11_support = meta.compiler_ver[:2] >= (7, 0)
    elif meta.compiler == 'clang' and meta.platform != 'darwin':
        meta.c11_support = meta.compiler_ver[:2] >= (3, 6)

# true if we have full-featured GNU toolchain with all needed compiler and linker options
# note that macOS is excluded
meta.gnu_toolchain = False
if meta.platform in ['linux', 'unix']:
    meta.gnu_toolchain = 'gnu' in meta.host
elif meta.platform in ['android']:
    meta.gnu_toolchain = True

conf.env['ROC_SYSTEM_BINDIR'] = GetOption('bindir')
conf.env['ROC_SYSTEM_INCDIR'] = GetOption('incdir')

if GetOption('libdir'):
    conf.env['ROC_SYSTEM_LIBDIR'] = GetOption('libdir')
else:
    conf.FindLibDir(GetOption('prefix'), meta.host)

conf.env['ROC_SYSTEM_PCDIR'] = os.path.join(env['ROC_SYSTEM_LIBDIR'], 'pkgconfig')

conf.FindPkgConfig(meta.toolchain)
conf.FindPkgConfigPath(GetOption('prefix'))

env = conf.Finish()

# build directories
env['ROC_BINDIR'] = '#bin/{}'.format(meta.host)

env['ROC_BUILDDIR'] = '#build/src/{}/{}'.format(
    meta.host,
    '-'.join(
        [s for s in [
            meta.compiler,
            '.'.join(map(str, meta.compiler_ver)) if meta.compiler_ver else '',
            meta.variant
        ] if s])
    )

env['ROC_THIRDPARTY_BUILDDIR'] = '#build/3rdparty/{}/{}'.format(
    meta.host,
    '-'.join(
        [s for s in [
            meta.compiler,
            '.'.join(map(str, meta.compiler_ver)) if meta.compiler_ver else '',
            meta.thirdparty_variant
        ] if s])
    )

# version info
env['ROC_VERSION'] = env.ParseProjectVersion('src/public_api/include/roc/version.h')
env['ROC_COMMIT'] = env.ParseGitHead()

env['ROC_SOVER'] = '.'.join(env['ROC_VERSION'].split('.')[:2])

# internal modules
env['ROC_MODULES'] = [
    'roc_core',
    'roc_status',
    'roc_stat',
    'roc_address',
    'roc_packet',
    'roc_fec',
    'roc_dbgio',
    'roc_audio',
    'roc_rtp',
    'roc_rtcp',
    'roc_sdp',
    'roc_netio',
    'roc_sndio',
    'roc_pipeline',
    'roc_ctl',
    'roc_node',
]

# build variant for roc and dependencies (e.g. 'debug' or 'release')
env['ROC_VARIANT'] = meta.variant
env['ROC_THIRDPARTY_VARIANT'] = meta.thirdparty_variant

# toolchain tuples (e.g. 'aarch64-linux-gnu')
env['ROC_BUILD'] = meta.build
env['ROC_HOST'] = meta.host
env['ROC_TOOLCHAIN'] = meta.toolchain

# platform identifier (e.g. 'linux', 'android')
env['ROC_PLATFORM'] = meta.platform

# platform version that we build against
env['ROC_POSIX_PLATFORM']   = '200809'
env['ROC_ANDROID_PLATFORM'] = '21'

# macOS target platform version and architecture(s)
env['ROC_MACOS_PLATFORM'] = env.ParseMacosPlatform(meta.host, GetOption('macos_platform'))
env['ROC_MACOS_ARCH'] = env.ParseMacosArch(meta.host, GetOption('macos_arch'))

# enabled target directories
env['ROC_TARGETS'] = []

if GetOption('override_targets'):
    for t in GetOption('override_targets').split(','):
        env['ROC_TARGETS'] += ['target_' + t]
else:
    if meta.platform in ['linux', 'darwin', 'unix']:
        env.Append(ROC_TARGETS=[
            'target_pc',
        ])

    if meta.platform in ['linux', 'android', 'darwin', 'unix']:
        env.Append(ROC_TARGETS=[
            'target_posix',
        ])

    if meta.platform in ['linux', 'darwin', 'unix']:
        env.Append(ROC_TARGETS=[
            'target_posix_pc',
        ])

    if meta.platform in ['linux', 'android', 'unix']:
        env.Append(ROC_TARGETS=[
            'target_posix_ext',
        ])

    if meta.platform in ['linux', 'android' 'darwin'] or meta.gnu_toolchain:
        env.Append(ROC_TARGETS=[
            # GNU C++ Standard Library (libstdc++), or compatible, like
            # LLVM C++ Standard Library (libc++)
            'target_gnu',
        ])

    if meta.platform in ['darwin']:
        env.Append(ROC_TARGETS=[
            'target_darwin',
        ])

    if meta.platform in ['android']:
        env.Append(ROC_TARGETS=[
            'target_android',
        ])

    if meta.c11_support:
        env.Append(ROC_TARGETS=[
            'target_c11',
        ])
    else:
        env.Append(ROC_TARGETS=[
            'target_libatomic_ops',
        ])

    if meta.platform in ['linux', 'darwin', 'unix'] and not GetOption('disable_libunwind'):
        env.Append(ROC_TARGETS=[
            'target_libunwind',
        ])

    env.Append(ROC_TARGETS=[
        'target_libuv',
    ])

    if not GetOption('disable_openfec'):
        env.Append(ROC_TARGETS=[
            'target_openfec',
        ])

    if not GetOption('disable_openssl'):
        env.Append(ROC_TARGETS=[
            'target_openssl',
        ])
    else:
        env.Append(ROC_TARGETS=[
            'target_nocsprng',
        ])

    if not GetOption('disable_speexdsp'):
        env.Append(ROC_TARGETS=[
            'target_speexdsp',
        ])

    if not GetOption('disable_tools'):
        if not GetOption('disable_sox'):
            env.Append(ROC_TARGETS=[
                'target_sox',
            ])
        if not GetOption('disable_sndfile'):
            env.Append(ROC_TARGETS=[
                'target_sndfile',
            ])
        if not GetOption('disable_alsa') and meta.platform in ['linux']:
            env.Append(ROC_TARGETS=[
                'target_alsa',
            ])
        if not GetOption('disable_pulseaudio') and meta.platform in ['linux']:
            env.Append(ROC_TARGETS=[
                'target_pulseaudio',
            ])

    if 'target_gnu' not in env['ROC_TARGETS']:
        env.Append(ROC_TARGETS=[
            'target_nodemangle',
        ])

    if 'target_libunwind' not in env['ROC_TARGETS'] and \
      'target_android' not in env['ROC_TARGETS']:
        env.Append(ROC_TARGETS=[
            'target_nobacktrace',
        ])

# env will hold settings common to all code
# subenvs will hold settings specific to particular parts of code
subenv_names = 'internal_modules public_libs examples tools tests generated_code'.split()

subenv_attrs = {field: env.DeepClone() for field in subenv_names}
subenv_attrs['all'] = list(subenv_attrs.values())

subenvs = type('subenvs', (), subenv_attrs)

# find or build third-party dependencies
env, subenvs = env.SConscript('3rdparty/SConscript',
                       duplicate=0, exports='env subenvs meta')

env.Append(CPPDEFINES=[
    # for UINT32_MAX and others (https://bugzilla.mozilla.org/show_bug.cgi?id=673556):
    ('__STDC_LIMIT_MACROS', '1'),
])

if 'target_posix' in env['ROC_TARGETS'] and meta.platform not in ['darwin']:
    # macOS is special, otherwise rely on _POSIX_C_SOURCE
    env.Append(CPPDEFINES=[('_POSIX_C_SOURCE', env['ROC_POSIX_PLATFORM'])])

if meta.platform in ['darwin']:
    if env['ROC_MACOS_PLATFORM']:
        for var in ['CXXFLAGS', 'CFLAGS', 'LINKFLAGS']:
            env.Append(**{var: [
                '-mmacosx-version-min=' + env['ROC_MACOS_PLATFORM'],
            ]})
    for arch in env['ROC_MACOS_ARCH']:
        for var in ['CXXFLAGS', 'CFLAGS', 'LINKFLAGS']:
            env.Append(**{var: [
                '-arch', arch,
            ]})

if meta.platform in ['linux', 'unix']:
    env.AddManualDependency(libs=['rt', 'dl', 'm'])

if meta.platform in ['android']:
    env.AddManualDependency(libs=['log', 'android'])

if meta.compiler in ['gcc', 'clang']:
    if not meta.platform in ['android']:
        env.Append(CXXFLAGS=[
            '-std=c++98',
        ])
        env.Append(CFLAGS=[
            '-std=c99',
        ])

    env.Append(CXXFLAGS=[
        '-fno-exceptions',
    ])

    for var in ['CXXFLAGS', 'CFLAGS']:
        env.Append(**{var: [
            '-pthread',
            '-fPIC',
        ]})

    if meta.platform in ['linux', 'darwin']:
        env.AddManualDependency(libs=['pthread'])

    if meta.platform in ['linux', 'android'] or meta.gnu_toolchain:
        if not GetOption('disable_soversion'):
            subenvs.public_libs['SHLIBSUFFIX'] = '{}.{}'.format(
                subenvs.public_libs['SHLIBSUFFIX'], env['ROC_SOVER'])

        subenvs.public_libs.Append(LINKFLAGS=[
            '-Wl,-soname,libroc{}'.format(subenvs.public_libs['SHLIBSUFFIX']),
        ])

        if meta.variant == 'release':
            subenvs.public_libs.Append(LINKFLAGS=[
                '-Wl,--version-script={}'.format(env.File('#src/public_api/roc.version').path)
            ])

    if meta.platform in ['darwin']:
        if not GetOption('disable_soversion'):
            subenvs.public_libs['SHLIBSUFFIX'] = '.{}{}'.format(
                env['ROC_SOVER'], subenvs.public_libs['SHLIBSUFFIX'])

            subenvs.public_libs.Append(LINKFLAGS=[
                '-Wl,-compatibility_version,{}'.format(env['ROC_SOVER']),
                '-Wl,-current_version,{}'.format(env['ROC_VERSION']),
            ])

        subenvs.public_libs.Append(LINKFLAGS=[
            '-Wl,-install_name,{}/libroc{}'.format(
                env.Dir(env['ROC_BINDIR']).abspath, subenvs.public_libs['SHLIBSUFFIX']),
        ])

        if meta.variant == 'release':
            subenvs.public_libs.Append(LINKFLAGS=[
                '-Wl,-exported_symbol,_roc_*',
            ])

    if not(meta.compiler == 'clang' and meta.variant == 'debug'):
        env.Append(CXXFLAGS=[
            '-fno-rtti',
        ])

    if meta.variant == 'debug':
        for var in ['CXXFLAGS', 'CFLAGS']:
            env.Append(**{var: [
                '-ggdb',
                '-funwind-tables',
                '-fno-omit-frame-pointer',
            ]})
        env.Append(LINKFLAGS=[
            '-rdynamic'
        ])
    else:
        for var in ['CXXFLAGS', 'CFLAGS']:
            env.Append(**{var: [
                '-fvisibility=hidden',
                '-O3',
            ]})

    if meta.compiler == 'gcc' and meta.compiler_ver[:2] < (4, 6):
        for var in ['CXXFLAGS', 'CFLAGS']:
            env.Append(**{var: [
                '-fno-strict-aliasing',
            ]})

if meta.compiler in ['cc']:
    env.AddManualDependency(libs=['pthread'])

    if meta.variant == 'debug':
        for var in ['CXXFLAGS', 'CFLAGS']:
            env.Append(**{var: [
                '-g',
            ]})
    else:
        for var in ['CXXFLAGS', 'CFLAGS']:
            env.Append(**{var: [
                '-O3',
            ]})

    for var in ['CXXFLAGS', 'CFLAGS']:
        conf.env.Append(**{var: [
            '-fPIC',
        ]})

if meta.compiler in ['gcc', 'clang']:
    if GetOption('enable_werror'):
        for var in ['CXXFLAGS', 'CFLAGS']:
            env.Append(**{var: [
                '-Werror',
            ]})

if meta.compiler == 'gcc':
    for var in ['CXXFLAGS', 'CFLAGS']:
        env.Append(**{var: [
            # generic
            '-Wall',
            '-Wextra',

            # enable
            '-Wcast-qual',
            '-Wformat-security',
            '-Wformat=2',
            '-Wmissing-declarations',
            '-Wpointer-arith',
            '-Wuninitialized',
            '-Wunused',

            # disable
            '-Wno-array-bounds',
            '-Wno-cast-function-type',
            '-Wno-psabi',
            '-Wno-restrict',
            '-Wno-shadow',
            '-Wno-stringop-overflow',
            '-Wno-system-headers',
            '-Wno-unused-const-variable',
            '-Wno-unused-function',
            '-Wno-unused-parameter',
        ]})

    env.Append(CXXFLAGS=[
        # enable
        '-Wctor-dtor-privacy',
        '-Wnon-virtual-dtor',
        '-Wstrict-null-sentinel',

        # disable
        '-Wno-invalid-offsetof',
    ])

    if meta.compiler_ver[:2] >= (10, 0):
        # enable
        for var in ['CXXFLAGS', 'CFLAGS']:
            env.Append(**{var: [
                '-Wdouble-promotion',
                '-Wfloat-conversion',
                '-Wlogical-op',
                '-Woverlength-strings',
                '-Wsign-conversion',
            ]})
    else:
        # disable
        env.Append(CXXFLAGS=[
            '-Wno-reorder',
        ])

if meta.compiler == 'clang':
    for var in ['CXXFLAGS', 'CFLAGS']:
        env.Append(**{var: [
            # generic
            '-Wall',
            '-Wextra',

            # enable
            '-Wcast-qual',
            '-Wdouble-promotion',
            '-Wfloat-conversion',
            '-Wformat-security',
            '-Wformat=2',
            '-Wnull-dereference',
            '-Woverlength-strings',
            '-Woverloaded-virtual',
            '-Wpointer-arith',
            '-Wshadow',
            '-Wsign-conversion',
            '-Wuninitialized',
            '-Wunused',

            # disable
            '-Wno-format-nonliteral',
            '-Wno-psabi',
            '-Wno-shadow',
            '-Wno-system-headers',
            '-Wno-unused-const-variable',
            '-Wno-unused-function',
            '-Wno-unused-parameter',
        ]})

    env.Append(CXXFLAGS=[
        # enable
        '-Wnon-virtual-dtor',

        # disable
        '-Wno-invalid-offsetof',
    ])

    if meta.platform not in ['darwin', 'android'] and meta.compiler_ver[:2] >= (11, 0):
        # enable
        pass
    else:
        # disable
        for var in ['CXXFLAGS', 'CFLAGS']:
            env.Append(**{var: [
                '-Wno-unknown-warning-option',
            ]})
        env.Append(CXXFLAGS=[
            '-Wno-reorder',
        ])

if meta.compiler in ['gcc', 'clang']:
    for var in ['CXXFLAGS', 'CFLAGS']:
        subenvs.generated_code.AppendUnique(**{var: [
            '-w',
        ]})

sanitizers = env.ParseList(GetOption('sanitizers'), supported_sanitizers)
if sanitizers:
    if not meta.compiler in ['gcc', 'clang']:
        env.Die("sanitizers are not supported for compiler '{}'", meta.compiler)

    enabled = sanitizers
    disabled = ['vptr'] # disable due to false positives w/ virtual inheritance

    for name in enabled + disabled:
        if name in enabled:
            flags = ['-fsanitize=' + name,
                '-fno-sanitize-recover=' + name]
        else:
            flags = ['-fno-sanitize=' + name]
        env.AppendUnique(CFLAGS=flags)
        env.AppendUnique(CXXFLAGS=flags)
        env.AppendUnique(LINKFLAGS=flags)
else:
    if meta.platform in ['linux', 'android']:
        env.Append(LINKFLAGS=[
            '-Wl,--no-undefined',
        ])

subenvs.tests.Append(
    CPPDEFINES=('CPPUTEST_USE_MEM_LEAK_DETECTION', '0')
    )

if not env['STRIPFLAGS']:
    env.Append(STRIPFLAGS=['-x'])

env.Append(CPPPATH=[env['ROC_BUILDDIR'] + '/tools'])
env.Append(LIBPATH=[env['ROC_BUILDDIR']])

# both env and subenvs have been modified after subenvs were cloned from env
# here we propagate modifications from env to all subenvs
for senv in subenvs.all:
    senv.MergeFrom(env)

# enable compiler launcher
for senv in subenvs.all:
    if 'COMPILER_LAUNCHER' in senv.Dictionary():
        for var in ['CC', 'CXX']:
            senv[var] = env.WrapLauncher(senv[var], senv['COMPILER_LAUNCHER'])

# enable generation of compile_commands.json (a.k.a. clangdb)
if meta.compiler in ['gcc', 'clang']:
    for senv in subenvs.all:
        for var in ['CC', 'CXX']:
            senv[var] = env.WrapClangDb(senv[var], env['ROC_BUILDDIR'])

    env['ROC_CLANGDB'] = '{}/compile_commands.json'.format(env['ROC_BUILDDIR'])

# post-process paths after merging environments
if meta.compiler in ['gcc', 'clang']:
    for senv in subenvs.all:
        for var in ['CXXFLAGS', 'CFLAGS']:
            dirs = [('-isystem', senv.Dir(path).path) for path in senv['CPPPATH']]

            # workaround to force our 3rdparty directories to be placed
            # before /usr/local/include on macos: explicitly place it
            # after previous -isystem options
            if meta.compiler == 'clang' and meta.platform == 'darwin':
                dirs += [('-isystem', '/usr/local/include')]

            senv.Prepend(**{var: dirs})

        # workaround for "skipping incompatible" linker warning
        if '/usr/lib64' in senv.ParseLinkDirs(senv['CXXLD']):
            senv.Prepend(LINKFLAGS=['-L/usr/lib64'])

# finally build the project
env.SConscript('src/SConscript',
            variant_dir=env['ROC_BUILDDIR'], duplicate=0, exports='env subenvs')
