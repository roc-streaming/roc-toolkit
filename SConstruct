import re
import os
import os.path
import SCons.Script

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

AddOption('--enable-werror',
          dest='enable_werror',
          action='store_true',
          help='enable -Werror compiler option')

AddOption('--disable-api',
          dest='disable_api',
          action='store_true',
          help='disable API building')

AddOption('--disable-tools',
          dest='disable_tools',
          action='store_true',
          help='disable tools building')

AddOption('--disable-tests',
          dest='disable_tests',
          action='store_true',
          help='disable tests building')

AddOption('--disable-doc',
          dest='disable_doc',
          action='store_true',
          help='disable doxygen documentation generation')

AddOption('--disable-sanitizers',
          dest='disable_sanitizers',
          action='store_true',
          help='disable GCC/clang sanitizers')

AddOption('--with-openfec',
          dest='with_openfec',
          choices=['yes', 'no'],
          default='yes',
          help='use OpenFEC for LDPC-Staircase codecs')

AddOption('--with-sox',
          dest='with_sox',
          choices=['yes', 'no'],
          default='yes',
          help='use SoX for audio input/output')

AddOption('--with-3rdparty',
          dest='with_3rdparty',
          action='store',
          type='string',
          help='download and build 3rdparty libraries')

AddOption('--with-targets',
          dest='with_targets',
          action='store',
          type='string',
          help='overwrite targets to use')

if GetOption('help'):
    Return()

if 'clean' in COMMAND_LINE_TARGETS and COMMAND_LINE_TARGETS != ['clean']:
    env.Die("combining 'clean' with other targets is not allowed")

clean = [
    env.DeleteDir('#bin'),
    env.DeleteDir('#build'),
    env.DeleteDir('#doc/doxygen'),
    env.DeleteDir('#3rdparty'),
    env.DeleteDir('#.sconf_temp'),
    env.DeleteFile('#.sconsign.dblite'),
    env.DeleteFile('#config.log'),
]

env.AlwaysBuild(env.Alias('clean', [], clean))

if env.GetOption('clean'):
    env.Execute(clean)
    Return()

if not 'DOXYGEN' in env.Dictionary():
    env['DOXYGEN'] = 'doxygen'

if 'doxygen' in COMMAND_LINE_TARGETS:
    enable_doxygen = True
else:
    enable_doxygen = not GetOption('disable_doc') \
      and not set(COMMAND_LINE_TARGETS).intersection(['tidy', 'fmt']) \
      and env.Which(env['DOXYGEN']) \
      and env.CompilerVersion(env['DOXYGEN'])[:2] >= (1, 6)

if enable_doxygen:
    env.AlwaysBuild(
        env.Alias('doxygen', env.Doxygen(
                'doc/doxygen',
                ['Doxyfile'] + env.RecursiveGlob('#src', ['*.h']),
                werror=GetOption('enable_werror'))))

supported_platforms = [
    'linux',
]

supported_compilers = [
    'gcc',
    'clang',
]

supported_variants = [
    'debug',
    'release',
]

# type of system on which Roc is being compiled, e.g. 'x86_64-pc-linux-gnu'
build = ARGUMENTS.get('build', '')

# type of system on which Roc will run, e.g. 'arm-linux-gnueabihf'
host = ARGUMENTS.get('host', '')

# platform name on which Roc will run, e.g. 'linux'
platform = ARGUMENTS.get('platform', '')

# compiler name, e.g. 'gcc'
compiler = ARGUMENTS.get('compiler', '')

# build variant, e.g. 'debug'
variant = ARGUMENTS.get('variant', 'release')

# build variant for 3rdparty libraries
thirdparty_variant = ARGUMENTS.get('3rdparty_variant', 'release')

# toolchain prefix for compiler, linker, etc. may be equal to 'host' or empty
toolchain = host

for v in [variant, thirdparty_variant]:
    if not variant in supported_variants:
        env.Die("unknown variant '%s', expected one of: %s",
                v, ', '.join(supported_variants))

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

conf = Configure(env, custom_tests=env.CustomTests)

if compiler == 'gcc':
    env['CC'] = 'gcc'
    env['CXX'] = 'g++'
    env['LD'] = 'g++'
    env['AR'] = 'ar'
    env['RANLIB'] = 'ranlib'

if compiler == 'clang':
    env['CC'] = 'clang'
    env['CXX'] = 'clang++'
    env['LD'] = 'clang++'
    env['AR'] = 'llvm-ar'
    env['RANLIB'] = 'llvm-ranlib'

    if compiler_ver[:2] < (3, 6):
        env['RANLIB'] = 'ranlib'

