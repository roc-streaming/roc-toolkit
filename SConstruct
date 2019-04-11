import re
import os
import os.path
import SCons.Script

# supported platform names
supported_platforms = [
    'linux',
    'darwin',
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
    'uv':         '1.5.0',
    'openfec':    '1.4.2.1',
    'cpputest':   '3.6',
    'sox':        '14.4.2',
    'alsa':       '1.0.29',
    'pulseaudio': '5.0',
    'json':       '0.11-20130402',
    'ltdl':       '2.4.6',
    'sndfile':    '1.0.20',
    'gengetopt':  '2.22.6',
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

AddOption('--prefix',
          dest='prefix',
          action='store',
          type='string',
          default='/usr',
          help="installation prefix, /usr by default")

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
                "equal to --build if empty"))

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
          "supported names: '', 'all', "+
          ', '.join(["'%s'" % s for s in supported_sanitizers]))

AddOption('--enable-debug',
          dest='enable_debug',
          action='store_true',
          help='enable debug build')

AddOption('--enable-debug-3rdparty',
          dest='enable_debug_3rdparty',
          action='store_true',
          help='enable debug build for 3rdparty libraries')

AddOption('--enable-werror',
          dest='enable_werror',
          action='store_true',
          help='enable -Werror compiler option')

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
          help='disable Doxygen documentation generation')

AddOption('--disable-openfec',
          dest='disable_openfec',
          action='store_true',
          help='disable OpenFEC support required for FEC codes')

AddOption('--with-pulseaudio',
          dest='with_pulseaudio',
          action='store',
          type='string',
          help=("path to the fully built pulseaudio source directory used when "+
                "building pulseaudio modules"))

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
                "e.g. 'gnu,posix,uv,openfec,...'"))

if GetOption('help'):
    Return()

if 'clean' in COMMAND_LINE_TARGETS and COMMAND_LINE_TARGETS != ['clean']:
    env.Die("combining 'clean' with other targets is not allowed")

clean = [
    env.DeleteDir('#bin'),
    env.DeleteDir('#build'),
    env.DeleteDir('#3rdparty'),
    env.DeleteDir('#html'),
    env.DeleteDir('#man'),
    env.DeleteDir('#.sconf_temp'),
    env.DeleteFile('#.sconsign.dblite'),
    env.DeleteFile('#config.log'),
    env.DeleteFile('#compile_commands.json'),
]

env.AlwaysBuild(env.Alias('clean', [], clean))

if env.GetOption('clean'):
    env.Execute(clean)
    Return()

for var in ['CC', 'CXX', 'LD', 'AR', 'RANLIB',
            'GENGETOPT', 'DOXYGEN', 'SPHINX_BUILD', 'BREATHE_APIDOC', 'PKG_CONFIG']:
    if env.HasArg(var):
        env[var] = env.GetArg(var)

if not 'DOXYGEN' in env.Dictionary():
    env['DOXYGEN'] = 'doxygen'

if not 'SPHINX_BUILD' in env.Dictionary():
    env['SPHINX_BUILD'] = 'sphinx-build'

if not 'BREATHE_APIDOC' in env.Dictionary():
    env['BREATHE_APIDOC'] = 'breathe-apidoc'

if set(COMMAND_LINE_TARGETS).intersection(['doxygen', 'docs']):
    enable_doxygen = True
elif GetOption('disable_doc') or set(COMMAND_LINE_TARGETS).intersection(['tidy', 'fmt']):
    enable_doxygen = False
else:
    doxygen_version = env.CompilerVersion(env['DOXYGEN'])
    enable_doxygen = doxygen_version and doxygen_version[:2] >= (1, 6)

if enable_doxygen:
    doxygen_targets = [
        env.Doxygen(
            html_dir='html/doxygen',
            build_dir='build/docs/modules',
            config='src/modules/Doxyfile',
            sources=(env.RecursiveGlob('#src/modules', ['*.h', '*.dox']) +
                env.RecursiveGlob('#docs/images', ['*'])),
            werror=GetOption('enable_werror')),
        env.Doxygen(
            build_dir='build/docs/lib',
            config='src/lib/Doxyfile',
            sources=env.RecursiveGlob('#src/lib/include', ['*.h', '*.dox']),
            werror=GetOption('enable_werror')),
    ]
    env.AlwaysBuild(env.Alias('doxygen', doxygen_targets))

