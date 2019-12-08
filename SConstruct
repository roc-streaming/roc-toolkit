import re
import os
import os.path
import platform
import SCons.Script

# supported platform names
supported_platforms = [
    'linux',
    'darwin',
    'android',
]

# supported compiler names (without version)
supported_compilers = [
    'gcc',
    'clang',
]

# supported sanitizers
supported_sanitizers = [
    'undefined',
    'address',
]

# 3rdparty library default versions
thirdparty_versions = {
    'libuv':      '1.35.0',
    'libunwind':  '1.2.1',
    'openfec':    '1.4.2.4',
    'sox':        '14.4.2',
    'alsa':       '1.0.29',
    'pulseaudio': '5.0',
    'json':       '0.11-20130402',
    'ltdl':       '2.4.6',
    'sndfile':    '1.0.20',
    'ragel':      '6.10',
    'gengetopt':  '2.22.6',
    'cpputest':   '3.6',
}

SCons.SConf.dryrun = 0 # configure even in dry run mode

env = Environment(ENV=os.environ, tools=[
    'default',
    'roc',
])

# performance tuning
env.Decider('MD5-timestamp')
env.SetOption('implicit_cache', 1)
env.SourceCode('.', None)

# provide absolute path to force single sconsign file
# per-directory sconsign files seems to be buggy with generated sources
env.SConsignFile(os.path.join(env.Dir('#').abspath, '.sconsign.dblite'))

# we always use -fPIC, so object files built for static and shared
# libraries are no different
env['STATIC_AND_SHARED_OBJECTS_ARE_THE_SAME'] = 1

if platform.system() == 'Linux':
    # it would be better to use /usr/local on Linux too, but PulseAudio
    # is usually installed in /usr and does no search /usr/local for
    # dynamic libraries; so by default we also use /usr for consistency
    default_prefix = '/usr'
else:
    default_prefix = '/usr/local'

AddOption('--prefix',
          dest='prefix',
          action='store',
          type='string',
          default=default_prefix,
          help="installation prefix, '%s' by default" % default_prefix)

AddOption('--bindir',
          dest='bindir',
          action='store',
          type='string',
          default=os.path.join(GetOption('prefix'), 'bin'),
          help=("path to the binary installation directory (where to "+
                "install Roc command-line tools), '<prefix>/bin' by default"))

AddOption('--libdir',
          dest='libdir',
          action='store',
          type='string',
          help=("path to the library installation directory (where to "+
                "install Roc library), auto-detect if empty"))

AddOption('--incdir',
          dest='incdir',
          action='store',
          type='string',
          default=os.path.join(GetOption('prefix'), 'include'),
          help=("path to the headers installation directory (where to "+
                "install Roc headers), '<prefix>/include' by default"))

AddOption('--mandir',
          dest='mandir',
          action='store',
          type='string',
          default=os.path.join(GetOption('prefix'), 'share/man/man1'),
          help=("path to the manuals installation directory (where to "+
                "install Roc manual pages), '<prefix>/share/man/man1' by default"))

AddOption('--pulseaudio-module-dir',
          dest='pulseaudio_module_dir',
          action='store',
          type='string',
          help=("path to the PulseAudio modules installation directory (where "+
                "to install Roc PulseAudio modules), auto-detect if empty"))

AddOption('--build',
          dest='build',
          action='store',
          type='string',
          help=("system name where Roc is being compiled, "+
                "e.g. 'x86_64-pc-linux-gnu', "+
                "auto-detect if empty"))

AddOption('--host',
          dest='host',
          action='store',
          type='string',
          help=("system name where Roc will run, "+
                "e.g. 'arm-linux-gnueabihf', "+
                "auto-detect if empty"))

AddOption('--platform',
          dest='platform',
          action='store',
          choices=([''] + supported_platforms),
          help=("platform name where Roc will run, "+
            "supported values: empty (detect from host), %s" % (
              ', '.join(["'%s'" % s for s in supported_platforms]))))

AddOption('--compiler',
          dest='compiler',
          action='store',
          type='string',
          help=("compiler name and optional version, e.g. 'gcc-4.9', "+
            "supported names: empty (detect what available), %s" % (
              ', '.join(["'%s'" % s for s in supported_compilers]))))

AddOption('--sanitizers',
          dest='sanitizers',
          action='store',
          type='string',
          help="list of gcc/clang sanitizers, "+
          "supported names: empty (no sanitizers), 'all', "+
          ', '.join(["'%s'" % s for s in supported_sanitizers]))

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

AddOption('--enable-pulseaudio-modules',
          dest='enable_pulseaudio_modules',
          action='store_true',
          help='enable building of pulseaudio modules')

AddOption('--disable-lib',
          dest='disable_lib',
          action='store_true',
          help='disable libroc building')

AddOption('--disable-tools',
          dest='disable_tools',
          action='store_true',
          help='disable tools building')

AddOption('--disable-tests',
          dest='disable_tests',
          action='store_true',
          help='disable tests building')

AddOption('--disable-examples',
          dest='disable_examples',
          action='store_true',
          help='disable examples building')

AddOption('--disable-doc',
          dest='disable_doc',
          action='store_true',
          help='disable Doxygen and Sphinx documentation generation')

AddOption('--disable-openfec',
          dest='disable_openfec',
          action='store_true',
          help='disable OpenFEC support required for FEC codes')

AddOption('--disable-sox',
          dest='disable_sox',
          action='store_true',
          help='disable SoX support in tools')

AddOption('--disable-libunwind',
          dest='disable_libunwind',
          action='store_true',
          help='disable libunwind support required for printing backtrace')

AddOption('--disable-pulseaudio',
          dest='disable_pulseaudio',
          action='store_true',
          help='disable PulseAudio support in tools')