for var in ['CC', 'CXX', 'LD', 'AR', 'RANLIB', 'GENGETOPT', 'DOXYGEN', 'PKG_CONFIG']:
    if var in os.environ:
        env[var] = os.environ[var]

checked = set()

for var in ['CC', 'CXX', 'LD', 'AR', 'RANLIB']:
    if var in os.environ:
        if not env[var] in checked:
            conf.CheckProg(env[var])
    else:
        unversioned = (env[var] in ['ar', 'ranlib'])

        if toolchain:
            env[var] = '%s-%s' % (toolchain, env[var])

        if unversioned:
            continue

        search_versions = [
            compiler_ver[:3],
            compiler_ver[:2],
        ]

        default_ver = env.CompilerVersion(env[var])

        if default_ver and default_ver[:len(compiler_ver)] == compiler_ver:
            search_versions += [default_ver]

        for ver in reversed(sorted(set(search_versions))):
            tool = '%s-%s' % (env[var], '.'.join(map(str, ver)))
            if env.Which(tool):
                env[var] = tool
                break

        if not env[var] in checked:
            conf.CheckProg(env[var])

            actual_ver = env.CompilerVersion(env[var])
            if actual_ver:
                actual_ver = actual_ver[:len(compiler_ver)]

            if actual_ver != compiler_ver:
                env.Die(
                    "can't detect %s: '%s' not found in PATH, '%s' version is %s" % (
                        var,
                        '%s-%s' % (env[var], '.'.join(map(str, compiler_ver))),
                        env[var],
                        actual_ver))

    checked.add(env[var])

for var in ['CFLAGS', 'CXXFLAGS', 'LDFLAGS']:
    if var in os.environ:
        if var == 'LDFLAGS':
            tvar = 'LINKFLAGS'
        else:
            tvar = var
        env.Prepend(**{tvar: os.environ[var]})

env = conf.Finish()

# get full version
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

if not GetOption('with_targets'):
    if not platform:
        env.Die(("can't detect platform for host '%s', looked for one of: %s\nyou should "+
                 "provide either known 'platform' argument or '--with-targets' option"),
                    host, ', '.join(supported_platforms))

    if not platform in supported_platforms:
        env.Die(("unknown platform '%s', expected one of: %s\nyou should "+
                 "provide either known 'platform' argument or '--with-targets' option"),
                    platform, ', '.join(supported_platforms))

crosscompile = (host != build)

build_dir = 'build/%s/%s' % (
    host,
    '-'.join([s for s in [compiler, '.'.join(map(str, compiler_ver)), variant] if s]))

if compiler == 'clang':
    for var in ['CC', 'CXX']:
        env[var] = env.ClangDB(build_dir, '*.cpp', env[var])

env['ROC_BINDIR'] = '#bin/%s' % host
env['ROC_VERSION'] = open(env.File('#.version').path).read().strip()
env['ROC_TARGETS'] = []

fmt = []

clang_format_ver = None
if compiler == 'clang':
    clang_format_ver = compiler_ver
elif env.Which('clang'):
    clang_format_ver = env.CompilerVer('clang')

clang_format_tools = ['clang-format']
if clang_format_ver:
    clang_format_tools += ['clang-format-%s' % '-'.join(map(str, clang_format_ver[:2]))]

clang_format = None
for tool in clang_format_tools:
    if env.Which(tool):
        clang_format = tool

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
]

env.AlwaysBuild(
    env.Alias('fmt', [], fmt))

if set(COMMAND_LINE_TARGETS).intersection(['clean', 'fmt', 'doxygen']):
    Return()

if GetOption('with_targets'):
    for t in GetOption('with_targets').split(','):
        env['ROC_TARGETS'] += ['target_%s' % t]
else:
    if platform in ['linux']:
        env.Append(ROC_TARGETS=[
            'target_posix',
            'target_stdio',
            'target_gnu',
            'target_uv',
        ])

    if GetOption('with_openfec') == 'yes':
        env.Append(ROC_TARGETS=[
            'target_openfec',
        ])

    if GetOption('with_sox') == 'yes':
        env.Append(ROC_TARGETS=[
            'target_sox',
        ])
    else:
        if not GetOption('disable_tools'):
            env.Die("--with-sox=no requires --disable-tools option")

env.Append(CXXFLAGS=[])
env.Append(CPPDEFINES=[])
env.Append(CPPPATH=[])
env.Append(LIBPATH=[])
env.Append(LIBS=[])

alldeps = env['ROC_TARGETS']
getdeps = []

if not GetOption('disable_tools'):
    alldeps += ['target_gengetopt']

if not GetOption('disable_tests'):
    alldeps += ['target_cpputest']