if set(COMMAND_LINE_TARGETS).intersection(['sphinx', 'docs']):
    enable_sphinx = True
elif GetOption('disable_doc') or set(COMMAND_LINE_TARGETS).intersection(['tidy', 'fmt']):
    enable_sphinx = False
else:
    enable_sphinx = env.Which(env['SPHINX_BUILD']) and env.Which(env['BREATHE_APIDOC'])

if enable_doxygen and enable_sphinx:
    sphinx_targets = [
        env.Sphinx(
            build_dir='build',
            output_type='html',
            output_dir='html/docs',
            source_dir='docs/sphinx',
            sources=(env.RecursiveGlob('docs/sphinx', ['*']) +
                env.RecursiveGlob('docs/images', ['*']) +
                env.RecursiveGlob('#src/lib/include', ['*.h', '*.dox']) +
                doxygen_targets),
            werror=GetOption('enable_werror')),
        env.Sphinx(
            build_dir='build',
            output_type='man',
            output_dir='man',
            source_dir='docs/sphinx',
            sources=env.RecursiveGlob('docs/sphinx', ['*']),
            werror=GetOption('enable_werror')),
    ]
    env.AlwaysBuild(env.Alias('sphinx', sphinx_targets))
    for man in ['roc-send', 'roc-recv', 'roc-conv']:
        env.AddDistfile(GetOption('prefix'), 'share/man/man1', '#man/%s.1' % man)

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

if clang_format and env.CompilerVersion(clang_format) >= (3, 6):
    fmt += [
        env.Action(
            '%s -i %s' % (clang_format, ' '.join(map(str,
                env.RecursiveGlob(
                    '#src', ['*.h', '*.cpp'],
                    exclude=open(env.File('#.fmtignore').path).read().split())
            ))),
            env.Pretty('FMT', 'src', 'yellow')
        ),
    ]
elif 'fmt' in COMMAND_LINE_TARGETS:
    print("warning: clang-format >= 3.6 not found")

fmt += [
    env.Action(
        '%s scripts/format.py src/modules' % env.Python(),
        env.Pretty('FMT', 'src/modules', 'yellow')
    ),
    env.Action(
        '%s scripts/format.py src/tests' % env.Python(),
        env.Pretty('FMT', 'src/tests', 'yellow')
    ),
    env.Action(
        '%s scripts/format.py src/tools' % env.Python(),
        env.Pretty('FMT', 'src/tools', 'yellow')
    ),
    env.Action(
        '%s scripts/format.py src/lib/src' % env.Python(),
        env.Pretty('FMT', 'src/lib/src', 'yellow')
    ),
]

env.AlwaysBuild(
    env.Alias('fmt', [], fmt))

non_build_targets = ['clean', 'fmt', 'docs', 'shpinx', 'doxygen']
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
    if not toolchain and env.Which('clang'):
        compiler = 'clang'
    else:
        compiler = 'gcc'

if '-' in compiler:
    compiler, compiler_ver = compiler.split('-')
    compiler_ver = tuple(map(int, compiler_ver.split('.')))
else:
    if toolchain:
        compiler_ver = env.CompilerVersion('%s-%s' % (toolchain, compiler))
    else:
        compiler_ver = env.CompilerVersion(compiler)

if not compiler in supported_compilers:
    env.Die("unknown compiler '%s', expected one of: %s",
            compiler, ', '.join(supported_compilers))

if not compiler_ver:
    env.Die("can't detect compiler version for compiler '%s'",
            '-'.join([s for s in [toolchain, compiler] if s]))

llvmdir = env.LLVMDir(compiler_ver)
if llvmdir:
    env['ENV']['PATH'] += ':%s/bin' % llvmdir

conf = Configure(env, custom_tests=env.CustomTests)

unversioned = set(['ar', 'ranlib'])

tools = dict()

if compiler == 'gcc':
    tools['CC'] = ['gcc']
    tools['CXX'] = ['g++']
    tools['LD'] = ['g++']
    tools['AR'] = ['ar']
    tools['RANLIB'] = ['ranlib']

