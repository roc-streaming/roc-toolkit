import re
import os
import os.path
import platform

env = Environment(ENV=os.environ, tools=[
    'default',
    'roc',
])

AddOption('--enable-werror',
          dest='werror',
          action='store_true',
          help='enable -Werror compiler option')

AddOption('--with-doxygen',
          dest='with_doxygen',
          choices=['yes', 'no'],
          default='yes' if env.Which('doxygen') else 'no',
          help='enable doxygen documentation generation')

AddOption('--with-openfec',
          dest='with_openfec',
          choices=['yes', 'no'],
          default='yes',
          help='enable OpenFEC implementation for LDPC-Staircase codecs')

supported_platforms = [
    'linux',
]

supported_archs = [
    'x86_64',
]

supported_compilers = [
    'gcc',
    'clang',
]

supported_variants = [
    'debug',
    'release',
]

host = '%s_%s' % (
    platform.system().lower(), platform.machine().lower())

target = ARGUMENTS.get('target', host)
variant = ARGUMENTS.get('variant', 'debug')
toolchain = ARGUMENTS.get('toolchain', '')
compiler = ARGUMENTS.get('compiler', '')

target_platform, target_arch = target.split('_', 1)

if not target_platform in supported_platforms:
    env.Die("unknown target platform `%s' in `%s', expected on of: %s",
            target_platform, target, ', '.join(supported_platforms))

if not target_arch in supported_archs:
    env.Die("unknown target arch `%s' in `%s', expected on of: %s",
            target_arch, target, ', '.join(supported_archs))

if not variant in supported_variants:
    env.Die("unknown variant `%s', expected on of: %s",
            variant, ', '.join(supported_variants))

if not compiler:
    if host == target and env.Which('clang'):
        compiler = 'clang'
    else:
        compiler = 'gcc'

if not compiler in supported_compilers:
    env.Die("unknown compiler `%s', expected on of: %s",
            compiler, ', '.join(supported_compilers))

if not toolchain:
    if host != target:
        env.Die("toolchain option is required when cross-compiling")

build_dir = 'build/%s/%s' % (
    '-'.join([s for s in [target, toolchain, compiler] if s]),
    variant)

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

    clang_ver = env.CompilerVersion(env['CC'])[:2]

    for var in ['AR', 'RANLIB']:
        versioned = '%s-%s' % (env[var], '.'.join(map(str, clang_ver)))

        if env.Which(versioned):
            env[var] = versioned

    if clang_ver < (3, 6):
        env['RANLIB'] = 'ranlib'

if toolchain:
    for var in ['CC', 'CXX', 'LD', 'AR', 'RANLIB']:
        env[var] = '%s-%s' % (toolchain, env[var])

if compiler == 'clang':
    for var in ['CC', 'CXX']:
        env[var] = env.ClangDB(build_dir, '*.cpp', env[var])

env['ROC_VERSION'] = '0.1'
env['ROC_TARGETS'] = []

if target_platform in ['linux']:
    env.Append(ROC_TARGETS=[
        'target_posix',
        'target_stdio',
        'target_gnu',
        'target_uv',
        'target_sox',
    ])

if GetOption('with_openfec') == 'yes':
    env.Append(ROC_TARGETS=[
        'target_openfec',
    ])

if 'target_posix' in env['ROC_TARGETS']:
    env.Append(CPPDEFINES=[('_POSIX_C_SOURCE', '200809')])

env.Append(LIBS=[])

if 'target_uv' in env['ROC_TARGETS']:
    env.Append(LIBS=[
        'uv'
    ])

if 'target_sox' in env['ROC_TARGETS']:
    env.Append(LIBS=[
        'sox'
    ])

if 'target_openfec' in env['ROC_TARGETS']:
    env.Append(CPPDEFINES=[
        'OF_USE_ENCODER',
        'OF_USE_DECODER',
        'OF_USE_LDPC_STAIRCASE_CODEC',
    ])
    env.Append(CPPPATH=[
        '/usr/include/openfec/lib_common',
        '/usr/include/openfec/lib_stable',
    ])
    env.Append(LIBS=[
        'openfec',
    ])

for t in env['ROC_TARGETS']:
    env.Append(CPPDEFINES=['ROC_' + t.upper()])

env.Append(LIBPATH=['#bin'])
env.Append(CPPPATH=[])