if GetOption('with_3rdparty'):
    getdeps = ['target_%s' % t for t in GetOption('with_3rdparty').split(',')]

    if 'target_all' in getdeps:
        getdeps = alldeps

extdeps = set(alldeps) - set(getdeps)

conf = Configure(env, custom_tests=env.CustomTests)

if 'target_uv' in extdeps:
    env.TryParseConfig('--cflags --libs libuv')

    if not crosscompile:
        if not conf.CheckLibWithHeaderExpr(
            'uv', 'uv.h', 'c', expr='UV_VERSION_MAJOR >= 1 && UV_VERSION_MINOR >= 4'):
            env.Die("libuv >= 1.4 not found (see 'config.log' for details)")
    else:
        if not conf.CheckLibWithHeaderUniq('uv', 'uv.h', 'c'):
            env.Die("libuv not found (see 'config.log' for details)")

if 'target_openfec' in extdeps:
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

if 'target_sox' in extdeps:
    env.TryParseConfig('--cflags --libs sox')

    if not crosscompile:
        if not conf.CheckLibWithHeaderExpr(
                'sox', 'sox.h', 'c',
                expr='SOX_LIB_VERSION_CODE >= SOX_LIB_VERSION(14, 4, 0)'):
            env.Die("libsox >= 14.4.0 not found (see 'config.log' for details)")
    else:
        if not conf.CheckLibWithHeaderUniq('sox', 'sox.h', 'c'):
            env.Die("libsox not found (see 'config.log' for details)")

if 'target_gengetopt' in extdeps:
    if 'GENGETOPT' in env.Dictionary():
        gengetopt = env['GENGETOPT']
    else:
        gengetopt = 'gengetopt'

    if not conf.CheckProg(gengetopt):
        env.Die("gengetopt not found in PATH (looked for '%s')" % gengetopt)

env = conf.Finish()

test_env = env.Clone()
test_conf = Configure(test_env, custom_tests=test_env.CustomTests)

if 'target_cpputest' in extdeps:
    test_env.TryParseConfig('--cflags --libs cpputest')

    if not test_conf.CheckLibWithHeaderUniq('CppUTest', 'CppUTest/TestHarness.h', 'cxx'):
        test_env.Die("CppUTest not found (see 'config.log' for details)")

test_env = test_conf.Finish()

if 'target_uv' in getdeps:
    env.ThirdParty(host, toolchain, thirdparty_variant, 'uv-1.4.2')

if 'target_openfec' in getdeps:
    env.ThirdParty(host, toolchain, thirdparty_variant, 'openfec-1.4.2', includes=[
        'lib_common',
        'lib_stable',
    ])

if 'target_sox' in getdeps:
    env.ThirdParty(host, toolchain, thirdparty_variant, 'alsa-1.0.29')
    env.ThirdParty(host, toolchain, thirdparty_variant, 'sox-14.4.2', deps=['alsa-1.0.29'])

    conf = Configure(env, custom_tests=env.CustomTests)

    for lib in [
            'z', 'ltdl', 'magic',
            'sndfile', 'gsm', 'FLAC',
            'vorbis', 'vorbisenc', 'vorbisfile', 'ogg',
            'mad', 'mp3lame',
            'pulse', 'pulse-simple']:
        conf.CheckLib(lib)

    env = conf.Finish()

if 'target_gengetopt' in getdeps:
    env.ThirdParty(build, "", thirdparty_variant, 'gengetopt-2.22.6')

    env['GENGETOPT'] = env.File(
        '#3rdparty/%s/build/gengetopt-2.22.6/bin/gengetopt' % build + env['PROGSUFFIX'])

if 'target_cpputest' in getdeps:
    test_env.ThirdParty(host, toolchain, thirdparty_variant, 'cpputest-3.6')

if 'target_posix' in env['ROC_TARGETS']:
    env.Append(CPPDEFINES=[('_POSIX_C_SOURCE', '200809')])

for t in env['ROC_TARGETS']:
    env.Append(CPPDEFINES=['ROC_' + t.upper()])

env.Append(LIBPATH=['#%s' % build_dir])

if platform in ['linux']:
    env.AppendUnique(LIBS=['rt', 'dl'])

if compiler in ['gcc', 'clang']:
    env.Append(CXXFLAGS=[
        '-std=c++98',
        '-fno-exceptions',
        '-fPIC',
    ])
    env.Append(LINKFLAGS=[
        '-pthread',
    ])
    if not(compiler == 'clang' and variant == 'debug'):
        env.Append(CXXFLAGS=[
            '-fno-rtti',
        ])
    if GetOption('enable_werror'):
        env.Append(CXXFLAGS=[
            '-Werror'
        ])
    if variant == 'debug':
        env.Append(CXXFLAGS=['-ggdb'])
        env.Append(LINKFLAGS=['-rdynamic'])
    else:
        env.Append(CXXFLAGS=['-O2'])