AddOption('--with-pulseaudio',
          dest='with_pulseaudio',
          action='store',
          type='string',
          help=("path to the PulseAudio source directory used when "+
                "building PulseAudio modules"))

AddOption('--with-pulseaudio-build-dir',
          dest='with_pulseaudio_build_dir',
          action='store',
          type='string',
          help=("path to the PulseAudio build directory used when "+
                "building PulseAudio modules (needed in case you build "+
                "PulseAudio out of source; if empty, the build directory is "+
                "assumed to be the same as the source directory)"))

AddOption('--with-openfec-includes',
          dest='with_openfec_includes',
          action='store',
          type='string',
          help=("path to the directory with OpenFEC headers (it should contain "+
                "lib_common and lib_stable subdirectories)"))

AddOption('--with-includes',
          dest='with_includes',
          action='append',
          type='string',
          help=("additional include directory, may be used multiple times"))

AddOption('--with-libraries',
          dest='with_libraries',
          action='append',
          type='string',
          help=("additional library directory, may be used multiple times"))

AddOption('--build-3rdparty',
          dest='build_3rdparty',
          action='store',
          type='string',
          help=("download and build specified 3rdparty libraries, "+
                "pass a comma-separated list of library names and optional versions, "+
                "e.g. 'uv:1.4.2,openfec'"))

AddOption('--override-targets',
          dest='override_targets',
          action='store',
          type='string',
          help=("override targets to use, "+
                "pass a comma-separated list of target names, "+
                "e.g. 'glibc,stdio,posix,libuv,openfec,...'"))

if GetOption('help'):
    Return()

if 'clean' in COMMAND_LINE_TARGETS and COMMAND_LINE_TARGETS != ['clean']:
    env.Die("combining 'clean' with other targets is not allowed")

cleanbuild = [
    env.DeleteDir('#bin'),
    env.DeleteDir('#build'),
    env.DeleteFile('#compile_commands.json'),
]

cleandocs = [
    env.DeleteDir('#html'),
    env.DeleteDir('#man'),
    env.DeleteDir('#build/docs'),
]

clean = cleanbuild + cleandocs + [
    env.DeleteDir('#3rdparty'),
    env.DeleteFile('#config.log'),
    env.DeleteDir('#.sconf_temp'),
    env.DeleteFile('#.sconsign.dblite'),
]

env.AlwaysBuild(env.Alias('clean', [], clean))
env.AlwaysBuild(env.Alias('cleanbuild', [], cleanbuild))
env.AlwaysBuild(env.Alias('cleandocs', [], cleandocs))

# handle "scons -c"
if env.GetOption('clean'):
    env.Execute(clean)
    Return()

for var in ['CXX', 'CC', 'AR', 'RANLIB', 'RAGEL', 'GENGETOPT', 'PKG_CONFIG', 'CONFIG_GUESS']:
    env.OverrideFromArg(var)

env.OverrideFromArg('CXXLD', names=['CXXLD', 'CXX'])
env.OverrideFromArg('CCLD', names=['CCLD', 'LD', 'CC'])

env.OverrideFromArg('STRIP', default='strip')

env.OverrideFromArg('DOXYGEN', default='doxygen')
env.OverrideFromArg('SPHINX_BUILD', default='sphinx-build')
env.OverrideFromArg('BREATHE_APIDOC', default='breathe-apidoc')

if set(COMMAND_LINE_TARGETS).intersection(['doxygen', 'docs']):
    enable_doxygen = True
elif GetOption('disable_doc') or set(COMMAND_LINE_TARGETS).intersection(['tidy', 'fmt']):
    enable_doxygen = False
else:
    doxygen_version = env.ParseCompilerVersion(env['DOXYGEN'])
    enable_doxygen = doxygen_version and doxygen_version[:2] >= (1, 6)

if enable_doxygen:
    doxygen_targets = [
        env.Doxygen(
            html_dir='html/doxygen',
            build_dir='build/docs/modules',
            config='src/modules/Doxyfile',
            sources=(env.GlobRecursive('#src/modules', ['*.h', '*.dox']) +
                env.GlobRecursive('#docs/images', ['*'])),
            werror=GetOption('enable_werror')),
        env.Doxygen(
            build_dir='build/docs/lib',
            config='src/lib/Doxyfile',
            sources=env.GlobRecursive('#src/lib/include', ['*.h', '*.dox']),
            werror=GetOption('enable_werror')),
    ]
    env.AlwaysBuild(env.Alias('doxygen', doxygen_targets))

if set(COMMAND_LINE_TARGETS).intersection(['sphinx', 'docs']):
    enable_sphinx = True
elif GetOption('disable_doc') or set(COMMAND_LINE_TARGETS).intersection(['tidy', 'fmt']):
    enable_sphinx = False
elif env.HasArg('SPHINX_BUILD') and env.HasArg('BREATHE_APIDOC'):
    enable_sphinx = True
else:
    enable_sphinx = env.Which(env['SPHINX_BUILD']) and env.Which(env['BREATHE_APIDOC'])

if enable_doxygen and enable_sphinx:
    sphinx_targets = [
        env.Sphinx(
            build_dir='build',
            output_type='html',
            output_dir='html/docs',
            source_dir='docs/sphinx',
            sources=(env.GlobRecursive('docs/sphinx', ['*']) +
                env.GlobRecursive('docs/images', ['*']) +
                env.GlobRecursive('#src/lib/include', ['*.h', '*.dox']) +
                doxygen_targets),
            werror=GetOption('enable_werror')),
        env.Sphinx(
            build_dir='build',
            output_type='man',
            output_dir='man',
            source_dir='docs/sphinx',
            sources=env.GlobRecursive('docs/sphinx', ['*']),
            werror=GetOption('enable_werror')),
    ]
    env.AlwaysBuild(env.Alias('sphinx', sphinx_targets))
    for man in ['roc-send', 'roc-recv', 'roc-conv']:
        env.AddDistFile(GetOption('mandir'), '#man/%s.1' % man)