if compiler == 'clang':
    tools['CC'] = ['clang']
    tools['CXX'] = ['clang++']
    tools['LD'] = ['clang++']
    tools['AR'] = ['llvm-ar', 'ar']
    tools['RANLIB'] = ['llvm-ranlib', 'ranlib']

checked = set()

for var in ['CC', 'CXX', 'LD', 'AR', 'RANLIB']:
    if env.HasArg(var):
        if not env[var] in checked:
            conf.CheckProg(env[var])
    else:
        for tool_name in tools[var]:
            if not toolchain:
                tool = tool_name
            else:
                tool = '%s-%s' % (toolchain, tool_name)

            if not tool_name in unversioned:
                search_versions = [
                    compiler_ver[:3],
                    compiler_ver[:2],
                ]

                default_ver = env.CompilerVersion(tool)

                if default_ver and default_ver[:len(compiler_ver)] == compiler_ver:
                    search_versions += [default_ver]

                for ver in reversed(sorted(set(search_versions))):
                    versioned_tool = '%s-%s' % (tool, '.'.join(map(str, ver)))
                    if env.Which(versioned_tool):
                        tool = versioned_tool
                        break

            if env.Which(tool):
                env[var] = tool
                break
        else:
            env.Die("can't detect %s: looked for any of: %s" % (
                var,
                ', '.join(tools[var])))

        if not env[var] in checked:
            conf.CheckProg(env[var])

            if not tool_name in unversioned:
                actual_ver = env.CompilerVersion(env[var])
                if actual_ver:
                    actual_ver = actual_ver[:len(compiler_ver)]

                if actual_ver != compiler_ver:
                    env.Die(
                        "can't detect %s: '%s' not found in PATH, '%s' version is %s" % (
                            var,
                            '%s-%s' % (tool, '.'.join(map(str, compiler_ver))),
                            env[var],
                            actual_ver))

    checked.add(env[var])

env['LINK'] = env['LD']
env['SHLINK'] = env['LD']

for var in ['CFLAGS', 'CXXFLAGS', 'LDFLAGS']:
    if env.HasArg(var):
        if var == 'LDFLAGS':
            tvar = 'LINKFLAGS'
        else:
            tvar = var
        env.Prepend(**{tvar: env.GetArg(var)})

env = conf.Finish()

# get full compiler version
compiler_ver = env.CompilerVersion(env['CXX'])

if not build:
    build = env.CompilerTarget(env['CXX'])
    if not build:
        env.Die(("can't detect system type, please specify 'build={type}' manually, "+
                 "e.g. 'build=x86_64-pc-linux-gnu'"))

if not host:
    host = build

if not platform:
    if 'linux' in host:
        platform = 'linux'
    elif 'darwin' in host:
        platform = 'darwin'

if not GetOption('override_targets'):
    if not platform:
        env.Die(("can't detect platform for host '%s', looked for one of: %s\nyou should "+
                 "provide either known '--platform' or '--override-targets' option"),
                    host, ', '.join(supported_platforms))

crosscompile = (host != build)

build_dir = 'build/%s/%s' % (
    host,
    '-'.join([s for s in [compiler, '.'.join(map(str, compiler_ver)), variant] if s]))

if compiler in ['gcc', 'clang']:
    for var in ['CC', 'CXX']:
        env[var] = env.ClangDB(build_dir, env[var])

    clangdb = env.Install('#', '%s/compile_commands.json' % build_dir)
    env.Requires(clangdb, env.Dir('#src'))

env['ROC_BINDIR'] = '#bin/%s' % host
env['ROC_VERSION'] = open(env.File('#.version').path).read().strip()

env['ROC_MODULES'] = [
    'roc_core',
    'roc_packet',
    'roc_audio',
    'roc_rtp',
    'roc_fec',
    'roc_pipeline',
    'roc_netio',
    'roc_sndio',
]

env['ROC_TARGETS'] = []

if GetOption('override_targets'):
    for t in GetOption('override_targets').split(','):
        env['ROC_TARGETS'] += ['target_%s' % t]
else:
    if platform in ['linux', 'darwin']:
        env.Append(ROC_TARGETS=[
            'target_stdio',
            'target_gnu',
            'target_uv',
            'target_posix',
        ])

    if platform in ['linux']:
        env.Append(ROC_TARGETS=[
            'target_posixtime',
        ])

    if platform in ['darwin']:
        env.Append(ROC_TARGETS=[
            'target_darwin',
        ])

    if not GetOption('disable_tools') or not GetOption('disable_examples'):
        env.Append(ROC_TARGETS=[
            'target_sox',
        ])

    if not GetOption('disable_openfec'):
        env.Append(ROC_TARGETS=[
            'target_openfec',
        ])

