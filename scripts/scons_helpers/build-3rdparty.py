from __future__ import print_function

import argparse
import doctest
import fileinput
import fnmatch
import glob
import multiprocessing
import os
import os.path
import re
import shutil
import ssl
import subprocess
import sys
import tarfile
import textwrap

try:
    from shlex import quote
except:
    from pipes import quote

try:
    from urllib.request import urlopen
except ImportError:
    from urllib2 import urlopen

try:
    # workaround for SSL certificate error
    ssl._create_default_https_context = ssl._create_unverified_context
except:
    pass

DEV_NULL = open(os.devnull, 'w+')

Context = type(
    'Context', (),
    {
        field: '' for field in (
            'root_dir work_dir dist_dir log_file commit_file'
            ' pkg_dir pkg_src_dir pkg_bin_dir pkg_lib_dir pkg_inc_dir pkg_rpath_dir'
            ' pkg pkg_name pkg_ver pkg_ver_major pkg_ver_minor pkg_ver_patch pkg_deps'
            ' build host toolchain variant android_platform macos_platform macos_arch'
            ' prefer_cmake'
            ' env unparsed_env').split()
    })

#
# Helpers.
#

def changedir(ctx, dir_path):
    # format arguments
    dir_path = dir_path.format(ctx=ctx)

    with open(ctx.log_file, 'a+') as fp:
        print('>>> cd ' + dir_path, file=fp)

    os.chdir(dir_path)

def download(ctx, url, out):
    def _fetch_vendored(url, path):
        for subdir, _, _ in os.walk(ctx.dist_dir):
            distfile = os.path.join(subdir, os.path.basename(path))
            if os.path.exists(distfile):
                msg('[found vendored] {}', os.path.basename(distfile))
                with open(distfile, 'rb') as rp:
                    with open(path, 'wb') as wp:
                        wp.write(rp.read())
                        return
        raise

    def _fetch_urlopen(url, path):
        rp = urlopen(url)
        with open(path, 'wb') as wp:
            wp.write(rp.read())

    def _fetch_tool(url, path, tool, cmd):
        msg('[trying {}] {}', tool, url)
        with open(ctx.log_file, 'a+') as fp:
            print('>>> ' + cmd, file=fp)
        if os.system(cmd) != 0:
            raise

    def _fetch_wget(url, path):
        _fetch_tool(url, path, 'wget',
                    'wget "{}" --quiet -O "{}"'.format(url, path))

    def _fetch_curl(url, path):
        _fetch_tool(url, path, 'curl',
                    'curl -Ls "{}" -o "{}"'.format(url, path))

    # format arguments
    url = url.format(ctx=ctx)
    out = out.format(ctx=ctx)

    path_res = 'src/' + out
    path_tmp = 'tmp/' + out

    if os.path.exists(path_res):
        msg('[found downloaded] {}', out)
        return

    rmpath(path_res)
    rmpath(path_tmp)
    mkpath('tmp')

    error = None
    for fn in [_fetch_vendored, _fetch_urlopen, _fetch_curl, _fetch_wget]:
        try:
            fn(url, path_tmp)
            shutil.move(path_tmp, path_res)
            rm_emptydir('tmp')
            return
        except Exception as e:
            if fn == _fetch_vendored:
                msg('[download] {}', url)
            if fn == _fetch_urlopen:
                error = e

    die("can't download '{}': {}", url, error)

def unpack(ctx, filename, dirname):
    # format arguments
    filename = filename.format(ctx=ctx)
    dirname = dirname.format(ctx=ctx)

    dirname_res = 'src/' + dirname
    dirname_tmp = 'tmp/' + dirname

    if os.path.exists(dirname_res):
        msg('[found unpacked] {}', dirname)
        return

    msg('[unpack] {}', filename)

    rmpath(dirname_res)
    rmpath(dirname_tmp)
    mkpath('tmp')

    tar = tarfile.open('src/'+filename, 'r')
    try:
        tar.extractall('tmp', filter='data')
    except:
        tar.extractall('tmp')
    tar.close()

    shutil.move(dirname_tmp, dirname_res)
    rm_emptydir('tmp')

def execute(ctx, cmd, ignore_error=False, clear_env=False):
    msg('[execute] {}', cmd)

    with open(ctx.log_file, 'a+') as fp:
        print('>>> ' + cmd, file=fp)

    env = None
    if clear_env:
        env = {'HOME': os.environ['HOME'], 'PATH': os.environ['PATH']}

    code = subprocess.call('{} >>{} 2>&1'.format(cmd, ctx.log_file), shell=True, env=env)
    if code != 0:
        if ignore_error:
            with open(ctx.log_file, 'a+') as fp:
                print('command exited with status {}'.format(code), file=fp)
        else:
            exit(1)

def execute_make(ctx, cpu_count=None):
    if cpu_count is None:
        cpu_count = detect_cpu_count()

    cmd = ['make']
    if cpu_count:
        cmd += ['-j' + str(cpu_count)]

    execute(ctx, ' '.join(cmd))

def execute_cmake(ctx, src_dir, args=None):
    def _getvar(var, default):
        if var in ctx.env:
            return ctx.env[var]
        return '-'.join([s for s in [ctx.toolchain, default] if s])

    # format arguments
    src_dir = src_dir.format(ctx=ctx)

    if not args:
        args = []

    compiler = _getvar('CC', 'gcc')
    sysroot = find_sysroot(ctx.toolchain, compiler)

    need_sysroot = bool(sysroot)
    need_tools = True

    # cross-compiling for yocto linux
    if 'OE_CMAKE_TOOLCHAIN_FILE' in os.environ:
        need_tools = False

    # building for macOS
    if ctx.macos_platform:
        args += [
            '-DCMAKE_OSX_DEPLOYMENT_TARGET=' + quote(ctx.macos_platform),
        ]

        if ctx.macos_arch:
            args += [
                '-DCMAKE_OSX_ARCHITECTURES=' + quote(';'.join(ctx.macos_arch)),
            ]

    # cross-compiling for Android
    if ctx.android_platform:
        args += [
            '-DCMAKE_SYSTEM_NAME=Android',
            '-DANDROID_PLATFORM=android-' + ctx.android_platform,
        ]

        api = detect_android_api(compiler)
        abi = detect_android_abi(ctx.toolchain)

        toolchain_file = find_android_toolchain_file(compiler)

        if toolchain_file:
            need_sysroot = False
            need_tools = False
            args += [quote('-DCMAKE_TOOLCHAIN_FILE=' + toolchain_file)]
            if api:
                args += ['-DANDROID_NATIVE_API_LEVEL=' + api]
            if abi:
                args += ['-DANDROID_ABI=' + abi]
        else:
            sysroot = find_android_sysroot(compiler)
            need_sysroot = bool(sysroot)
            need_tools = True
            if api:
                args += ['-DCMAKE_SYSTEM_VERSION=' + api]
            if abi:
                args += ['-DCMAKE_ANDROID_ARCH_ABI=' + abi]

    if need_sysroot:
        args += [
            '-DCMAKE_FIND_ROOT_PATH=' + quote(sysroot),
            '-DCMAKE_SYSROOT='        + quote(sysroot),
        ]

    if need_tools:
        if not ctx.android_platform:
            args += [
                '-DCMAKE_C_COMPILER=' + quote(find_tool(compiler)),
            ]
        args += [
            '-DCMAKE_LINKER=' + quote(find_tool(_getvar('CCLD', 'gcc'))),
            '-DCMAKE_AR='     + quote(find_tool(_getvar('AR', 'ar'))),
            '-DCMAKE_RANLIB=' + quote(find_tool(_getvar('RANLIB', 'ranlib'))),
        ]

    if ctx.env.get('COMPILER_LAUNCHER', None):
        args += [
            '-DCMAKE_CXX_COMPILER_LAUNCHER=' + quote(ctx.env['COMPILER_LAUNCHER']),
            '-DCMAKE_C_COMPILER_LAUNCHER=' + quote(ctx.env['COMPILER_LAUNCHER']),
        ]

    args += [
        '-DCMAKE_POSITION_INDEPENDENT_CODE=ON',
    ]

    cc_flags = [
        '-fPIC', # -fPIC should be set explicitly in older cmake versions
    ]

    if ctx.variant == 'debug':
        cc_flags += [
            '-ggdb', # -ggdb is required for sanitizer backtrace
        ]
        args += [
            '-DCMAKE_BUILD_TYPE=Debug',
            '-DCMAKE_C_FLAGS_DEBUG:STRING=' + quote(' '.join(cc_flags)),
        ]
    else:
        args += [
            '-DCMAKE_BUILD_TYPE=Release',
            '-DCMAKE_C_FLAGS_RELEASE:STRING=' + quote(' '.join(cc_flags)),
        ]

    execute(ctx, 'cmake ' + src_dir + ' ' + ' '.join(args))