if (enable_doxygen and enable_sphinx) or 'docs' in COMMAND_LINE_TARGETS:
    env.AlwaysBuild(env.Alias('docs', ['doxygen', 'sphinx']))

fmt = []

clang_format_tools = ['clang-format']
for n in range(6, 10):
    clang_format_tools += ['clang-format-3.%s' % n]

clang_format = None
for tool in clang_format_tools:
    if env.Which(tool):
        clang_format = tool
        break

if clang_format and env.ParseCompilerVersion(clang_format) >= (3, 6):
    fmt += [
        env.Action(
            '%s -i %s' % (clang_format, ' '.join(map(str,
                env.GlobRecursive(
                    '#src', ['*.h', '*.cpp'],
                    exclude=open(env.File('#.fmtignore').path).read().split())
            ))),
            env.PrettyCommand('FMT', 'src', 'yellow')
        ),
    ]
elif 'fmt' in COMMAND_LINE_TARGETS:
    print("warning: clang-format >= 3.6 not found")

fmt += [
    env.Action(
        '%s scripts/format.py src/modules' % env.PythonExecutable(),
        env.PrettyCommand('FMT', 'src/modules', 'yellow')
    ),
    env.Action(
        '%s scripts/format.py src/tests' % env.PythonExecutable(),
        env.PrettyCommand('FMT', 'src/tests', 'yellow')
    ),
    env.Action(
        '%s scripts/format.py src/tools' % env.PythonExecutable(),
        env.PrettyCommand('FMT', 'src/tools', 'yellow')
    ),
    env.Action(
        '%s scripts/format.py src/lib/src' % env.PythonExecutable(),
        env.PrettyCommand('FMT', 'src/lib/src', 'yellow')
    ),
]

env.AlwaysBuild(
    env.Alias('fmt', [], fmt))

non_build_targets = ['clean', 'cleandocs', 'fmt', 'docs', 'shpinx', 'doxygen']
if set(COMMAND_LINE_TARGETS) \
  and set(COMMAND_LINE_TARGETS).intersection(non_build_targets) == set(COMMAND_LINE_TARGETS):
    Return()

build = GetOption('build') or ''
host = GetOption('host') or ''
platform = GetOption('platform') or ''
compiler = GetOption('compiler') or ''

if GetOption('enable_debug'):
    variant = 'debug'
else:
    variant = 'release'

if GetOption('enable_debug_3rdparty'):
    thirdparty_variant = 'debug'
else:
    thirdparty_variant = 'release'

# toolchain prefix for compiler, linker, etc
toolchain = host

if not compiler:
    if env.HasArg('CXX'):
        if 'gcc' in env['CXX'] or 'g++' in env['CXX']:
            compiler = 'gcc'
        elif 'clang' in env['CXX']:
            compiler = 'clang'
    else:
        if not toolchain and env.Which('clang'):
            compiler = 'clang'
        else:
            compiler = 'gcc'

if '-' in compiler:
    compiler, compiler_ver = compiler.split('-')
    compiler_ver = tuple(map(int, compiler_ver.split('.')))
else:
    if toolchain:
        compiler_ver = env.ParseCompilerVersion('%s-%s' % (toolchain, compiler))
    else:
        compiler_ver = env.ParseCompilerVersion(compiler)

if not compiler in supported_compilers:
    env.Die("unknown compiler '%s', expected one of: %s",
            compiler, ', '.join(supported_compilers))

if not compiler_ver:
    env.Die("can't detect compiler version for compiler '%s'",
            '-'.join([s for s in [toolchain, compiler] if s]))

conf = Configure(env, custom_tests=env.CustomTests)

if compiler == 'clang':
    conf.FindLLVMDir(compiler_ver)

if compiler == 'clang':
    conf.FindTool('CXX', toolchain, compiler_ver, ['clang++'])
elif compiler == 'gcc':
    conf.FindTool('CXX', toolchain, compiler_ver, ['g++'])

full_compiler_ver = env.ParseCompilerVersion(conf.env['CXX'])
if full_compiler_ver:
    compiler_ver = full_compiler_ver

if not build:
    if conf.FindConfigGuess():
        build = env.ParseConfigGuess(conf.env['CONFIG_GUESS'])

if not build and not host:
    if conf.CheckCanRunProgs():
        build = env.ParseCompilerTarget(conf.env['CXX'])

if not build:
    for c in ['/usr/bin/gcc', '/usr/bin/clang']:
        build = env.ParseCompilerTarget(c)
        if build:
            break

if not build:
    env.Die(("can't detect system type, please specify '--build={type}' manually, "+
             "e.g. '--build=x86_64-pc-linux-gnu'"))

if not host:
    host = env.ParseCompilerTarget(conf.env['CXX'])

if not host:
    host = build

crosscompile = (host != build)

if not platform:
    if 'android' in host:
        platform = 'android'
    elif 'linux' in host:
        platform = 'linux'
    elif 'darwin' in host:
        platform = 'darwin'