else:
    env.Die("CXXFLAGS setup not implemented for compiler '%s'", compiler)

if compiler == 'gcc':
    env.Append(CXXFLAGS=[
        '-Wall',
        '-Wextra',
        '-Wshadow',
        '-Wcast-qual',
        '-Wfloat-equal',
        '-Wpointer-arith',
        '-Wformat=2',
        '-Wformat-security',
        '-Wstrict-null-sentinel',
        '-Wctor-dtor-privacy',
        '-Wnon-virtual-dtor',
        '-Wno-invalid-offsetof',
        '-Wno-system-headers',
    ])

    if compiler_ver[:2] >= (4, 4):
        env.Append(CXXFLAGS=[
            '-Wlogical-op',
            '-Wmissing-declarations',
            '-Woverlength-strings',
        ])

    if compiler_ver[:2] >= (4, 8):
        env.Append(CXXFLAGS=[
            '-Wdouble-promotion',
            '-Wabi',
        ])

    if compiler_ver[:2] < (4, 6):
        env.Append(CXXFLAGS=[
            '-fno-strict-aliasing',
        ])

if compiler == 'clang':
    env.Append(CXXFLAGS=[
        '-Weverything',
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
        '-Wno-system-headers',
    ])

    if compiler_ver[:2] >= (3, 6):
        env.Append(CXXFLAGS=[
            '-Wno-reserved-id-macro',
        ])
    else:
        env.Append(CXXFLAGS=[
            '-Wno-unreachable-code',
        ])

if compiler in ['gcc', 'clang']:
    if not GetOption('disable_sanitizers') \
      and not crosscompile \
      and variant == 'debug' and (
        (compiler == 'gcc' and compiler_ver[:2] >= (4, 9)) or
        (compiler == 'clang' and compiler_ver[:2] >= (3, 6))):

        san_env = env.Clone()
        san_conf = Configure(san_env, custom_tests=env.CustomTests)

        flags = [
            '-fsanitize=undefined',
        ]

        san_env.Append(CFLAGS=flags)
        san_env.Append(CXXFLAGS=flags)
        san_env.Append(LINKFLAGS=flags)

        if san_conf.CheckLib('ubsan'):
            env.Append(LIBS=['ubsan'])
            env.Append(CFLAGS=flags)
            env.Append(CXXFLAGS=flags)
            env.Append(LINKFLAGS=flags)

        san_conf.Finish()

    env.Prepend(
        CXXFLAGS=[('-isystem', env.Dir(path).path) for path in \
                  env['CPPPATH'] + ['%s/tools' % build_dir]])

if not GetOption('disable_tests'):
    for var in ['CXXFLAGS', 'CPPDEFINES', 'CPPPATH', 'LIBPATH', 'LIBS']:
        env['TEST_' + var] = [p for p in test_env[var] if not p in env[var]]

    env['TEST_CPPDEFINES'] += [('CPPUTEST_USE_MEM_LEAK_DETECTION', '0')]

    if compiler in ['gcc', 'clang']:
        env.Prepend(
            TEST_CXXFLAGS=[
                ('-isystem', env.Dir(path).path) for path in env['TEST_CPPPATH']])

    if compiler == 'clang':
        env.AppendUnique(TEST_CXXFLAGS=[
            '-Wno-weak-vtables',
        ])

env.AlwaysBuild(
    env.Alias('tidy', [env.Dir('#')],
        env.Action(
            "clang-tidy -p %s -checks='%s' -header-filter='src/.*' %s" % (
                build_dir,
                ','.join([
                    '*',
                    '-readability-named-parameter',
                    '-readability-else-after-return',
                    '-google-readability-todo',
                    '-google-readability-function',
                    '-google-readability-casting',
                    '-google-explicit-constructor',
                    '-google-build-using-namespace',
                    '-google-runtime-int',
                    '-llvm-include-order',
                    '-llvm-header-guard',
                    '-misc-macro-parentheses',
                    '-misc-use-override',
                    '-clang-analyzer-alpha.core.CastToStruct',
                    '-clang-analyzer-alpha.security.ReturnPtrRange',
                ]),
                ' '.join(map(str,
                             env.RecursiveGlob('#src', '*.cpp')
                ))
            ),
            env.Pretty('TIDY', 'src', 'yellow')
        )))

Export('env')

env.SConscript('src/SConscript',
            variant_dir=build_dir, duplicate=0)