def execute_cmake_build(ctx):
    cmd = ['cmake', '--build', '.']

    # assume we're using -G"Unix Makefiles"
    cmd += ['--', 'VERBOSE=1']
    cpu_count = detect_cpu_count()
    if cpu_count:
        cmd += ['-j' + str(cpu_count)]

    execute(ctx, ' '.join(cmd))

def install_tree(ctx, src, dst, include=None, exclude=None):
    # format arguments
    src = src.format(ctx=ctx)
    dst = dst.format(ctx=ctx)

    msg('[install] {}', os.path.relpath(dst, ctx.root_dir))

    def _match_patterns(src, names):
        ignore_names = []
        for n in names:
            if os.path.isdir(os.path.join(src, n)):
                continue
            matched = False
            for m in include:
                if fnmatch.fnmatch(n, m):
                    matched = True
                    break
            if not matched:
                ignore_names.append(n)
        return set(ignore_names)

    if include:
        ignore_fn = _match_patterns
    elif exclude:
        ignore_fn = shutil.ignore_patterns(*exclude)
    else:
        ignore_fn = None

    mkpath(os.path.dirname(dst))
    rmpath(dst)
    shutil.copytree(src, dst, ignore=ignore_fn)

def install_files(ctx, src, dst):
    # format arguments
    src = src.format(ctx=ctx)
    dst = dst.format(ctx=ctx)

    msg('[install] {}', os.path.join(
        os.path.relpath(dst, ctx.root_dir),
        os.path.basename(src)))

    for f in glob.glob(src):
        mkpath(dst)
        shutil.copy(f, dst)

def subst_tree(ctx, dir_path, file_patterns, from_, to):
    def _match_path(path):
        try:
            with open(path) as fp:
                for line in fp:
                    if from_ in line:
                        return True
        except:
            pass

    # format arguments
    dir_path = dir_path.format(ctx=ctx)

    for filepat in file_patterns:
        for root, dirnames, filenames in os.walk(dir_path):
            for filename in fnmatch.filter(filenames, filepat):
                filepath = os.path.join(root, filename)
                if _match_path(filepath):
                    subst_files(ctx, filepath, from_, to)

def subst_files(ctx, file_path, from_, to):
    # format arguments
    file_path = file_path.format(ctx=ctx)

    msg('[patch] {}', file_path)

    for line in fileinput.input(file_path, inplace=True):
        print(line.replace(from_, to), end='')

def apply_patch(ctx, dir_path, patch_url, patch_name):
    # format arguments
    dir_path = dir_path.format(ctx=ctx)

    if subprocess.call(
        'patch --version', stdout=DEV_NULL, stderr=DEV_NULL, shell=True) != 0:
        return

    download(ctx, patch_url, patch_name)
    execute(ctx, 'patch -p1 -N -d {} -i {}'.format(
        dir_path,
        '../' + patch_name), ignore_error=True)

def format_vars(ctx, disable_launcher=False, env=None):
    ret = []
    if not env:
        env = ctx.env
    for k, v in env.items():
        if v is None:
            continue
        if k == 'COMPILER_LAUNCHER':
            continue
        elif k in ['CC', 'CXX'] and not disable_launcher:
            if ctx.env.get('COMPILER_LAUNCHER', None):
                ret.append(quote('{}={} {}'.format(k, ctx.env['COMPILER_LAUNCHER'], v)))
            else:
                ret.append(quote('{}={}'.format(k, v)))
        else:
            ret.append(quote('{}={}'.format(k, v)))
    return ' '.join(ret)

def format_flags(ctx, cflags='', ldflags='', pthread=False):
    inc_dirs = []
    lib_dirs = []
    rpath_dirs = []

    for dep in ctx.pkg_deps:
        inc_dirs += [os.path.join(ctx.work_dir, dep, 'include')]
        lib_dirs += [os.path.join(ctx.work_dir, dep, 'lib')]
        rpath_dirs += [os.path.join(ctx.work_dir, dep, 'rpath')]

    cflags = ([cflags] if cflags else []) + ['-I' + path for path in inc_dirs]
    ldflags = ['-L' + path for path in lib_dirs] + ([ldflags] if ldflags else [])

    is_gnu = detect_compiler_family(ctx.env, ctx.toolchain, 'gcc')
    is_clang = detect_compiler_family(ctx.env, ctx.toolchain, 'clang')

    if ctx.variant == 'debug':
        if is_gnu or is_clang:
            cflags += ['-ggdb']
        else:
            cflags += ['-g']
    elif ctx.variant == 'release':
        cflags += ['-O2']

    if pthread and not ctx.android_platform:
        if is_gnu or is_clang:
            cflags += ['-pthread']
        if is_gnu:
            ldflags += ['-pthread']
        else:
            ldflags += ['-lpthread']

    if is_gnu:
        ldflags += ['-Wl,-rpath-link=' + path for path in rpath_dirs]

    if ctx.macos_platform:
        cflags += ['-mmacosx-version-min=' + ctx.macos_platform]
        ldflags += ['-mmacosx-version-min=' + ctx.macos_platform]

        for arch in ctx.macos_arch:
            cflags += ['-arch', arch]
            ldflags += ['-arch', arch]

    return ' '.join([
        'CXXFLAGS=' + quote(' '.join(cflags)),
        'CFLAGS='   + quote(' '.join(cflags)),
        'LDFLAGS='  + quote(' '.join(ldflags)),
    ])