if compiler == 'clang':
    conf.FindTool('CC', toolchain, compiler_ver, ['clang'])
    conf.FindTool('CXXLD', toolchain, compiler_ver, ['clang++'])
    conf.FindTool('CCLD', toolchain, compiler_ver, ['clang'])

    install_dir = env.ParseCompilerDirectory(conf.env['CXX'])
    if install_dir:
        prepend_path = [install_dir]
    else:
        prepend_path = []

    conf.FindTool('AR', toolchain, None, ['llvm-ar', 'ar'],
                  prepend_path=prepend_path)

    conf.FindTool('RANLIB', toolchain, None, ['llvm-ranlib', 'ranlib'],
                  prepend_path=prepend_path)

    conf.FindTool('STRIP', toolchain, None, ['llvm-strip', 'strip'],
                  prepend_path=prepend_path)

elif compiler == 'gcc':
    conf.FindTool('CC', toolchain, compiler_ver, ['gcc'])
    conf.FindTool('CXXLD', toolchain, compiler_ver, ['g++'])
    conf.FindTool('CCLD', toolchain, compiler_ver, ['gcc'])
    conf.FindTool('AR', toolchain, None, ['ar'])
    conf.FindTool('RANLIB', toolchain, None, ['ranlib'])
    conf.FindTool('STRIP', toolchain, None, ['strip'])

env['LINK'] = env['CXXLD']
env['SHLINK'] = env['CXXLD']

env.PrependFromArg('CPPFLAGS')
env.PrependFromArg('CXXFLAGS')
env.PrependFromArg('CFLAGS')
env.PrependFromArg('LINKFLAGS', names=['LINKFLAGS', 'LDFLAGS'])
env.PrependFromArg('STRIPFLAGS')

env = conf.Finish()

compiler_spec = '-'.join(
    [s for s in [compiler, '.'.join(map(str, compiler_ver)), variant] if s])

thirdparty_compiler_spec = '-'.join(
    [s for s in [compiler, '.'.join(map(str, compiler_ver)), thirdparty_variant] if s])

build_dir = 'build/%s/%s' % (
    host,
    compiler_spec)

env['ROC_BINDIR'] = '#bin/%s' % host

env['ROC_VERSION'] = env.ParseProjectVersion()
env['ROC_SHA'] = env.ParseGitHead()

if env['ROC_SHA']:
    env['ROC_VERSION_STR'] = '%s (%s)' % (env['ROC_VERSION'], env['ROC_SHA'])
else:
    env['ROC_VERSION_STR'] = env['ROC_VERSION']

abi_version = '.'.join(env['ROC_VERSION'].split('.')[:2])

env['ROC_MODULES'] = [
    'roc_core',
    'roc_address',
    'roc_packet',
    'roc_audio',
    'roc_rtp',
    'roc_fec',
    'roc_netio',
    'roc_sndio',
    'roc_pipeline',
]

env['ROC_TARGETS'] = []

if GetOption('override_targets'):
    for t in GetOption('override_targets').split(','):
        env['ROC_TARGETS'] += ['target_%s' % t]
else:
    if not platform:
        env.Die(("can't detect platform for host '%s', looked for one of: %s\nyou should "+
                 "provide either known '--platform' or '--override-targets' option"),
                    host, ', '.join(supported_platforms))

    if platform in ['linux', 'android', 'darwin']:
        env.Append(ROC_TARGETS=[
            'target_posix',
            'target_stdio',
            'target_gcc',
            'target_libuv',
        ])

    if platform in ['linux', 'android']:
        env.Append(ROC_TARGETS=[
            'target_posixtime',
            'target_linux',
        ])

    if platform in ['linux']:
        if not GetOption('disable_libunwind'):
            env.Append(ROC_TARGETS=[
                'target_libunwind',
            ])
        else:
            env.Append(ROC_TARGETS=[
                'target_nobacktrace',
            ])

    if platform in ['android']:
        env.Append(ROC_TARGETS=[
            'target_bionic',
        ])

    if platform in ['darwin']:
        env.Append(ROC_TARGETS=[
            'target_darwin',
            'target_libunwind',
        ])

    is_glibc = not 'musl' in host

    if is_glibc:
        env.Append(ROC_TARGETS=[
            'target_glibc',
        ])
    else:
        env.Append(ROC_TARGETS=[
            'target_nodemangle',
        ])

    if not GetOption('disable_openfec'):
        env.Append(ROC_TARGETS=[
            'target_openfec',
        ])

    if not GetOption('disable_tools') or not GetOption('disable_examples'):
        if not GetOption('disable_sox'):
            env.Append(ROC_TARGETS=[
                'target_sox',
            ])

        if platform in ['linux'] and not GetOption('disable_pulseaudio'):
            env.Append(ROC_TARGETS=[
                'target_pulseaudio',
            ])

env.Append(CXXFLAGS=[])
env.Append(CPPDEFINES=[])
env.Append(CPPPATH=[])
env.Append(LIBPATH=[])
env.Append(LIBS=[])
env.Append(STRIPFLAGS=[])

if GetOption('with_includes'):
    env.Append(CPPPATH=GetOption('with_includes'))

if GetOption('with_libraries'):
    env.Append(LIBPATH=GetOption('with_libraries'))

lib_env = env.Clone()
gen_env = env.Clone()
tool_env = env.Clone()
test_env = env.Clone()
pulse_env = env.Clone()

# all possible dependencies on this platform
all_dependencies = set([t.replace('target_', '') for t in env['ROC_TARGETS']])

# on macos libunwind is provided by the OS
if platform in ['darwin']:
    all_dependencies.discard('libunwind')

all_dependencies.add('ragel')

if not GetOption('disable_tools'):
    all_dependencies.add('gengetopt')

if not GetOption('disable_tests'):
    all_dependencies.add('cpputest')

if ((not GetOption('disable_tools') \
        or not GetOption('disable_examples')) \
    and not GetOption('disable_pulseaudio')) \
  or GetOption('enable_pulseaudio_modules'):
    if platform in ['linux', 'android']:
        all_dependencies.add('alsa')
        all_dependencies.add('pulseaudio')