env.Append(CXXFLAGS=[])
env.Append(CPPDEFINES=[])
env.Append(CPPPATH=[])
env.Append(LIBPATH=[])
env.Append(LIBS=[])

lib_env = env.Clone()
gen_env = env.Clone()
tool_env = env.Clone()
test_env = env.Clone()
pulse_env = env.Clone()

# all possible dependencies on this platform
all_dependencies = set(env['ROC_TARGETS'])

if not GetOption('disable_tests'):
    all_dependencies.add('target_cpputest')

if not GetOption('disable_tools'):
    all_dependencies.add('target_gengetopt')

if not GetOption('disable_tools') \
  or not GetOption('disable_examples') \
  or GetOption('enable_pulseaudio_modules'):
    if platform in ['linux']:
        all_dependencies.add('target_alsa')
        all_dependencies.add('target_pulseaudio')

# dependencies that we should download and build manually
download_dependencies = set()

# dependencies that have explicitly provided version
explicit_version = set()

for name, version in env.ParseThirdParties(GetOption('build_3rdparty')):
    download_dependencies.add('target_%s' % name)
    if version:
        thirdparty_versions[name] = version
        explicit_version.add(name)

if 'target_all' in download_dependencies:
    download_dependencies = all_dependencies

# dependencies that should be pre-installed on system
system_dependecies = all_dependencies - download_dependencies

if 'target_uv' in system_dependecies:
    conf = Configure(env, custom_tests=env.CustomTests)

    env.TryParseConfig('--cflags --libs libuv')

    if not crosscompile:
        if not conf.CheckLibWithHeaderExpr(
            'uv', 'uv.h', 'c', expr='UV_VERSION_MAJOR >= 1 && UV_VERSION_MINOR >= 4'):
            env.Die("libuv >= 1.4 not found (see 'config.log' for details)")
    else:
        if not conf.CheckLibWithHeaderUniq('uv', 'uv.h', 'c'):
            env.Die("libuv not found (see 'config.log' for details)")

    env = conf.Finish()

if 'target_openfec' in system_dependecies:
    conf = Configure(env, custom_tests=env.CustomTests)

    if not env.TryParseConfig('--silence-errors --cflags --libs openfec') \
      and not crosscompile:
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

    if not conf.CheckLibWithHeaderUniq('openfec', 'of_openfec_api.h', 'c'):
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

if 'target_pulseaudio' in system_dependecies and GetOption('enable_pulseaudio_modules'):
    conf = Configure(pulse_env, custom_tests=env.CustomTests)

    if not conf.CheckLibWithHeaderUniq('ltdl', 'ltdl.h', 'c'):
        env.Die("ltdl not found (see 'config.log' for details)")

    pulse_env = conf.Finish()

    pa_dir = GetOption('with_pulseaudio')
    if not pa_dir:
        env.Die('--enable-pulseaudio-modules requires either --with-pulseaudio'+
                'or --build-3rdparty=pulseaudio')

    pulse_env.Append(CPPPATH=[
        pa_dir,
        pa_dir + '/src',
    ])

    for lib in ['libpulsecore-*.so', 'libpulsecommon-*.so']:
        path = '%s/src/.libs/%s' % (pa_dir, lib)
        libs = env.Glob(path)
        if not libs:
            env.Die("can't find %s" % path)
        pulse_env.Append(LIBS=libs)

if 'target_sox' in system_dependecies:
    conf = Configure(tool_env, custom_tests=env.CustomTests)

    tool_env.TryParseConfig('--cflags --libs sox')

    if not crosscompile:
        if not conf.CheckLibWithHeaderExpr(
                'sox', 'sox.h', 'c',
                expr='SOX_LIB_VERSION_CODE >= SOX_LIB_VERSION(14, 4, 0)'):
            env.Die("libsox >= 14.4.0 not found (see 'config.log' for details)")
    else:
        if not conf.CheckLibWithHeaderUniq('sox', 'sox.h', 'c'):
            env.Die("libsox not found (see 'config.log' for details)")

    tool_env = conf.Finish()