# https://mesonbuild.com/Reference-tables.html
def generate_meson_crossfile(ctx, pc_dir, cross_file):
    def _meson_string(s):
        m = {
            "\\": "\\\\",
            "'":  "\\'",
            "\a": "\\a",
            "\b": "\\b",
            "\f": "\\f",
            "\n": "\\n",
            "\r": "\\r",
            "\t": "\\t",
            "\v": "\\v",
        }
        for k, v in m.items():
            s = s.replace(k, v)
        return "'" + s + "'"

    def _meson_list(l):
        return '[{}]'.format(', '.join(map(_meson_string, l)))

    def _meson_compiler(compiler, launcher):
        if launcher:
            return _meson_list([launcher, compiler])
        else:
            return _meson_string(compiler)

    msg('[generate] {}', cross_file)

    pkg_config = None
    if 'PKG_CONFIG' in ctx.env:
        pkg_config = ctx.env['PKG_CONFIG']
    elif which('pkg-config'):
        pkg_config = 'pkg-config'

    cflags = []
    ldflags = []

    for dep in ctx.pkg_deps:
        cflags.append('-I' + os.path.join(ctx.work_dir, dep, 'include'))
        ldflags.append('-L' + os.path.join(ctx.work_dir, dep, 'lib'))

    if ctx.toolchain:
        system = 'linux'
        cpu_family = 'x86_64'
        cpu = 'x86_64'
        endian = 'little'

        system_list = ('android cygwin darwin dragonfly emscripten freebsd haiku netbsd'+
                   ' openbsd windows sunos').split()

        cpu_list = ('aarch64 alpha arc arm avr c2000 csky dspic e2k ft32 ia64'+
                    ' loongarch64 m68k microblaze mips mips64 msp430 parisc pic24'+
                    ' ppc ppc64 riscv32 riscv64 rl78 rx s390 s390x sh4 sparc sparc64'+
                    ' wasm32 wasm64 x86 x86_64').split()

        for s in reversed(sorted(system_list, key=len)):
            if s in ctx.toolchain:
                system = s
                break

        for s in reversed(sorted(cpu_list, key=len)):
            if s in ctx.toolchain:
                cpu_family = s

                if s == 'aarch64':
                    cpu = 'armv8'
                elif s == 'arm':
                    cpu = 'armv5'
                elif s == 'x86':
                    cpu = 'i686'
                else:
                    cpu = s

                if s+'le' in ctx.toolchain or s+'el' in ctx.toolchain:
                    endian = 'little'
                elif s+'be' in ctx.toolchain or s+'eb' in ctx.toolchain:
                    endian = 'big'
                elif s in 'wasm32 wasm64 x86 x86_64'.split():
                    endian = 'little'
                else:
                    endian = 'big'

                break

    with open(cross_file, 'w') as fp:
        fp.write(textwrap.dedent("""\
                [binaries]
                c = {cc}
                cpp = {cxx}
                ar = {ar}
        """).format(
            cc=_meson_compiler(ctx.env['CC'],
                ctx.env.get('COMPILER_LAUNCHER', None)),
            cxx=_meson_compiler(ctx.env['CXX'],
                ctx.env.get('COMPILER_LAUNCHER', None)),
            ar=_meson_string(ctx.env['AR'])))

        if pkg_config:
            fp.write(textwrap.dedent("""\
                pkgconfig = {pkg_config}
            """).format(
                pkg_config=_meson_string(pkg_config)))

        if pc_dir:
            fp.write(textwrap.dedent("""\

                [built-in options]
                pkg_config_path = {pc_dir}
            """).format(
                pc_dir=_meson_string(os.path.abspath(pc_dir))))

        if cflags or ldflags:
            fp.write(textwrap.dedent("""\

                [properties]
                c_args = {cflags}
                c_link_args = {ldflags}
                cpp_args = {cflags}
                cpp_link_args = {ldflags}
            """).format(
                cflags=_meson_list(cflags),
                ldflags=_meson_list(ldflags)))

        if ctx.toolchain:
            fp.write(textwrap.dedent("""\

                [host_machine]
                system = {system}
                cpu_family = {cpu_family}
                cpu = {cpu}
                endian = {endian}
            """).format(
                system=_meson_string(system),
                cpu_family=_meson_string(cpu_family),
                cpu=_meson_string(cpu),
                endian=_meson_string(endian)))

# Generate temporary .pc files for pkg-config.
def generate_pc_files(ctx, pc_dir):
    for dep in ctx.pkg_deps:
        name, ver = parse_dep(dep)
        pc_file = os.path.join(pc_dir, name) + '.pc'

        with open(pc_file, 'w') as fp:
            msg('[generate] {}', pc_file)

            cflags = '-I' + os.path.join(ctx.work_dir, dep, 'include')
            ldflags = '-L' + os.path.join(ctx.work_dir, dep, 'lib')

            for lib in glob.glob(os.path.join(ctx.work_dir, dep, 'lib', 'lib*')):
                lib = os.path.basename(lib)
                lib = re.sub('^lib', '', lib)
                lib = re.sub(r'\.[a-z]+$', '', lib)

                ldflags += ' -l' + lib

            fp.write(textwrap.dedent("""\
                Name: {name}
                Description: {name}
                Version: {ver}
                Cflags: {cflags}
                Libs: {ldflags}
            """).format(
                name=name,
                ver=ver,
                cflags=cflags,
                ldflags=ldflags))

def find_tool(tool):
    if '/' in tool:
        return tool

    path = which(tool)
    if not path:
        return tool

    return path

def find_sysroot(toolchain, compiler):
    if not toolchain:
        return ''

    if not compiler:
        compiler = '{}-gcc'.format(toolchain)

    try:
        sysroot = run_command([compiler, '-print-sysroot'])
        if os.path.isdir(sysroot):
            return sysroot
    except:
        pass

    return None

def find_android_sysroot(compiler):
    compiler_exe = which(compiler)
    if compiler_exe:
        return find_file_upwards(compiler_exe, 'sysroot')

def find_android_toolchain_file(compiler):
    compiler_exe = which(compiler)
    if compiler_exe:
        return find_file_upwards(compiler_exe, 'build/cmake/android.toolchain.cmake')

def detect_android_abi(toolchain):
    try:
        arch = toolchain.split('-')[0]
    except:
        return
    if arch == 'arm' or arch == 'armv7a':
        return 'armeabi-v7a'
    if arch == 'aarch64':
        return 'arm64-v8a'
    if arch == 'i686':
        return 'x86'
    if arch == 'x86_64':
        return 'x86_64'
    return arch

def detect_android_api(compiler):
    try:
        for line in run_command([compiler, '-dM', '-E', '-']).splitlines():
            m = re.search(r'__ANDROID_API__\s+(\d+)', line)
            if m:
                return m.group(1)
    except:
        pass