# dependencies that we should download and build manually
download_dependencies = set()

# dependencies that have explicitly provided version
explicit_version = set()

for name, version in env.ParseThirdParties(GetOption('build_3rdparty')):
    if name != 'all' and not name in thirdparty_versions:
        env.Die("unknown thirdparty name '%s' in '--build-3rdparty', expected any of: %s",
                    name, ', '.join(['all'] + list(sorted(thirdparty_versions.keys()))))
    download_dependencies.add(name)
    if version:
        thirdparty_versions[name] = version
        explicit_version.add(name)

if 'all' in download_dependencies:
    download_dependencies = all_dependencies

# dependencies that should be pre-installed on system
system_dependencies = all_dependencies - download_dependencies

if 'libuv' in system_dependencies:
    conf = Configure(env, custom_tests=env.CustomTests)

    env.ParsePkgConfig('--cflags --libs libuv')

    if not crosscompile:
        if not conf.CheckLibWithHeaderExt(
            'uv', 'uv.h', 'C', expr='UV_VERSION_MAJOR >= 1 && UV_VERSION_MINOR >= 4'):
            env.Die("libuv >= 1.4 not found (see 'config.log' for details)")
    else:
        if not conf.CheckLibWithHeaderExt('uv', 'uv.h', 'C', run=False):
            env.Die("libuv not found (see 'config.log' for details)")

    env = conf.Finish()

if 'libunwind' in system_dependencies:
    conf = Configure(env, custom_tests=env.CustomTests)

    env.ParsePkgConfig('--cflags --libs libunwind')

    if not conf.CheckLibWithHeaderExt('unwind', 'libunwind.h', 'C', run=not crosscompile):
        env.Die("libunwind not found (see 'config.log' for details)")

    env = conf.Finish()

if 'openfec' in system_dependencies:
    conf = Configure(env, custom_tests=env.CustomTests)

    if env.ParsePkgConfig('--silence-errors --cflags --libs openfec'):
        pass
    elif GetOption('with_openfec_includes'):
        openfec_includes = GetOption('with_openfec_includes')
        env.Append(CPPPATH=[
            openfec_includes,
            '%s/lib_common' % openfec_includes,
            '%s/lib_stable' % openfec_includes,
        ])
    elif not crosscompile:
       for prefix in ['/usr/local', '/usr']:
           if os.path.exists('%s/include/openfec' % prefix):
               env.Append(CPPPATH=[
                   '%s/include/openfec' % prefix,
                   '%s/include/openfec/lib_common' % prefix,
                   '%s/include/openfec/lib_stable' % prefix,
               ])
               env.Append(LIBPATH=[
                   '%s/lib' % prefix,
               ])
               break

    if not conf.CheckLibWithHeaderExt(
            'openfec', 'of_openfec_api.h', 'C', run=not crosscompile):
        env.Die("openfec not found (see 'config.log' for details)")

    if not conf.CheckDeclaration('OF_USE_ENCODER', '#include <of_openfec_api.h>', 'c'):
        env.Die("openfec has no encoder support (OF_USE_ENCODER)")

    if not conf.CheckDeclaration('OF_USE_DECODER', '#include <of_openfec_api.h>', 'c'):
        env.Die("openfec has no encoder support (OF_USE_DECODER)")

    if not conf.CheckDeclaration('OF_USE_LDPC_STAIRCASE_CODEC',
                                 '#include <of_openfec_api.h>', 'c'):
        env.Die(
            "openfec has no LDPC-Staircase codec support (OF_USE_LDPC_STAIRCASE_CODEC)")

    env = conf.Finish()

if 'pulseaudio' in system_dependencies:
    conf = Configure(tool_env, custom_tests=env.CustomTests)

    tool_env.ParsePkgConfig('--cflags --libs libpulse')

    if not conf.CheckLibWithHeaderExt(
            'pulse', 'pulse/pulseaudio.h', 'C', run=not crosscompile):
        env.Die("libpulse not found (see 'config.log' for details)")

    tool_env = conf.Finish()

    if GetOption('enable_pulseaudio_modules'):
        conf = Configure(pulse_env, custom_tests=env.CustomTests)

        if not conf.CheckLibWithHeaderExt('ltdl', 'ltdl.h', 'C', run=not crosscompile):
            env.Die("ltdl not found (see 'config.log' for details)")

        pulse_env = conf.Finish()

        pa_src_dir = GetOption('with_pulseaudio')
        if not pa_src_dir:
            env.Die('--enable-pulseaudio-modules requires either --with-pulseaudio'+
                    ' or --build-3rdparty=pulseaudio')

        pa_build_dir = GetOption('with_pulseaudio_build_dir')
        if not pa_build_dir:
            pa_build_dir = pa_src_dir

        pulse_env.Append(CPPPATH=[
            pa_build_dir,
            pa_src_dir + '/src',
        ])

        for lib in ['libpulsecore-*.so', 'libpulsecommon-*.so']:
            path = '%s/src/.libs/%s' % (pa_build_dir, lib)
            libs = env.Glob(path)
            if not libs:
                env.Die("can't find %s" % path)

            pulse_env.Append(LIBS=libs)

            m = re.search('-([0-9.]+).so$', libs[0].path)
            if m:
                pa_ver = m.group(1)

        if not pa_ver:
            env.Die("can't determine pulseaudio version")

        env['ROC_PULSE_VERSION'] = pa_ver