if 'target_gengetopt' in system_dependecies:
    conf = Configure(env, custom_tests=env.CustomTests)

    if 'GENGETOPT' in env.Dictionary():
        gengetopt = env['GENGETOPT']
    else:
        gengetopt = 'gengetopt'

    if not conf.CheckProg(gengetopt):
        env.Die("gengetopt not found in PATH (looked for '%s')" % gengetopt)

    env = conf.Finish()

if 'target_cpputest' in system_dependecies:
    conf = Configure(test_env, custom_tests=env.CustomTests)

    test_env.TryParseConfig('--cflags --libs cpputest')

    if not conf.CheckLibWithHeaderUniq('CppUTest', 'CppUTest/TestHarness.h', 'cxx'):
        test_env.Die("CppUTest not found (see 'config.log' for details)")

    test_env = conf.Finish()

if 'target_uv' in download_dependencies:
    env.ThirdParty(host, toolchain, thirdparty_variant, thirdparty_versions, 'uv')

if 'target_openfec' in download_dependencies:
    env.ThirdParty(host, toolchain, thirdparty_variant, thirdparty_versions, 'openfec',
                   includes=[
                    'lib_common',
                    'lib_stable',
                    ])

if 'target_alsa' in download_dependencies:
    tool_env.ThirdParty(host, toolchain, thirdparty_variant, thirdparty_versions, 'alsa')

if 'target_pulseaudio' in download_dependencies:
    if not 'pulseaudio' in explicit_version and not crosscompile:
        pa_ver = env.ToolVersion(['pulseaudio', '--version'])
        if pa_ver:
            thirdparty_versions['pulseaudio'] = pa_ver

    pa_deps = [
        'ltdl',
        'json',
        'sndfile',
        ]

    if 'target_alsa' in download_dependencies:
        pa_deps += ['alsa']

    env['ROC_PULSE_VERSION'] = thirdparty_versions['pulseaudio']

    tool_env.ThirdParty(host, toolchain, thirdparty_variant, thirdparty_versions, 'ltdl')
    tool_env.ThirdParty(host, toolchain, thirdparty_variant, thirdparty_versions, 'json')
    tool_env.ThirdParty(host, toolchain, thirdparty_variant, thirdparty_versions, 'sndfile')
    tool_env.ThirdParty(host, toolchain, thirdparty_variant, thirdparty_versions,
                        'pulseaudio', deps=pa_deps, libs=['pulse', 'pulse-simple'])

    pulse_env.ImportThridParty(host, toolchain, thirdparty_versions, 'ltdl')
    pulse_env.ImportThridParty(host, toolchain, thirdparty_versions, 'pulseaudio',
                               libs=[
                                   'pulsecore-%s' % thirdparty_versions['pulseaudio'],
                                   'pulsecommon-%s' % thirdparty_versions['pulseaudio'],
                                   ])

if 'target_sox' in download_dependencies:
    sox_deps = []

    if 'target_alsa' in download_dependencies:
        sox_deps += ['alsa']

    if 'target_pulseaudio' in download_dependencies:
        sox_deps += ['pulseaudio']

    tool_env.ThirdParty(
        host, toolchain, thirdparty_variant, thirdparty_versions, 'sox', sox_deps)

    conf = Configure(tool_env, custom_tests=env.CustomTests)

    for lib in [
            'z', 'magic',
            'gsm', 'FLAC',
            'vorbis', 'vorbisenc', 'vorbisfile', 'ogg',
            'mad', 'mp3lame']:
        conf.CheckLib(lib)

    if not 'target_alsa' in download_dependencies:
        for lib in [
                'asound',
                ]:
            conf.CheckLib(lib)

    if not 'target_pulseaudio' in download_dependencies:
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

if 'target_gengetopt' in download_dependencies:
    env.ThirdParty(build, "", thirdparty_variant, thirdparty_versions, 'gengetopt')

    gen_env['GENGETOPT'] = env.File(
        '#3rdparty/%s/build/gengetopt-2.22.6/bin/gengetopt' % build + env['PROGSUFFIX'])