if compiler in ['gcc', 'clang']:
    env.Append(CXXFLAGS=[
        '-std=c++98',
        '-fno-exceptions',
    ])
    if not(compiler == 'clang' and variant == 'debug'):
        env.Append(CXXFLAGS=[
            '-fno-rtti',
        ])
    if GetOption('werror'):
        env.Append(CXXFLAGS=[
            '-Werror'
        ])
    if target_platform in ['linux']:
        env.Append(LINKFLAGS=[
            '-pthread',
        ])
    if variant == 'debug':
        env.Append(CXXFLAGS=['-ggdb'])
        env.Append(LINKFLAGS=['-rdynamic'])
    else:
        env.Append(CXXFLAGS=['-O2'])
else:
    env.Die("CXXFLAGS setup not implemented for compiler `%s'", compiler)

if compiler == 'gcc':
    env.Append(CXXFLAGS=[
        '-Wall',
        '-Wextra',
        '-Wabi',
        '-Winit-self',
        '-Wshadow',
        '-Wcast-qual',
        '-Wcast-align',
        '-Wfloat-equal',
        '-Wpointer-arith',
        '-Wformat=2',
        '-Wformat-security',
        '-Wstrict-null-sentinel',
        '-Wlogical-op',
        '-Wmissing-declarations',
        '-Woverlength-strings',
        '-Wctor-dtor-privacy',
        '-Wnon-virtual-dtor',
        '-Wno-invalid-offsetof',
        '-Wno-system-headers',
    ])

    if env.CompilerVersion(env['CC']) >= (4, 8):
        env.Append(CXXFLAGS=[
            '-Wdouble-promotion',
        ])

        if variant == 'debug':
            flags = [
                '-fsanitize=undefined',
            ]

            env.Append(CFLAGS=flags)
            env.Append(CXXFLAGS=flags)
            env.Append(LINKFLAGS=flags)

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

    if clang_ver >= (3, 6):
        env.Append(CXXFLAGS=[
            '-Wno-reserved-id-macro',
        ])
    else:
        env.Append(CXXFLAGS=[
            '-Wno-unreachable-code',
        ])

if compiler in ['gcc', 'clang']:
    env.Prepend(
        CXXFLAGS=[('-isystem', path) for path in \
                  env['CPPPATH'] + ['%s/tools' % build_dir]])

env['TEST_CPPDEFINES'] = [('CPPUTEST_USE_MEM_LEAK_DETECTION', '0')]
env['TEST_CXXFLAGS'] = []
env['TEST_LIBS'] = ['CppUTest']

if compiler == 'clang':
    env.Append(TEST_CXXFLAGS=[
        '-Wno-weak-vtables',
    ])

env.AlwaysBuild(
    env.Alias('clean', [], [
        env.DeleteDir('#bin'),
        env.DeleteDir('#build'),
        env.DeleteDir('#doc/doxygen'),
    ]))

env.AlwaysBuild(
    env.Alias('fmt', [], [
        env.Action(
            'clang-format -i %s' % ' '.join(map(str,
                env.RecursiveGlob(
                    '#src', ['*.h', '*.cpp'],
                    exclude=open(env.File('#.fmtignore').path).read().split())
            )),
            env.Pretty('FMT', 'src', 'yellow')
        ),
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
    ]))

env.AlwaysBuild(
    env.Alias('tidy', [env.Dir('#')],
        env.Action(
            "clang-tidy -p %s -checks='%s' -header-filter='src/.*' %s" % (
                build_dir,
                ','.join([
                    '*',
                    '-google-readability-todo',
                    '-google-readability-function',
                    '-google-readability-casting',
                    '-google-explicit-constructor',
                    '-google-build-using-namespace',
                    '-google-runtime-int',
                    '-llvm-include-order',
                    '-llvm-header-guard',
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

if 'doxygen' in COMMAND_LINE_TARGETS or (
        GetOption('with_doxygen') == 'yes' and not set(COMMAND_LINE_TARGETS).intersection(
            ['tidy'])):
        env.AlwaysBuild(
            env.Alias('doxygen', env.Doxygen(
                    'doc/doxygen',
                    ['Doxyfile'] + env.RecursiveGlob('#src', ['*.h']))))

# performance tuning
env.Decider('MD5-timestamp')
env.SetOption('implicit_cache', 1)
env.SourceCode('.', None)

# provide absolute path to force single sconsign file
# per-directory sconsign files seems to be buggy with generated sources
env.SConsignFile(os.path.join(env.Dir('#').abspath, '.sconsign.dblite'))

Export('env')

if 'clean' in COMMAND_LINE_TARGETS:
    if COMMAND_LINE_TARGETS != ['clean']:
        env.Die("combining 'clean' with other targets is not supported")

if not set(COMMAND_LINE_TARGETS).intersection(['clean', 'fmt', 'doxygen']):
    env.SConscript('src/SConscript',
                variant_dir=build_dir,
                duplicate=0)