if 'sox' in system_dependencies:
    conf = Configure(tool_env, custom_tests=env.CustomTests)

    tool_env.ParsePkgConfig('--cflags --libs sox')

    if not crosscompile:
        if not conf.CheckLibWithHeaderExt(
                'sox', 'sox.h', 'C',
                expr='SOX_LIB_VERSION_CODE >= SOX_LIB_VERSION(14, 4, 0)'):
            env.Die("libsox >= 14.4.0 not found (see 'config.log' for details)")
    else:
        if not conf.CheckLibWithHeaderExt('sox', 'sox.h', 'C', run=False):
            env.Die("libsox not found (see 'config.log' for details)")

    tool_env = conf.Finish()

if 'ragel' in system_dependencies:
    conf = Configure(env, custom_tests=env.CustomTests)

    if 'RAGEL' in env.Dictionary():
        ragel = env['RAGEL']
    else:
        ragel = 'ragel'

    if not conf.CheckProg(ragel):
        env.Die("ragel not found in PATH (looked for '%s')" % ragel)

    env = conf.Finish()

if 'gengetopt' in system_dependencies:
    conf = Configure(env, custom_tests=env.CustomTests)

    if 'GENGETOPT' in env.Dictionary():
        gengetopt = env['GENGETOPT']
    else:
        gengetopt = 'gengetopt'

    if not conf.CheckProg(gengetopt):
        env.Die("gengetopt not found in PATH (looked for '%s')" % gengetopt)

    env = conf.Finish()

if 'cpputest' in system_dependencies:
    conf = Configure(test_env, custom_tests=env.CustomTests)

    test_env.ParsePkgConfig('--cflags --libs cpputest')

    if not conf.CheckLibWithHeaderExt(
            'CppUTest', 'CppUTest/TestHarness.h', 'CXX', run=not crosscompile):
        test_env.Die("CppUTest not found (see 'config.log' for details)")

    test_env = conf.Finish()

if 'libuv' in download_dependencies:
    env.ThirdParty(host, thirdparty_compiler_spec, toolchain,
                   thirdparty_variant, thirdparty_versions, 'libuv')

if 'libunwind' in download_dependencies:
    env.ThirdParty(host, thirdparty_compiler_spec,
                   toolchain, thirdparty_variant,
                   thirdparty_versions, 'libunwind')

if 'openfec' in download_dependencies:
    env.ThirdParty(host, thirdparty_compiler_spec, toolchain,
                   thirdparty_variant, thirdparty_versions,
                   'openfec', includes=[
                        'lib_common',
                        'lib_stable',
                        ])

if 'alsa' in download_dependencies:
    tool_env.ThirdParty(host, thirdparty_compiler_spec, toolchain,
                        thirdparty_variant, thirdparty_versions, 'alsa')

if 'pulseaudio' in download_dependencies:
    if not 'pulseaudio' in explicit_version and not crosscompile:
        pa_ver = env.ParseToolVersion('pulseaudio --version')
        if pa_ver:
            thirdparty_versions['pulseaudio'] = pa_ver

    pa_deps = [
        'ltdl',
        'json',
        'sndfile',
        ]

    if 'alsa' in download_dependencies:
        pa_deps += ['alsa']

    env['ROC_PULSE_VERSION'] = thirdparty_versions['pulseaudio']

    tool_env.ThirdParty(host, thirdparty_compiler_spec, toolchain,
                        thirdparty_variant, thirdparty_versions, 'ltdl')
    tool_env.ThirdParty(host, thirdparty_compiler_spec, toolchain,
                        thirdparty_variant, thirdparty_versions, 'json')
    tool_env.ThirdParty(host, thirdparty_compiler_spec, toolchain,
                        thirdparty_variant, thirdparty_versions, 'sndfile')
    tool_env.ThirdParty(host, thirdparty_compiler_spec, toolchain,
                        thirdparty_variant, thirdparty_versions,
                        'pulseaudio', deps=pa_deps, libs=['pulse', 'pulse-simple'])

    pulse_env.ImportThridParty(host, thirdparty_compiler_spec, toolchain,
                               thirdparty_versions, 'ltdl')
    pulse_env.ImportThridParty(host, thirdparty_compiler_spec, toolchain,
                               thirdparty_versions, 'pulseaudio',
                               libs=[
                                   'pulsecore-%s' % thirdparty_versions['pulseaudio'],
                                   'pulsecommon-%s' % thirdparty_versions['pulseaudio'],
                                   ])

if 'sox' in download_dependencies:
    sox_deps = []

    if 'alsa' in download_dependencies:
        sox_deps += ['alsa']

    if 'pulseaudio' in download_dependencies:
        sox_deps += ['pulseaudio']

    tool_env.ThirdParty(host, thirdparty_compiler_spec, toolchain,
                        thirdparty_variant, thirdparty_versions, 'sox', sox_deps)

    conf = Configure(tool_env, custom_tests=env.CustomTests)

    for lib in [
            'z', 'magic',
            'gsm', 'FLAC',
            'vorbis', 'vorbisenc', 'vorbisfile', 'ogg',
            'mad', 'mp3lame']:
        conf.CheckLib(lib)

    if not 'alsa' in download_dependencies:
        for lib in [
                'asound',
                ]:
            conf.CheckLib(lib)

    if not 'pulseaudio' in download_dependencies:
        for lib in [
                'sndfile',
                'pulse', 'pulse-simple',
                ]:
            conf.CheckLib(lib)

    if platform in ['darwin']:
        tool_env.Append(LINKFLAGS=[
            '-Wl,-framework,CoreAudio'
        ])

    tool_env = conf.Finish()

if 'ragel' in download_dependencies:
    env.ThirdParty(build, thirdparty_compiler_spec, "",
                   thirdparty_variant, thirdparty_versions, 'ragel')

    gen_env['RAGEL'] = env.File(
        '#3rdparty/%s/%s/build/ragel-%s/bin/ragel' % (
            build + env['PROGSUFFIX'], thirdparty_compiler_spec,
            thirdparty_versions['ragel']))