if 'target_cpputest' in download_dependencies:
    test_env.ThirdParty(
        host, toolchain, thirdparty_variant, thirdparty_versions, 'cpputest')

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

    env.Append(LIBS=[
        'pthread',
    ])

    if platform in ['linux']:
        lib_env.Append(LINKFLAGS=[
            '-Wl,--version-script=' + env.File('#src/lib/roc.version').path
        ])

    if not(compiler == 'clang' and variant == 'debug'):
        env.Append(CXXFLAGS=[
            '-fno-rtti',
        ])

    if GetOption('enable_werror'):
        for var in ['CXXFLAGS', 'CFLAGS']:
            env.Append(**{var: [
                '-Werror',
            ]})

    if variant == 'debug':
        for var in ['CXXFLAGS', 'CFLAGS']:
            env.Append(**{var: [
                '-ggdb',
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
else:
    env.Die("CXXFLAGS setup not implemented for compiler '%s'", compiler)

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

    if compiler_ver[:2] < (4, 6):
        for var in ['CXXFLAGS', 'CFLAGS']:
            env.Append(**{var: [
                '-fno-strict-aliasing',
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

    if platform == 'linux':
        if compiler_ver[:2] >= (6, 0):
            for var in ['CXXFLAGS', 'CFLAGS']:
                env.Append(**{var: [
                    '-Wno-redundant-parens',
                ]})
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

if compiler in ['gcc', 'clang']:
    for e in [env, lib_env, tool_env, test_env, pulse_env]:
        for var in ['CXXFLAGS', 'CFLAGS']:
            e.Prepend(**{var:
                [('-isystem', env.Dir(path).path) for path in \
                      e['CPPPATH'] + ['%s/tools' % build_dir]]
                      })

sanitizers = env.ParseSanitizers(GetOption('sanitizers'), supported_sanitizers)
if sanitizers:
    if not compiler in ['gcc', 'clang']:
        env.Die("sanitizers are not supported for compiler '%s'" % compiler)

    for name in sanitizers:
        flags = ['-fsanitize=%s' % name]

        env.AppendUnique(CFLAGS=flags)
        env.AppendUnique(CXXFLAGS=flags)
        env.AppendUnique(LINKFLAGS=flags)
else:
    if platform in ['linux']:
        env.Append(LINKFLAGS=[
            '-Wl,--no-undefined',
        ])

if platform in ['linux']:
    tool_env.Append(LINKFLAGS=[
        '-Wl,-rpath-link,%s' % env.Dir('#3rdparty/%s/rpath' % host).abspath,
    ])

test_env.Append(CPPDEFINES=('CPPUTEST_USE_MEM_LEAK_DETECTION', '0'))

if compiler == 'clang':
    test_env.AppendUnique(CXXFLAGS=[
        '-Wno-weak-vtables',
    ])

env.AlwaysBuild(
    env.Alias('tidy', [env.Dir('#')],
        env.Action(
            "clang-tidy -p %s -checks='%s' -header-filter='src/.*' %s" % (
                build_dir,
                ','.join([
                    '*',
                    '-modernize-*',
                    '-cppcoreguidelines-*',
                    '-google-readability-*',
                    '-hicpp-*',
                    '-fuchsia-*',
                    '-cert-*',
                    '-readability-implicit-bool-conversion',
                    '-readability-inconsistent-declaration-parameter-name',
                    '-readability-named-parameter',
                    '-readability-else-after-return',
                    '-readability-redundant-declaration',
                    '-readability-non-const-parameter',
                    '-google-explicit-constructor',
                    '-google-build-using-namespace',
                    '-google-runtime-int',
                    '-google-runtime-references',
                    '-llvm-include-order',
                    '-llvm-header-guard',
                    '-clang-analyzer-valist.*',
                    '-misc-unconventional-assign-operator',
                    '-misc-macro-parentheses',
                    '-misc-misplaced-widening-cast',
                ]),
                ' '.join(map(str, (env.RecursiveGlob('#src/modules', '*.cpp') +
                                   env.RecursiveGlob('#src/lib', '*.cpp') +
                                   env.RecursiveGlob('#src/tools', '*.cpp'))
                ))
            ),
            env.Pretty('TIDY', 'src', 'yellow')
        )))

Export('env', 'lib_env', 'gen_env', 'tool_env', 'test_env', 'pulse_env')

env.SConscript('src/SConscript',
            variant_dir=build_dir, duplicate=0)