def detect_compiler_family(env, toolchain, family):
    if family == 'gcc':
        keys = ['GNU', 'gnu', 'gcc', 'g++']
    elif family == 'clang':
        keys = ['clang']

    def _check_tool(toolchain, tool):
        if toolchain:
            tool = '{}-{}'.format(toolchain, tool)
        try:
            out = run_command([tool, '-v'],
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            for k in keys:
                if k in out:
                    return True
        except:
            pass
        return False

    for var in ['CC', 'CCLD', 'CXX', 'CXXLD']:
        if var in env:
            if not _check_tool('', env[var]):
                return False

    if not 'gnu' in toolchain:
        if 'CC' not in env:
            for tool in ['cc', 'gcc', 'clang']:
                if _check_tool(toolchain, tool):
                    break
            else:
                return False

        if 'CCLD' not in env:
            for tool in ['ld', 'gcc', 'clang']:
                if _check_tool(toolchain, tool):
                    break
            else:
                return False

        if 'CXX' not in env or 'CXXLD' not in env:
            for tool in ['g++', 'clang++']:
                if _check_tool(toolchain, tool):
                    break
            else:
                return False

    return True

def detect_compiler_presence(compiler):
    try:
        subprocess.run([compiler, '-v'],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False

def detect_native_cc_cxx_compilers():
    compilers = {}
    for compiler in ['gcc', 'clang', 'cc']:
        if detect_compiler_presence(compiler):
            compilers['CC'] = which(compiler)
            compilers['CCLD'] = which(compiler)
            break
    for compiler in ['g++', 'clang++', 'c++']:
        if detect_compiler_presence(compiler):
            compilers['CXX'] = which(compiler)
            compilers['CXXLD'] = which(compiler)
            break
    return compilers

# Guess platform argument for OpenSSL's Configure script basing on toolchain string.
# (see `./Configure LIST` for a list of all platforms and their format)
# See also openssl/Configurations/[0-9]*.conf
def detect_openssl_platform(toolchain):
    """
    >>> detect_openssl_platform('x86_64-pc-linux-gnu')
    'linux-x86_64'
    >>> detect_openssl_platform('aarch64-linux-gnu')
    'linux-aarch64'
    >>> detect_openssl_platform('mips-openwrt-linux-uclibc')
    'linux-mips32'
    >>> detect_openssl_platform('arm-linux-gnueabihf')
    'linux-generic32'
    >>> detect_openssl_platform('arm-bcm2708hardfp-linux-gnueabi')
    'linux-generic32'
    >>> detect_openssl_platform('armv7a-linux-androideabi33')
    'android-arm'
    >>> detect_openssl_platform('mips64-linux-musl')
    'linux64-mips64'
    >>> detect_openssl_platform('arm-linux-androideabi')
    'android-arm'
    >>> detect_openssl_platform('aarch64-linux-android')
    'android-arm64'
    >>> detect_openssl_platform('aarch64-linux-android29')
    'android-arm64'
    >>> detect_openssl_platform('i686-linux-android')
    'android-x86'
    >>> detect_openssl_platform('x86_64-linux-android')
    'android-x86_64'
    >>> detect_openssl_platform('i686-pc-linux-gnu')
    'linux-x86'

    # other edge cases
    >>> detect_openssl_platform('mingw-w64-x86_64')
    'mingw64'
    >>> detect_openssl_platform('x86_64-pc-cygwin')
    'Cygwin-x86_64'
    >>> detect_openssl_platform('')
    'cc'
    """

    # we assume that:
    # - 'android' has a higher priority than 'linux'
    # - if toolchain contains '64' and 'android' => we have android64 on 64-bit CPU
    # - if toolchain contains '64' and 'linux' => we have linux64 on 64-bit CPU
    # - openssl architectures 'aout', 'elf', 'latomic' are meaningless (they cannot be found in
    #   toolchain)
    # - compiler suffixes '-cc', '-gcc', '-clang' are meaningless too
    # - aix* OSes are never used :)
    # - 'android64' are currently just aliases for 'android' architectures;
    #   'android64-aarch64' is the same as 'android-arm64'

    bitness = 64 if '64' in toolchain else 32

    ### generic detection

    # XXX: this is a dirty parser based on very simple heuristics and verified on a
    # limited set of platforms

    # Set of platforms formatted as (OS, [arch1, ...]).
    #
    # `arch` is an architecture that openssl accepts for a given `os`. arch in a toolchain may
    # be the same but called differently, e.g. 'x86' vs 'i686'. To handle such cases,
    # `arch_aliases` are used (see below): if `arch` is not in toolchain, we try to search all
    # aliases (one by one) before moving to the next arch.
    #
    # Some tunings:
    # - 'android-armeabi' is an alias of 'android-arm' => removed 'armeabi'
    # - 'linux-armv4' fails on android toolchain; we use 'linux-generic32' => removed 'armv4'
    # - 'android64' is an alias for 'android' => removed 'android64'
    # - 'generic64' and 'generic32' never occur in toolchains => removed them
    platforms = [
        # order is significant here as we search substrings in a whole toolchain string!
        # [(os,      [arch, ...]), ...]
        ('BSD',      ['aarch64', 'ia64', 'riscv64',
                      'sparc64', 'sparcv8', 'x86_64', 'x86']),
        ('Cygwin',   ['i386', 'i486', 'i586', 'i686', 'x86_64', 'x86']),
        ('android',  ['arm64', 'arm', 'mips64', 'mips', 'x86_64', 'x86']),
        ('darwin64', ['arm64', 'ppc', 'x86_64']),
        ('darwin',   ['i386', 'ppc']),
        ('linux64',  ['loongarch64', 'mips64', 'riscv64', 's390x', 'sparcv9']),
        ('linux32',  ['s390x']),
        ('linux',    ['aarch64', 'arm64ilp32', 'ia64', 'mips64', 'mips32', 'ppc64le',
                      'ppc64', 'ppc', 'sparcv8', 'sparcv9', 'x86_64', 'x86', 'x32']),
        ('mingw64',  []),
        ('mingw',    []),
    ]
    arch_aliases = {
        'arm64':     ['aarch64'],
        'mips32':    ['mips'],
        'x86':       ['i386', 'i486', 'i586', 'i686'],
    }

    #~print('0: ', toolchain)
    for ossl_os, ossl_archs in platforms:
        os = ossl_os.lower()
        # when 64 bit, we explicitly prefer 'linux64' and similar (assuming libc and other
        # libs to be 64 bit)
        if os in toolchain or (bitness == 64 and os.endswith('64') and os[:-2] in toolchain):
            for arch in ossl_archs:
                #~print('1: ', os, arch)
                if arch in toolchain:
                    return '-'.join([ossl_os, arch])
                # or any of the aliases in toolchain
                for alias in arch_aliases.get(arch, []):
                    if alias in toolchain:
                        return '-'.join([ossl_os, arch])

            # ignore 'android64' without generality restriction
            if ossl_os in ['mingw', 'mingw64', 'Cygwin']:
                return ossl_os

            # on 'linux' and 'BSD', unless arch was found, return '...-generic{32,64}'
            if ossl_os in ['BSD', 'linux']:
                return '-'.join([os, 'generic64' if bitness == 64 else 'generic32'])
    return 'cc' # just a fallback

def detect_cpu_count():
    try:
        return len(os.sched_getaffinity(0))
    except:
        pass

    try:
        return multiprocessing.cpu_count()
    except:
        pass

    return None

def parse_env(unparsed_vars):
    env = dict()
    for e in unparsed_vars:
        k, v = e.split('=', 1)
        env[k] = v
    return env

def parse_dep(unparsed_dep):
    m = re.match(r'^(.*?)-([0-9][a-z0-9.-]+)$', unparsed_dep)
    if not m:
        die("can't determine version of '{}'", unparsed_dep)
    return m.group(1), m.group(2)

def parse_ver(unparsed_ver, fn=str, count=None):
    try:
        comps = list(map(fn, unparsed_ver.split('.')))
        if count:
            if len(comps) > count:
                comps = comps[:count]
            elif len(comps) < count:
                comps = comps + [0] * (count-len(comps))
        return tuple(comps)
    except:
        die("can't parse version '{}'", unparsed_ver)

def run_command(cmd, stdin=DEV_NULL, stdout=subprocess.PIPE, stderr=DEV_NULL):
    proc = subprocess.Popen(
        cmd,
        shell=not isinstance(cmd, list),
        stdin=stdin, stdout=stdout, stderr=stderr)

    out = proc.stdout.read()

    try:
        out = out.decode()
    except:
        pass
    try:
        out = str(out)
    except:
        pass
    try:
        out = out.strip()
    except:
        pass
    return out

def find_file_upwards(path, search_file):
    while True:
        parent = os.path.dirname(path)
        if parent == path:
            break # root
        path = parent
        child_path = os.path.join(path, search_file)
        if os.path.exists(child_path):
            return child_path

def which(tool):
    try:
        path = shutil.which(tool)
        if path:
            return path
    except:
        pass

    out = run_command(['which', tool])
    if out:
        return out

def mkpath(path):
    try:
        os.makedirs(path)
    except:
        pass

def rmpath(path):
    try:
        if os.path.isdir(path):
            shutil.rmtree(path)
        else:
            os.remove(path)
    except:
        pass

def rm_emptydir(path):
    try:
        os.rmdir(path)
    except:
        pass

def touch(path):
    open(path, 'w').close()

def msg(text, *args):
    if args:
        text = text.format(*args)
    print(text)

def die(text, *args):
    if args:
        text = text.format(*args)
    print('error: ' + text, file=sys.stderr)
    exit(1)

#
# Parse command-line options.
#

if '--self-test' in sys.argv:
    failures, total = doctest.testmod(report=True)

    if failures == 0:
        print('OK: {} tests passed'.format(total))
        exit(0)
    else:
        exit(1)

parser = argparse.ArgumentParser(description='build third-party packages')

parser.add_argument('--self-test', action='store_true',
                    help='run doctests and exit')

parser.add_argument('--root-dir', dest='root_dir', type=str, required=True,
                    help='path to project root')

parser.add_argument('--work-dir', dest='work_dir', type=str, required=True,
                    help='path to working directory')

parser.add_argument('--dist-dir', dest='dist_dir', type=str, required=True,
                    help='path to vendored distfiles directory')

parser.add_argument('--build', dest='build', type=str, required=True,
                    help='system when package is built (e.g. x86_64-pc-linux-gnu)')

parser.add_argument('--host', dest='host', type=str, required=True,
                    help='system when package will run (e.g. aarch64-linux-gnu)')

parser.add_argument('--toolchain', dest='toolchain', type=str,
                    help='toolchain prefix (e.g. aarch64-linux-gnu)')

parser.add_argument('--variant', dest='variant', type=str, required=True,
                    help='build variant', choices=['debug', 'release'])

parser.add_argument('--package', dest='package', type=str, required=True,
                    help='package name and version (e.g. openssl-3.0.7-rc1)')

parser.add_argument('--deps', dest='deps', type=str, nargs='*',
                    help='package dependencies (should be built previously)')

parser.add_argument('--vars', dest='vars', type=str, nargs='*',
                    help='environment variables (e.g. CC=gcc CXX=g++ ...)')

parser.add_argument('--android-platform', dest='android_platform', type=str,
                    help='android target platform version')

parser.add_argument('--macos-platform', dest='macos_platform', type=str,
                    help='macos target platform version')

parser.add_argument('--macos-arch', dest='macos_arch', type=str, nargs='*',
                    help='macos target architecture(s)')

args = parser.parse_args()

#
# Setup context.
#

ctx = Context()

ctx.pkg = args.package
ctx.pkg_name, ctx.pkg_ver = parse_dep(args.package)
ctx.pkg_ver_major, ctx.pkg_ver_minor, ctx.pkg_ver_patch = parse_ver(ctx.pkg_ver, count=3)
ctx.pkg_deps = args.deps or []

ctx.build = args.build
ctx.host = args.host
ctx.toolchain = args.toolchain or ''
ctx.variant = args.variant
ctx.android_platform = args.android_platform
ctx.macos_platform = args.macos_platform
ctx.macos_arch = args.macos_arch

ctx.unparsed_env = args.vars or []
ctx.env = parse_env(args.vars)

os.chdir(args.root_dir)

ctx.root_dir = os.path.abspath('.')
ctx.work_dir = os.path.abspath(args.work_dir)
ctx.dist_dir = os.path.abspath(args.dist_dir)

ctx.pkg_dir = os.path.join(ctx.work_dir, ctx.pkg)

ctx.pkg_src_dir = os.path.join(ctx.pkg_dir, 'src')
ctx.pkg_bin_dir = os.path.join(ctx.pkg_dir, 'bin')
ctx.pkg_lib_dir = os.path.join(ctx.pkg_dir, 'lib')
ctx.pkg_inc_dir = os.path.join(ctx.pkg_dir, 'include')
ctx.pkg_rpath_dir = os.path.join(ctx.pkg_dir, 'rpath')

ctx.log_file = os.path.join(ctx.pkg_dir, 'build.log')
ctx.commit_file = os.path.join(ctx.pkg_dir, 'commit')

ctx.prefer_cmake = bool(args.macos_platform or args.android_platform)

#
# Build package.
#

rmpath(ctx.commit_file)
mkpath(ctx.pkg_src_dir)

changedir(ctx, ctx.pkg_dir)

if ctx.pkg_name == 'libuv':
    download(
        ctx,
        'https://dist.libuv.org/dist/v{ctx.pkg_ver}/libuv-v{ctx.pkg_ver}.tar.gz',
        'libuv-v{ctx.pkg_ver}.tar.gz')
    unpack(
        ctx,
        'libuv-v{ctx.pkg_ver}.tar.gz',
        'libuv-v{ctx.pkg_ver}')
    changedir(ctx, 'src/libuv-v{ctx.pkg_ver}')
    if ctx.prefer_cmake:
        mkpath('build')
        changedir(ctx, 'build')
        execute_cmake(ctx, '..', args=[
            '-DLIBUV_BUILD_TESTS=OFF',
            ])
        execute_cmake_build(ctx)
        if os.path.isfile('libuv_a.a'):
            shutil.copy('libuv_a.a', '../libuv.a')
        else:
            shutil.copy('libuv.a', '../libuv.a')
        changedir(ctx, '..')
    else:
        subst_files(ctx, 'src/unix/core.c', ' dup3', ' uv__dup3')
        execute(ctx, './autogen.sh')
        execute(ctx, './configure --host={host} {vars} {flags} {opts}'.format(
            host=ctx.toolchain,
            vars=format_vars(ctx),
            flags=format_flags(ctx),
            opts=' '.join([
                '--with-pic',
                '--enable-static',
            ])))
        execute_make(ctx)
        shutil.copy('.libs/libuv.a', 'libuv.a')
    install_tree(ctx, 'include', ctx.pkg_inc_dir)
    install_files(ctx, 'libuv.a', ctx.pkg_lib_dir)
elif ctx.pkg_name == 'libatomic_ops':
    download(
        ctx,
        'https://github.com/ivmai/libatomic_ops/releases/download/'
            'v{ctx.pkg_ver}/libatomic_ops-{ctx.pkg_ver}.tar.gz',
        'libatomic_ops-{ctx.pkg_ver}.tar.gz')
    unpack(ctx,
           'libatomic_ops-{ctx.pkg_ver}.tar.gz',
           'libatomic_ops-{ctx.pkg_ver}')
    changedir(ctx, 'src/libatomic_ops-{ctx.pkg_ver}')
    execute(ctx, './configure --host={host} {vars} {flags} {opts}'.format(
        host=ctx.toolchain,
        vars=format_vars(ctx),
        flags=format_flags(ctx, cflags='-fPIC'),
        opts=' '.join([
            '--disable-docs',
            '--disable-shared',
            '--enable-static',
           ])))
    execute_make(ctx)
    install_tree(ctx, 'src', ctx.pkg_inc_dir, include=['*.h'])
    install_files(ctx, 'src/.libs/libatomic_ops.a', ctx.pkg_lib_dir)
elif ctx.pkg_name == 'libunwind':
    download(
        ctx,
        'http://download.savannah.nongnu.org/releases/libunwind/'
            'libunwind-{ctx.pkg_ver}.tar.gz',
        'libunwind-{ctx.pkg_ver}.tar.gz')
    unpack(ctx,
           'libunwind-{ctx.pkg_ver}.tar.gz',
           'libunwind-{ctx.pkg_ver}')
    changedir(ctx, 'src/libunwind-{ctx.pkg_ver}')
    execute(ctx, './configure --host={host} {vars} {flags} {opts}'.format(
        host=ctx.toolchain,
        vars=format_vars(ctx),
        flags=format_flags(ctx, cflags='-fcommon -fPIC'),
        opts=' '.join([
            '--disable-coredump',
            '--disable-minidebuginfo',
            '--disable-ptrace',
            '--disable-setjmp',
            '--disable-shared',
            '--enable-static',
           ])))
    execute_make(ctx)
    install_files(ctx, 'include/*.h', ctx.pkg_inc_dir)
    install_files(ctx, 'src/.libs/libunwind.a', ctx.pkg_lib_dir)
elif ctx.pkg_name == 'libuuid':
    download(
        ctx,
        'https://mirrors.edge.kernel.org/pub/linux/utils/util-linux/'+
            'v{ctx.pkg_ver_major}.{ctx.pkg_ver_minor}/util-linux-{ctx.pkg_ver}.tar.gz',
        'util-linux-{ctx.pkg_ver}.tar.gz')
    unpack(ctx,
           'util-linux-{ctx.pkg_ver}.tar.gz',
           'util-linux-{ctx.pkg_ver}')
    changedir(ctx, 'src/util-linux-{ctx.pkg_ver}')
    execute(ctx, './configure --host={host} {vars} {flags} {opts}'.format(
        host=ctx.toolchain,
        vars=format_vars(ctx),
        flags=format_flags(ctx, cflags='-fcommon -fPIC'),
        opts=' '.join([
            '--disable-year2038',
            '--disable-all-programs',
            '--enable-libuuid',
           ])))
    execute_make(ctx)
    install_files(ctx, 'libuuid/src/uuid.h', ctx.pkg_inc_dir)
    install_files(ctx, '.libs/libuuid.a', ctx.pkg_lib_dir)
elif ctx.pkg_name == 'openfec':
    if ctx.variant == 'debug':
        setattr(ctx, 'res_dir', 'bin/Debug')
    else:
        setattr(ctx, 'res_dir', 'bin/Release')
    download(
        ctx,
        'https://github.com/roc-streaming/openfec/archive/v{ctx.pkg_ver}.tar.gz',
        'openfec_v{ctx.pkg_ver}.tar.gz')
    unpack(ctx,
           'openfec_v{ctx.pkg_ver}.tar.gz',
           'openfec-{ctx.pkg_ver}')
    changedir(ctx, 'src/openfec-{ctx.pkg_ver}')
    mkpath('build')
    changedir(ctx, 'build')
    execute_cmake(ctx, '..', args=[
        '-DBUILD_STATIC_LIBS=ON',
        '-DDEBUG:STRING=%s' % ('ON' if ctx.variant == 'debug' else 'OFF'),
        ])
    execute_cmake_build(ctx)
    changedir(ctx, '..')
    install_tree(ctx, 'src', ctx.pkg_inc_dir, include=['*.h'])
    install_files(ctx, '{ctx.res_dir}/libopenfec.a', ctx.pkg_lib_dir)
elif ctx.pkg_name == 'openssl':
    download(
        ctx,
        'https://www.openssl.org/source/openssl-{ctx.pkg_ver}.tar.gz',
        'openssl-{ctx.pkg_ver}.tar.gz')
    unpack(
        ctx,
        'openssl-{ctx.pkg_ver}.tar.gz',
        'openssl-{ctx.pkg_ver}')
    changedir(ctx, 'src/openssl-{ctx.pkg_ver}')
    # see https://github.com/openssl/openssl/blob/master/INSTALL.md#configuration-options
    execute(ctx, '{vars} {flags} ./Configure {platform} {variant} {options}'.format(
        vars=format_vars(ctx, disable_launcher=True),
        flags=format_flags(ctx),
        platform=detect_openssl_platform(ctx.host),
        variant='--debug' if ctx.variant == 'debug' else '--release',
        options=' '.join([
            'no-asan',
            'no-buildtest-c++',
            'no-external-tests',
            'no-fuzz-afl',
            'no-fuzz-libfuzzer',
            'no-shared',
            'no-tests',
            'no-ubsan',
            'no-ui-console',
            'no-unit-test',
        ])))
    execute_make(ctx)
    install_tree(ctx, 'include', ctx.pkg_inc_dir)
    install_files(ctx, 'libssl.a', ctx.pkg_lib_dir)
    install_files(ctx, 'libcrypto.a', ctx.pkg_lib_dir)
elif ctx.pkg_name == 'speexdsp':
    if ctx.pkg_ver.split('.', 1) > ['1', '2'] and (
            not re.match('^1.2[a-z]', ctx.pkg_ver) or ctx.pkg_ver == '1.2rc3'):
        setattr(ctx, 'pkg_repo', 'speexdsp')
    else:
        setattr(ctx, 'pkg_repo', 'speex')
    download(
        ctx,
        'http://downloads.xiph.org/releases/speex/{ctx.pkg_repo}-{ctx.pkg_ver}.tar.gz',
        '{ctx.pkg_repo}-{ctx.pkg_ver}.tar.gz')
    unpack(
        ctx,
        '{ctx.pkg_repo}-{ctx.pkg_ver}.tar.gz',
        '{ctx.pkg_repo}-{ctx.pkg_ver}')
    changedir(ctx, 'src/{ctx.pkg_repo}-{ctx.pkg_ver}')
    execute(ctx, './configure --host={host} {vars} {flags} {opts}'.format(
        host=ctx.toolchain,
        vars=format_vars(ctx),
        flags=format_flags(ctx, cflags='-fPIC'),
        opts=' '.join([
            '--disable-examples',
            '--disable-shared',
            '--enable-static',
           ])))
    execute_make(ctx)
    install_tree(ctx, 'include', ctx.pkg_inc_dir)
    install_files(ctx, 'lib{ctx.pkg_repo}/.libs/libspeexdsp.a', ctx.pkg_lib_dir)
elif ctx.pkg_name == 'sndfile':
    download(
        ctx,
        'http://www.mega-nerd.com/libsndfile/files/libsndfile-{ctx.pkg_ver}.tar.gz',
        'libsndfile-{ctx.pkg_ver}.tar.gz')
    unpack(
        ctx,
        'libsndfile-{ctx.pkg_ver}.tar.gz',
        'libsndfile-{ctx.pkg_ver}')
    changedir(ctx, 'src/libsndfile-{ctx.pkg_ver}')
    execute(ctx, '{configure} --host={host} {vars} {flags} {opts}'.format(
        configure=' '.join(filter(None, [
            # workaround for outdated config.sub
            'ac_cv_host=%s' % ctx.toolchain if ctx.toolchain else '',
            # configure
            './configure',
        ])),
        host=ctx.toolchain,
        vars=format_vars(ctx),
        # explicitly enable -pthread because libtool doesn't add it on some platforms
        flags=format_flags(ctx, cflags='-fPIC', pthread=True),
        opts=' '.join([
            '--disable-external-libs',
            '--disable-shared',
            '--enable-static',
        ])))
    execute_make(ctx)
    install_files(ctx, 'src/sndfile.h', ctx.pkg_inc_dir)
    install_files(ctx, 'src/.libs/libsndfile.a', ctx.pkg_lib_dir)
elif ctx.pkg_name == 'sox':
    download(
        ctx,
        'https://downloads.sourceforge.net/project/sox/sox/'
            '{ctx.pkg_ver}/sox-{ctx.pkg_ver}.tar.gz',
        'sox-{ctx.pkg_ver}.tar.gz')
    unpack(
        ctx,
        'sox-{ctx.pkg_ver}.tar.gz',
        'sox-{ctx.pkg_ver}')
    changedir(ctx, 'src/sox-{ctx.pkg_ver}')
    execute(ctx, './configure --host={host} {vars} {flags} {opts}'.format(
        host=ctx.toolchain,
        vars=format_vars(ctx),
        flags=format_flags(ctx, cflags='-w -Wno-incompatible-function-pointer-types'),
        opts=' '.join([
            '--disable-openmp',
            '--disable-shared',
            '--enable-static',
            '--with-amrnb=no',
            '--with-amrwb=no',
            '--with-ao=no',
            '--with-flac=no',
            '--with-gsm=no',
            '--with-lpc10=no',
            '--with-mp3=no',
            '--with-oggvorbis=no',
            '--with-opus=no',
            '--with-pulseaudio=no',
            '--with-sndfile=no',
            '--with-sndio=no',
            '--with-wavpack=no',
            '--without-ao',
            '--without-id3tag',
            '--without-ladspa',
            '--without-lame',
            '--without-libltdl',
            '--without-mad',
            '--without-magic',
            '--without-opus',
            '--without-png',
            '--without-twolame',
        ])))
    execute_make(ctx)
    install_files(ctx, 'src/sox.h', ctx.pkg_inc_dir)
    install_files(ctx, 'src/.libs/libsox.a', ctx.pkg_lib_dir)
elif ctx.pkg_name == 'alsa':
    download(
        ctx,
        'https://www.alsa-project.org/files/pub/lib/alsa-lib-{ctx.pkg_ver}.tar.bz2',
        'alsa-lib-{ctx.pkg_ver}.tar.bz2')
    unpack(ctx,
           'alsa-lib-{ctx.pkg_ver}.tar.bz2',
           'alsa-lib-{ctx.pkg_ver}')
    changedir(ctx, 'src/alsa-lib-{ctx.pkg_ver}')
    execute(ctx, './configure --host={host} {vars} {opts}'.format(
        host=ctx.toolchain,
        vars=format_vars(ctx),
        opts=' '.join([
            '--disable-python',
            '--disable-static',
            '--enable-shared',
        ])))
    execute_make(ctx)
    install_tree(ctx, 'include/alsa',
            os.path.join(ctx.pkg_inc_dir, 'alsa'),
            exclude=['alsa'])
    install_files(ctx, 'src/.libs/libasound.so', ctx.pkg_lib_dir)
    install_files(ctx, 'src/.libs/libasound.so.*', ctx.pkg_rpath_dir)
elif ctx.pkg_name == 'pulseaudio':
    download(
        ctx,
        'https://freedesktop.org/software/pulseaudio/releases/'
            'pulseaudio-{ctx.pkg_ver}.tar.gz',
        'pulseaudio-{ctx.pkg_ver}.tar.gz')
    unpack(
        ctx,
        'pulseaudio-{ctx.pkg_ver}.tar.gz',
        'pulseaudio-{ctx.pkg_ver}')
    pa_ver = parse_ver(ctx.pkg_ver, int)
    if (8, 99, 1) <= pa_ver < (11, 99, 1):
        apply_patch(
            ctx,
            'src/pulseaudio-{ctx.pkg_ver}',
            'https://bugs.freedesktop.org/attachment.cgi?id=136927',
            '0001-memfd-wrappers-only-define-memfd_create-if-not-alrea.patch')
    if pa_ver < (12, 99, 1):
        subst_tree(ctx, 'src/pulseaudio-{ctx.pkg_ver}', ['*.h', '*.c'],
                   from_='#include <asoundlib.h>',
                   to='#include <alsa/asoundlib.h>')
    changedir(ctx, 'src/pulseaudio-{ctx.pkg_ver}')
    if pa_ver < (14, 99, 1):
        # workaround for "missing acolocal-1.15" and "missing automake-1.15" errors
        # on some systems; since we're not modifying any autotools stuff, it's safe
        # to replace corresponding commands with "true" command
        if os.path.exists('Makefile.in'):
            subst_files(ctx, 'Makefile.in', '@ACLOCAL@', 'true')
            subst_files(ctx, 'Makefile.in', '@AUTOMAKE@', 'true')
        execute(ctx, './configure --host={host} {vars} {flags} {deps} {opts}'.format(
            host=ctx.toolchain,
            vars=format_vars(ctx),
            flags=format_flags(ctx, cflags='-w -Wno-implicit-function-declaration' + \
                ' -fomit-frame-pointer -O2'),
            deps=' '.join([
                'LIBJSON_CFLAGS=" "',
                'LIBJSON_LIBS="-ljson-c"',
                'LIBSNDFILE_CFLAGS=" "',
                'LIBSNDFILE_LIBS="-lsndfile"',
            ]),
            opts=' '.join([
                '--disable-manpages',
                '--disable-neon-opt',
                '--disable-openssl',
                '--disable-orc',
                '--disable-static',
                '--disable-tests',
                '--disable-webrtc-aec',
                '--enable-shared',
                '--without-caps',
            ])))
        execute_make(ctx)
        install_files(ctx, 'config.h', ctx.pkg_inc_dir)
        install_tree(ctx, 'src/pulse', os.path.join(ctx.pkg_inc_dir, 'pulse'),
                     include=['*.h'])
        install_files(ctx, 'src/.libs/libpulse.so', ctx.pkg_lib_dir)
        install_files(ctx, 'src/.libs/libpulse.so.0', ctx.pkg_rpath_dir)
        install_files(ctx, 'src/.libs/libpulse-simple.so', ctx.pkg_lib_dir)
        install_files(ctx, 'src/.libs/libpulse-simple.so.0', ctx.pkg_rpath_dir)
        install_files(ctx, 'src/.libs/libpulsecommon-*.so', ctx.pkg_lib_dir)
        install_files(ctx, 'src/.libs/libpulsecommon-*.so', ctx.pkg_rpath_dir)
    else:
        mkpath('builddir')
        changedir(ctx, 'builddir')
        mkpath('pcdir')
        generate_pc_files(ctx, 'pcdir')
        generate_meson_crossfile(ctx, 'pcdir', 'crossfile.txt')
        execute(ctx, 'meson .. {opts}'.format(
            opts=' '.join([
                '--cross-file=crossfile.txt',
                '-Ddaemon=false',
                '-Ddoxygen=false',
                '-Dgcov=false',
                '-Dman=false',
                '-Dtests=false',
            ])))
        execute(ctx, 'ninja')
        execute(ctx, 'DESTDIR=../instdir ninja install')
        changedir(ctx, '..')
        install_tree(ctx, 'instdir/usr/local/include/pulse',
                     os.path.join(ctx.pkg_inc_dir, 'pulse'),
                     include=['*.h'])
        install_files(ctx, 'builddir/src/pulse/libpulse.so', ctx.pkg_lib_dir)
        install_files(ctx, 'builddir/src/pulse/libpulse.so.0', ctx.pkg_rpath_dir)
        install_files(ctx, 'builddir/src/pulse/libpulse-simple.so', ctx.pkg_lib_dir)
        install_files(ctx, 'builddir/src/pulse/libpulse-simple.so.0', ctx.pkg_rpath_dir)
        install_files(ctx, 'builddir/src/libpulsecommon-*.so', ctx.pkg_lib_dir)
        install_files(ctx, 'builddir/src/libpulsecommon-*.so', ctx.pkg_rpath_dir)
elif ctx.pkg_name == 'libogg':
    download(
        ctx,
        'https://downloads.xiph.org/releases/ogg/libogg-{ctx.pkg_ver}.tar.gz',
        'libogg-{ctx.pkg_ver}.tar.gz')
    unpack(ctx,
           'libogg-{ctx.pkg_ver}.tar.gz',
           'libogg-{ctx.pkg_ver}')
    changedir(ctx, 'src/libogg-{ctx.pkg_ver}')
    execute(ctx, './configure --host={host} {vars} {flags} {opts}'.format(
        host=ctx.toolchain,
        vars=format_vars(ctx),
        flags=format_flags(ctx, cflags='-fPIC'),
        opts=' '.join([
            '--disable-shared',
            '--enable-static',
           ])))
    execute_make(ctx)
    install_tree(ctx, 'include', ctx.pkg_inc_dir, include=['*.h'])
    install_files(ctx, 'src/.libs/libogg.a', ctx.pkg_lib_dir)
    install_files(ctx, 'ogg.pc', ctx.pkg_lib_dir)
elif ctx.pkg_name == 'libvorbis':
    download(
        ctx,
        'https://downloads.xiph.org/releases/vorbis/libvorbis-{ctx.pkg_ver}.tar.gz',
        'libvorbis-{ctx.pkg_ver}.tar.gz')
    unpack(ctx,
           'libvorbis-{ctx.pkg_ver}.tar.gz',
           'libvorbis-{ctx.pkg_ver}')
    changedir(ctx, 'src/libvorbis-{ctx.pkg_ver}')
    execute(ctx, './configure --host={host} {vars} {flags} {opts}'.format(
        host=ctx.toolchain,
        vars=format_vars(ctx),
        flags=format_flags(ctx, cflags='-fPIC'),
        opts=' '.join([
            '--disable-shared',
            '--enable-static',
           ])))
    execute_make(ctx)
    install_tree(ctx, 'include', ctx.pkg_inc_dir, include=['*.h'])
    install_files(ctx, 'lib/.libs/libvorbis.a', ctx.pkg_lib_dir)
    install_files(ctx, 'lib/.libs/libvorbisenc.a', ctx.pkg_lib_dir)
    install_files(ctx, 'lib/.libs/libvorbisfile.a', ctx.pkg_lib_dir)
elif ctx.pkg_name == 'ltdl':
    download(
        ctx,
        'https://ftp.gnu.org/gnu/libtool/libtool-{ctx.pkg_ver}.tar.gz',
        'libtool-{ctx.pkg_ver}.tar.gz')
    unpack(
        ctx,
        'libtool-{ctx.pkg_ver}.tar.gz',
        'libtool-{ctx.pkg_ver}')
    changedir(ctx, 'src/libtool-{ctx.pkg_ver}')
    execute(ctx, './configure --host={host} {vars} {opts}'.format(
        host=ctx.toolchain,
        vars=format_vars(ctx),
        opts=' '.join([
            '--disable-static',
            '--enable-shared',
        ])))
    execute_make(ctx)
    install_files(ctx, 'libltdl/ltdl.h', ctx.pkg_inc_dir)
    install_tree(ctx, 'libltdl/libltdl', os.path.join(ctx.pkg_inc_dir, 'libltdl'))
    install_files(ctx, 'libltdl/.libs/libltdl.so', ctx.pkg_lib_dir)
    install_files(ctx, 'libltdl/.libs/libltdl.so.*', ctx.pkg_rpath_dir)
elif ctx.pkg_name == 'json-c':
    download(
        ctx,
        'https://github.com/json-c/json-c/archive/json-c-{ctx.pkg_ver}.tar.gz',
        'json-c-{ctx.pkg_ver}.tar.gz')
    unpack(
        ctx,
        'json-c-{ctx.pkg_ver}.tar.gz',
        'json-c-json-c-{ctx.pkg_ver}')
    changedir(ctx, 'src/json-c-json-c-{ctx.pkg_ver}')
    execute(ctx, '{configure} --host={host} {vars} {flags} {opts}'.format(
        configure=' '.join(filter(None, [
            # workaround for outdated config.sub
            'ac_cv_host=%s' % ctx.toolchain if ctx.toolchain else '',
            # disable rpl_malloc and rpl_realloc
            'ac_cv_func_malloc_0_nonnull=yes',
            'ac_cv_func_realloc_0_nonnull=yes',
            # configure
            './configure',
        ])),
        host=ctx.toolchain,
        vars=format_vars(ctx),
        flags=format_flags(ctx, cflags='-w -fPIC'),
        opts=' '.join([
            '--enable-static',
            '--disable-shared',
        ])))
    execute_make(ctx, cpu_count=0) # -j is buggy for json-c
    install_tree(ctx, '.', ctx.pkg_inc_dir, include=['*.h'])
    install_files(ctx, '.libs/libjson-c.a', ctx.pkg_lib_dir)
elif ctx.pkg_name == 'gengetopt':
    native_compilers = detect_native_cc_cxx_compilers();
    download(
        ctx,
        'https://ftp.gnu.org/gnu/gengetopt/gengetopt-{ctx.pkg_ver}.tar.gz',
        'gengetopt-{ctx.pkg_ver}.tar.gz')
    unpack(
        ctx,
        'gengetopt-{ctx.pkg_ver}.tar.gz',
        'gengetopt-{ctx.pkg_ver}')
    changedir(ctx, 'src/gengetopt-{ctx.pkg_ver}')
    execute(ctx, './configure {vars}'.format(
        vars=format_vars(ctx, False, env={
            'CC' : native_compilers.get('CC', None),
            'CXX' : native_compilers.get('CXX', None),
            'COMPILER_LAUNCHER' : ctx.env.get('COMPILER_LAUNCHER', None),
        }),
        clear_env=True))
    execute_make(ctx, cpu_count=0) # -j is buggy for gengetopt
    install_files(ctx, 'src/gengetopt', ctx.pkg_bin_dir)
elif ctx.pkg_name == 'ragel':
    native_compilers = detect_native_cc_cxx_compilers();
    download(
        ctx,
        'https://www.colm.net/files/ragel/ragel-{ctx.pkg_ver}.tar.gz',
        'ragel-{ctx.pkg_ver}.tar.gz')
    unpack(
        ctx,
        'ragel-{ctx.pkg_ver}.tar.gz',
        'ragel-{ctx.pkg_ver}')
    changedir(ctx, 'src/ragel-{ctx.pkg_ver}')
    execute(ctx, './configure {vars}'.format(
        vars=format_vars(ctx, False, env={
            'CC' : native_compilers.get('CC', None),
            'CXX' : native_compilers.get('CXX', None),
            'COMPILER_LAUNCHER' : ctx.env.get('COMPILER_LAUNCHER', None),
        }),
        clear_env=True))
    execute_make(ctx)
    install_files(ctx, 'ragel/ragel', ctx.pkg_bin_dir)
elif ctx.pkg_name == 'cpputest':
    download(
        ctx,
        'https://github.com/cpputest/cpputest/releases/download/'
            'v{ctx.pkg_ver}/cpputest-{ctx.pkg_ver}.tar.gz',
        'cpputest-{ctx.pkg_ver}.tar.gz')
    unpack(
        ctx,
        'cpputest-{ctx.pkg_ver}.tar.gz',
        'cpputest-{ctx.pkg_ver}')
    changedir(ctx, 'src/cpputest-{ctx.pkg_ver}')
    if ctx.prefer_cmake:
        mkpath('build')
        changedir(ctx, 'build')
        execute_cmake(ctx, '..', args=[
            '-DMEMORY_LEAK_DETECTION=OFF',
            '-DTESTS=OFF',
            '-DBUILD_SHARED_LIBS=OFF',
            ])
        execute_cmake_build(ctx)
        shutil.copy('src/CppUTest/libCppUTest.a', '../libCppUTest.a')
        changedir(ctx, '..')
    else:
        execute(ctx, './configure --host={host} {vars} {flags} {opts}'.format(
                host=ctx.toolchain,
                vars=format_vars(ctx),
                # disable warnings, since CppUTest uses -Werror and may fail to
                # build on old GCC versions
                flags=format_flags(ctx, cflags='-w'),
                opts=' '.join([
                    # disable memory leak detection which is too hard to use properly
                    '--disable-memory-leak-detection',
                    # doesn't work on older platforms
                    '--disable-extensions',
                    '--enable-static',
                ])))
        execute_make(ctx)
        shutil.copy('lib/libCppUTest.a', 'libCppUTest.a')
    install_tree(ctx, 'include', ctx.pkg_inc_dir)
    install_files(ctx, 'libCppUTest.a', ctx.pkg_lib_dir)
elif ctx.pkg_name == 'google-benchmark':
    download(
        ctx,
        'https://github.com/google/benchmark/archive/v{ctx.pkg_ver}.tar.gz',
        'benchmark_v{ctx.pkg_ver}.tar.gz')
    unpack(
        ctx,
        'benchmark_v{ctx.pkg_ver}.tar.gz',
        'benchmark-{ctx.pkg_ver}')
    changedir(ctx, 'src/benchmark-{ctx.pkg_ver}')
    bench_ver = parse_ver(ctx.pkg_ver, int)
    if bench_ver < (1, 7, 1) and not which('python') and which('python3'):
        subst_tree(
            ctx, 'tools', ['*.py'],
            from_='#!/usr/bin/env python', to='#!/usr/bin/env python3')
    mkpath('build')
    changedir(ctx, 'build')
    execute_cmake(ctx, '..', args=[
        '-DBENCHMARK_ENABLE_GTEST_TESTS=OFF',
        '-DCMAKE_CXX_FLAGS=-w',
        ])
    execute_cmake_build(ctx)
    changedir(ctx, '..')
    install_tree(ctx, 'include', ctx.pkg_inc_dir, include=['*.h'])
    install_files(ctx, 'build/src/libbenchmark.a', ctx.pkg_lib_dir)
# end of deps
else:
    die("unknown 3rdparty '{}'", ctx.pkg)

# commit successful build
touch(ctx.commit_file)