if 'gengetopt' in download_dependencies:
    env.ThirdParty(build, thirdparty_compiler_spec, "",
                   thirdparty_variant, thirdparty_versions, 'gengetopt')

    gen_env['GENGETOPT'] = env.File(
        '#3rdparty/%s/%s/build/gengetopt-%s/bin/gengetopt' % (
            build + env['PROGSUFFIX'], thirdparty_compiler_spec,
            thirdparty_versions['gengetopt']))

if 'cpputest' in download_dependencies:
    test_env.ThirdParty(host, thirdparty_compiler_spec, toolchain,
                        thirdparty_variant, thirdparty_versions, 'cpputest')

conf = Configure(env, custom_tests=env.CustomTests)

conf.env['ROC_SYSTEM_BINDIR'] = GetOption('bindir')
conf.env['ROC_SYSTEM_INCDIR'] = GetOption('incdir')

if GetOption('libdir'):
    conf.env['ROC_SYSTEM_LIBDIR'] = GetOption('libdir')
else:
    conf.FindLibDir(GetOption('prefix'), host)

if GetOption('enable_pulseaudio_modules'):
    if GetOption('pulseaudio_module_dir'):
        conf.env['ROC_PULSE_MODULEDIR'] = GetOption('pulseaudio_module_dir')
    else:
        conf.FindPulseDir(GetOption('prefix'), build, host, env['ROC_PULSE_VERSION'])

env = conf.Finish()

if 'target_posix' in env['ROC_TARGETS'] and platform not in ['darwin']:
    env.Append(CPPDEFINES=[('_POSIX_C_SOURCE', '200809')])

for t in env['ROC_TARGETS']:
    env.Append(CPPDEFINES=['ROC_' + t.upper()])

env.Append(LIBPATH=['#%s' % build_dir])

if platform in ['linux']:
    env.AppendUnique(LIBS=['rt', 'dl', 'm'])

if compiler in ['gcc', 'clang']:
    env.Append(CXXFLAGS=[
        '-std=c++98',
        '-fno-exceptions',
    ])

    for var in ['CXXFLAGS', 'CFLAGS']:
        env.Append(**{var: [
            '-pthread',
            '-fPIC',
        ]})

    if platform in ['linux', 'darwin']:
        env.Append(LIBS=[
            'pthread',
        ])

    if platform in ['linux', 'android']:
        test_env['RPATH'] = test_env.Literal('\\$$ORIGIN')

        lib_env['SHLIBSUFFIX'] = '%s.%s' % (lib_env['SHLIBSUFFIX'], abi_version)
        lib_env.Append(LINKFLAGS=[
            '-Wl,-soname,libroc%s' % lib_env['SHLIBSUFFIX'],
        ])

        if variant == 'release':
            lib_env.Append(LINKFLAGS=[
                '-Wl,--version-script=%s' % env.File('#src/lib/roc.version').path
            ])

    if platform in ['darwin']:
        lib_env['SHLIBSUFFIX'] = '.%s%s' % (abi_version, lib_env['SHLIBSUFFIX'])
        lib_env.Append(LINKFLAGS=[
            '-Wl,-compatibility_version,%s' % abi_version,
            '-Wl,-current_version,%s' % env['ROC_VERSION'],
            '-Wl,-install_name,%s/libroc%s' % (
                env.Dir(env['ROC_BINDIR']).abspath, lib_env['SHLIBSUFFIX']),
        ])

    if not(compiler == 'clang' and variant == 'debug'):
        env.Append(CXXFLAGS=[
            '-fno-rtti',
        ])

    if variant == 'debug':
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
        env.Append(CXXFLAGS=[
            '-fvisibility=hidden',
            '-O3',
        ])

    if compiler == 'gcc' and compiler_ver[:2] < (4, 6):
        for var in ['CXXFLAGS', 'CFLAGS']:
            env.Append(**{var: [
                '-fno-strict-aliasing',
            ]})

else:
    env.Die("CXXFLAGS setup not implemented for compiler '%s'", compiler)

if compiler in ['gcc', 'clang']:
    if GetOption('enable_werror'):
        for var in ['CXXFLAGS', 'CFLAGS']:
            env.Append(**{var: [
                '-Werror',
            ]})

if compiler == 'gcc':
    for var in ['CXXFLAGS', 'CFLAGS']:
        env.Append(**{var: [
            '-Wall',
            '-Wextra',
            '-Wcast-qual',
            '-Wfloat-equal',
            '-Wpointer-arith',
            '-Wformat=2',
            '-Wformat-security',
            '-Wno-system-headers',
            '-Wno-psabi',
        ]})

    env.Append(CXXFLAGS=[
        '-Wctor-dtor-privacy',
        '-Wnon-virtual-dtor',
        '-Wstrict-null-sentinel',
        '-Wno-invalid-offsetof',
    ])

    if compiler_ver[:2] >= (4, 4):
        for var in ['CXXFLAGS', 'CFLAGS']:
            env.Append(**{var: [
                '-Wlogical-op',
                '-Woverlength-strings',
            ]})
        env.Append(CXXFLAGS=[
            '-Wmissing-declarations',
        ])

    if compiler_ver[:2] >= (4, 8):
        for var in ['CXXFLAGS', 'CFLAGS']:
            env.Append(**{var: [
                '-Wdouble-promotion',
            ]})

    if compiler_ver[:2] >= (8, 0):
        for var in ['CXXFLAGS', 'CFLAGS']:
            env.Append(**{var: [
                '-Wno-parentheses',
            ]})

if compiler == 'clang':
    for var in ['CXXFLAGS', 'CFLAGS']:
        env.Append(**{var: [
            '-Weverything',
            '-Wno-system-headers',
            '-Wno-old-style-cast',
            '-Wno-padded',
            '-Wno-packed',
            '-Wno-cast-align',
            '-Wno-global-constructors',
            '-Wno-exit-time-destructors',
            '-Wno-invalid-offsetof',
            '-Wno-shift-sign-overflow',
            '-Wno-used-but-marked-unused',
            '-Wno-unused-macros',
            '-Wno-format-nonliteral',
            '-Wno-variadic-macros',
            '-Wno-disabled-macro-expansion',
            '-Wno-c++11-long-long',
        ]})

    env.Append(CFLAGS=[
        '-Wno-missing-prototypes',
    ])

    if compiler_ver[:2] >= (3, 6):
        for var in ['CXXFLAGS', 'CFLAGS']:
            env.Append(**{var: [
                '-Wno-reserved-id-macro',
            ]})
    else:
        for var in ['CXXFLAGS', 'CFLAGS']:
            env.Append(**{var: [
                '-Wno-unreachable-code',
            ]})

    if platform in ['linux', 'android']:
        if compiler_ver[:2] >= (6, 0):
            for var in ['CXXFLAGS', 'CFLAGS']:
                env.Append(**{var: [
                    '-Wno-redundant-parens',
                ]})

    if platform in ['linux']:
        if compiler_ver[:2] >= (8, 0):
            for var in ['CXXFLAGS', 'CFLAGS']:
                env.Append(**{var: [
                    '-Wno-extra-semi-stmt',
                    '-Wno-atomic-implicit-seq-cst',
                ]})

    if platform == 'darwin':
        if compiler_ver[:2] >= (10, 0):
            for var in ['CXXFLAGS', 'CFLAGS']:
                env.Append(**{var: [
                    '-Wno-redundant-parens',
                ]})
        if compiler_ver[:2] >= (11, 0):
            for var in ['CXXFLAGS', 'CFLAGS']:
                env.Append(**{var: [
                    '-Wno-atomic-implicit-seq-cst',
                ]})

if compiler in ['gcc', 'clang']:
    for e in [env, lib_env, tool_env, test_env, pulse_env]:
        for var in ['CXXFLAGS', 'CFLAGS']:
            dirs = [('-isystem', env.Dir(path).path)
                    for path in e['CPPPATH'] + ['%s/tools' % build_dir]]

            # workaround to force our 3rdparty directories to be placed
            # before /usr/local/include on macos
            if compiler == 'clang' and platform == 'darwin':
                dirs += [('-isystem', '/usr/local/include')]

            e.Prepend(**{var: dirs})

    for var in ['CC', 'CXX']:
        env[var] = env.ClangDBWriter(env[var], build_dir)

    compile_commands = '%s/compile_commands.json' % build_dir

    env.Artifact(compile_commands, '#src')
    env.Install('#', compile_commands)

sanitizers = env.ParseList(GetOption('sanitizers'), supported_sanitizers)
if sanitizers:
    if not compiler in ['gcc', 'clang']:
        env.Die("sanitizers are not supported for compiler '%s'" % compiler)

    for name in sanitizers:
        flags = ['-fsanitize=%s' % name]

        env.AppendUnique(CFLAGS=flags)
        env.AppendUnique(CXXFLAGS=flags)
        env.AppendUnique(LINKFLAGS=flags)
else:
    if platform in ['linux', 'android']:
        env.Append(LINKFLAGS=[
            '-Wl,--no-undefined',
        ])

if platform in ['linux']:
    tool_env.Append(LINKFLAGS=[
        '-Wl,-rpath-link,%s' % env.Dir('#3rdparty/%s/%s/rpath' % (
            host, thirdparty_compiler_spec)).abspath,
    ])

test_env.Append(CPPDEFINES=('CPPUTEST_USE_MEM_LEAK_DETECTION', '0'))

if compiler == 'clang':
    for var in ['CXXFLAGS', 'CFLAGS']:
        gen_env.AppendUnique(**{var: [
            '-Wno-sign-conversion',
            '-Wno-missing-variable-declarations',
            '-Wno-switch-enum',
            '-Wno-shorten-64-to-32',
            '-Wno-unused-const-variable',
            '-Wno-documentation',
        ]})
    test_env.AppendUnique(CXXFLAGS=[
        '-Wno-weak-vtables',
        '-Wno-unused-member-function',
    ])

if compiler == 'gcc':
    for var in ['CXXFLAGS', 'CFLAGS']:
        gen_env.AppendUnique(**{var: [
            '-Wno-overlength-strings',
        ]})

if not env['STRIPFLAGS']:
    if platform in ['darwin']:
        env.Append(STRIPFLAGS=['-x'])

env.AlwaysBuild(
    env.Alias('tidy', [env.Dir('#')],
        env.Action(
            "clang-tidy -p %s -checks='%s' -header-filter='src/.*' %s" % (
                build_dir,
                ','.join(open(env.File('#.clang-checks').path).read().split()),
                ' '.join(map(str, (env.GlobRecursive('#src/modules', '*.cpp') +
                                   env.GlobRecursive('#src/lib', '*.cpp') +
                                   env.GlobRecursive('#src/tools', '*.cpp'))
                ))
            ),
            env.PrettyCommand('TIDY', 'src', 'yellow')
        )))

Export('env', 'lib_env', 'gen_env', 'tool_env', 'test_env', 'pulse_env')

env.SConscript('src/SConscript',
            variant_dir=build_dir, duplicate=0)
