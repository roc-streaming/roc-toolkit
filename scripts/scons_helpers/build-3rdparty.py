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
            ' pkg pkg_name pkg_ver pkg_deps'
            ' toolchain variant macos_platform android_platform'
            ' env unparsed_env').split()
    })

#
# Helpers.
#

def download(ctx, url, name):
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

    path_res = 'src/' + name
    path_tmp = 'tmp/' + name

    if os.path.exists(path_res):
        msg('[found downloaded] {}', name)
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

def unpack(ctx, file_name, dir_name):
    dir_name_res = 'src/' + dir_name
    dir_name_tmp = 'tmp/' + dir_name

    if os.path.exists(dir_name_res):
        msg('[found unpacked] {}', dir_name)
        return

    msg('[unpack] {}', file_name)

    rmpath(dir_name_res)
    rmpath(dir_name_tmp)
    mkpath('tmp')

    tar = tarfile.open('src/'+file_name, 'r')
    tar.extractall('tmp')
    tar.close()

    shutil.move(dir_name_tmp, dir_name_res)
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
        try:
            cpu_count = len(os.sched_getaffinity(0))
        except:
            pass

    if cpu_count is None:
        try:
            cpu_count = multiprocessing.cpu_count()
        except:
            pass

    cmd = ['make']
    if cpu_count:
        cmd += ['-j' + str(cpu_count)]

    execute(ctx, ' '.join(cmd))

def execute_cmake(ctx, src_dir, args=None):
    def _getvar(var, default):
        if var in ctx.env:
            return ctx.env[var]
        return '-'.join([s for s in [ctx.toolchain, default] if s])

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
        args += ['-DCMAKE_OSX_DEPLOYMENT_TARGET=' + ctx.macos_platform]

    # cross-compiling for android
    if 'android' in ctx.toolchain:
        args += ['-DCMAKE_SYSTEM_NAME=Android']

        if ctx.android_platform:
            args += ['-DANDROID_PLATFORM=android-' + ctx.android_platform]

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
        if not 'android' in ctx.toolchain:
            args += [
                '-DCMAKE_C_COMPILER=' + quote(find_tool(compiler)),
            ]
        args += [
            '-DCMAKE_LINKER=' + quote(find_tool(_getvar('CCLD', 'gcc'))),
            '-DCMAKE_AR='     + quote(find_tool(_getvar('AR', 'ar'))),
            '-DCMAKE_RANLIB=' + quote(find_tool(_getvar('RANLIB', 'ranlib'))),
        ]

    cc_flags = [
        '-fPIC', # -fPIC should be set explicitly in older cmake versions
        '-fvisibility=hidden', # hide private symbols
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

    args += [
        '-DCMAKE_POSITION_INDEPENDENT_CODE=ON',
    ]

    execute(ctx, 'cmake ' + src_dir + ' ' + ' '.join(args))

def install_tree(ctx, src, dst, include=None, exclude=None):
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

    for filepat in file_patterns:
        for root, dir_names, file_names in os.walk(dir_path):
            for file_name in fnmatch.filter(file_names, filepat):
                filepath = os.path.join(root, file_name)
                if _match_path(filepath):
                    subst_files(filepath, from_, to)

def subst_files(ctx, path, from_, to):
    msg('[patch] {}', path)

    for line in fileinput.input(path, inplace=True):
        print(line.replace(from_, to), end='')

def apply_patch(ctx, dir_name, patch_url, patch_name):
    if subprocess.call(
        'patch --version', stdout=DEV_NULL, stderr=DEV_NULL, shell=True) != 0:
        return

    download(ctx, patch_url, patch_name)
    execute(ctx, 'patch -p1 -N -d {} -i {}'.format(
        'src/' + dir_name,
        '../' + patch_name), ignore_error=True)

def format_vars(ctx):
    ret = []
    for e in ctx.unparsed_env:
        ret.append(quote(e))
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

    is_android = 'android' in ctx.toolchain
    is_gnu = detect_compiler_family(ctx.env, ctx.toolchain, 'gcc')
    is_clang = detect_compiler_family(ctx.env, ctx.toolchain, 'clang')

    if ctx.variant == 'debug':
        if is_gnu or is_clang:
            cflags += ['-ggdb']
        else:
            cflags += ['-g']
    elif ctx.variant == 'release':
        cflags += ['-O2']

    if pthread and not is_android:
        if is_gnu or is_clang:
            cflags += ['-pthread']
        if is_gnu:
            ldflags += ['-pthread']
        else:
            ldflags += ['-lpthread']

    if is_gnu:
        ldflags += ['-Wl,-rpath-link=' + path for path in rpath_dirs]

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

    def _meson_flags(flags):
        return '[{}]'.format(', '.join(map(_meson_string, flags)))

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
            cc=_meson_string(ctx.env['CC']),
            cxx=_meson_string(ctx.env['CXX']),
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
                cflags=_meson_flags(cflags),
                ldflags=_meson_flags(ldflags)))

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
                lib = re.sub('\.[a-z]+$', '', lib)

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

def parse_env(unparsed_vars):
    env = dict()
    for e in unparsed_vars:
        k, v = e.split('=', 1)
        env[k] = v
    return env

def parse_dep(unparsed_dep):
    m = re.match('^(.*?)-([0-9][a-z0-9.-]+)$', unparsed_dep)
    if not m:
        die("can't determine version of '{}'", unparsed_dep)
    return m.group(1), m.group(2)

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
# Parse command-line.
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

parser.add_argument('--root-dir', metavar='root_dir', type=str, required=True,
                    help='path to project root')

parser.add_argument('--work-dir', metavar='work_dir', type=str, required=True,
                    help='path to working directory')

parser.add_argument('--dist-dir', metavar='dist_dir', type=str, required=True,
                    help='path to vendored distfiles directory')

parser.add_argument('--toolchain', metavar='toolchain', type=str, required=True,
                    help='toolchain prefix (e.g. aarch64-linux-gnu)')

parser.add_argument('--variant', metavar='variant', type=str, required=True,
                    help='build variant', choices=['debug', 'release'])

parser.add_argument('--package', metavar='package', type=str, required=True,
                    help='package name and version (e.g. openssl-3.0.7-rc1)')

parser.add_argument('--deps', metavar='deps', type=str, nargs='*',
                    help='package dependencies (should be built previously)')

parser.add_argument('--vars', metavar='vars', type=str, nargs='*',
                    help='environment variables (e.g. CC=gcc CXX=g++ ...)')

parser.add_argument('--platform-macos', metavar='platform_macos', type=str,
                    help='macos platform version to build against')

parser.add_argument('--platform-android', metavar='platform_android', type=str,
                    help='android platform version to build against')

args = parser.parse_args()

#
# Setup context.
#

ctx = Context()

ctx.pkg = args.package
ctx.pkg_name, ctx.pkg_ver = parse_dep(args.package)
ctx.pkg_deps = args.deps or []

ctx.toolchain = args.toolchain
ctx.variant = args.variant
ctx.macos_platform = args.platform_macos
ctx.android_platform = args.platform_android

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

rmpath(ctx.commit_file)
mkpath(ctx.pkg_src_dir)

os.chdir(ctx.pkg_dir)

#
# Recipes for particular packages.
#

if ctx.pkg_name == 'libuv':
    download(
        ctx,
        'https://dist.libuv.org/dist/v%s/libuv-v%s.tar.gz' % (ctx.pkg_ver, ctx.pkg_ver),
        'libuv-v%s.tar.gz' % ctx.pkg_ver)
    unpack(
        ctx,
        'libuv-v%s.tar.gz' % ctx.pkg_ver,
        'libuv-v%s' % ctx.pkg_ver)
    os.chdir('src/libuv-v%s' %  ctx.pkg_ver)
    subst_files(
        ctx,
        'include/uv.h',
        from_='__attribute__((visibility("default")))', to='')
    if 'android' in ctx.toolchain:
        mkpath('build')
        os.chdir('build')
        execute_cmake(
            ctx,
            '..',
            args=[
                '-DLIBUV_BUILD_TESTS=OFF',
                ])
        execute_make(ctx)
        shutil.copy('libuv_a.a', 'libuv.a')
        os.chdir('..')
        install_files(ctx, 'build/libuv.a', ctx.pkg_lib_dir)
    else:
        execute(ctx, './autogen.sh')
        execute(ctx, './configure --host=%s %s %s %s' % (
            ctx.toolchain,
            format_vars(ctx),
            format_flags(ctx, cflags='-fvisibility=hidden'),
            ' '.join([
                '--with-pic',
                '--enable-static',
            ])))
        execute_make(ctx)
        install_files(ctx, '.libs/libuv.a', ctx.pkg_lib_dir)
    install_tree(ctx, 'include', ctx.pkg_inc_dir)
elif ctx.pkg_name == 'libunwind':
    download(
        ctx,
        'http://download.savannah.nongnu.org/releases/libunwind/libunwind-%s.tar.gz' % ctx.pkg_ver,
        'libunwind-%s.tar.gz' % ctx.pkg_ver)
    unpack(ctx,
           'libunwind-%s.tar.gz' % ctx.pkg_ver,
           'libunwind-%s' % ctx.pkg_ver)
    os.chdir('src/libunwind-%s' % ctx.pkg_ver)
    execute(ctx, './configure --host=%s %s %s %s' % (
        ctx.toolchain,
        format_vars(ctx),
        format_flags(ctx, cflags='-fcommon -fPIC'),
        ' '.join([
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
elif ctx.pkg_name == 'libatomic_ops':
    download(
        ctx,
        'https://github.com/ivmai/libatomic_ops/releases/download/v%s/libatomic_ops-%s.tar.gz' % (
            ctx.pkg_ver, ctx.pkg_ver),
        'libatomic_ops-%s.tar.gz' % ctx.pkg_ver)
    unpack(ctx,
           'libatomic_ops-%s.tar.gz' % ctx.pkg_ver,
           'libatomic_ops-%s' % ctx.pkg_ver)
    os.chdir('src/libatomic_ops-%s' % ctx.pkg_ver)
    execute(ctx, './configure --host=%s %s %s %s' % (
        ctx.toolchain,
        format_vars(ctx),
        format_flags(ctx, cflags='-fPIC'),
        ' '.join([
            '--disable-docs',
            '--disable-shared',
            '--enable-static',
           ])))
    execute_make(ctx)
    install_tree(ctx, 'src', ctx.pkg_inc_dir, include=['*.h'])
    install_files(ctx, 'src/.libs/libatomic_ops.a', ctx.pkg_lib_dir)
elif ctx.pkg_name == 'openfec':
    if ctx.variant == 'debug':
        dist = 'bin/Debug'
    else:
        dist = 'bin/Release'
    download(
        ctx,
        'https://github.com/roc-streaming/openfec/archive/v%s.tar.gz' % ctx.pkg_ver,
        'openfec_v%s.tar.gz' % ctx.pkg_ver)
    unpack(ctx,
           'openfec_v%s.tar.gz' % ctx.pkg_ver,
           'openfec-%s' % ctx.pkg_ver)
    os.chdir('src/openfec-%s' % ctx.pkg_ver)
    mkpath('build')
    os.chdir('build')
    execute_cmake(
        ctx,
        '..',
        args=[
            '-DBUILD_STATIC_LIBS=ON',
            '-DDEBUG:STRING=%s' % ('ON' if ctx.variant == 'debug' else 'OFF'),
            ])
    execute_make(ctx)
    os.chdir('..')
    install_tree(ctx, 'src', ctx.pkg_inc_dir, include=['*.h'])
    install_files(ctx, '%s/libopenfec.a' % dist, ctx.pkg_lib_dir)
elif ctx.pkg_name == 'speexdsp':
    if ctx.pkg_ver.split('.', 1) > ['1', '2'] and (
            not re.match('^1.2[a-z]', ctx.pkg_ver) or ctx.pkg_ver == '1.2rc3'):
        repo = 'speexdsp'
    else:
        repo = 'speex'
    download(
        ctx,
        'http://downloads.xiph.org/releases/speex/%s-%s.tar.gz' % (repo, ctx.pkg_ver),
        '%s-%s.tar.gz' % (repo, ctx.pkg_ver))
    unpack(
        ctx,
        '%s-%s.tar.gz' % (repo, ctx.pkg_ver),
        '%s-%s' % (repo, ctx.pkg_ver))
    os.chdir('src/%s-%s' % (repo, ctx.pkg_ver))
    execute(ctx, './configure --host=%s %s %s %s' % (
        ctx.toolchain,
        format_vars(ctx),
        format_flags(ctx, cflags='-fPIC'),
        ' '.join([
            '--disable-examples',
            '--disable-shared',
            '--enable-static',
           ])))
    execute_make(ctx)
    install_tree(ctx, 'include', ctx.pkg_inc_dir)
    install_files(ctx, 'lib%s/.libs/libspeexdsp.a' % repo, ctx.pkg_lib_dir)
elif ctx.pkg_name == 'alsa':
    download(
        ctx,
        'ftp://ftp.alsa-project.org/pub/lib/alsa-lib-%s.tar.bz2' % ctx.pkg_ver,
        'alsa-lib-%s.tar.bz2' % ctx.pkg_ver)
    unpack(ctx,
           'alsa-lib-%s.tar.bz2' % ctx.pkg_ver,
           'alsa-lib-%s' % ctx.pkg_ver)
    os.chdir('src/alsa-lib-%s' % ctx.pkg_ver)
    execute(ctx, './configure --host=%s %s %s' % (
        ctx.toolchain,
        format_vars(ctx),
        ' '.join([
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
elif ctx.pkg_name == 'ltdl':
    download(
        ctx,
        'ftp://ftp.gnu.org/gnu/libtool/libtool-%s.tar.gz' % ctx.pkg_ver,
        'libtool-%s.tar.gz' % ctx.pkg_ver)
    unpack(
        ctx,
        'libtool-%s.tar.gz' % ctx.pkg_ver,
        'libtool-%s' % ctx.pkg_ver)
    os.chdir('src/libtool-%s' % ctx.pkg_ver)
    execute(ctx, './configure --host=%s %s %s' % (
        ctx.toolchain,
        format_vars(ctx),
        ' '.join([
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
        'https://github.com/json-c/json-c/archive/json-c-%s.tar.gz' % ctx.pkg_ver,
        'json-c-%s.tar.gz' % ctx.pkg_ver)
    unpack(
        ctx,
        'json-c-%s.tar.gz' % ctx.pkg_ver,
        'json-c-json-c-%s' % ctx.pkg_ver)
    os.chdir('src/json-c-json-c-%s' % ctx.pkg_ver)
    execute(ctx, '%s --host=%s %s %s %s' % (
        ' '.join(filter(None, [
            # workaround for outdated config.sub
            'ac_cv_host=%s' % ctx.toolchain if ctx.toolchain else '',
            # disable rpl_malloc and rpl_realloc
            'ac_cv_func_malloc_0_nonnull=yes',
            'ac_cv_func_realloc_0_nonnull=yes',
            # configure
            './configure',
        ])),
        ctx.toolchain,
        format_vars(ctx),
        format_flags(ctx, cflags='-w -fPIC -fvisibility=hidden'),
        ' '.join([
            '--enable-static',
            '--disable-shared',
        ])))
    execute_make(ctx, cpu_count=0) # -j is buggy for json-c
    install_tree(ctx, '.', ctx.pkg_inc_dir, include=['*.h'])
    install_files(ctx, '.libs/libjson-c.a', ctx.pkg_lib_dir)
elif ctx.pkg_name == 'sndfile':
    download(
        ctx,
        'http://www.mega-nerd.com/libsndfile/files/libsndfile-%s.tar.gz' % ctx.pkg_ver,
        'libsndfile-%s.tar.gz' % ctx.pkg_ver)
    unpack(
        ctx,
        'libsndfile-%s.tar.gz' % ctx.pkg_ver,
        'libsndfile-%s' % ctx.pkg_ver)
    os.chdir('src/libsndfile-%s' % ctx.pkg_ver)
    execute(ctx, '%s --host=%s %s %s %s' % (
        ' '.join(filter(None, [
            # workaround for outdated config.sub
            'ac_cv_host=%s' % ctx.toolchain if ctx.toolchain else '',
            # configure
            './configure',
        ])),
        ctx.toolchain,
        format_vars(ctx),
        # explicitly enable -pthread because libtool doesn't add it on some platforms
        format_flags(ctx, cflags='-fPIC -fvisibility=hidden', pthread=True),
        ' '.join([
            '--disable-external-libs',
            '--disable-shared',
            '--enable-static',
        ])))
    execute_make(ctx)
    install_files(ctx, 'src/sndfile.h', ctx.pkg_inc_dir)
    install_files(ctx, 'src/.libs/libsndfile.a', ctx.pkg_lib_dir)
elif ctx.pkg_name == 'pulseaudio':
    download(
        ctx,
        'https://freedesktop.org/software/pulseaudio/releases/pulseaudio-%s.tar.gz' % ctx.pkg_ver,
        'pulseaudio-%s.tar.gz' % ctx.pkg_ver)
    unpack(
        ctx,
        'pulseaudio-%s.tar.gz' % ctx.pkg_ver,
        'pulseaudio-%s' % ctx.pkg_ver)
    pa_ver = tuple(map(int, ctx.pkg_ver.split('.')))
    if (8, 99, 1) <= pa_ver < (11, 99, 1):
        apply_patch(
            ctx,
            'pulseaudio-%s' % ctx.pkg_ver,
            'https://bugs.freedesktop.org/attachment.cgi?id=136927',
            '0001-memfd-wrappers-only-define-memfd_create-if-not-alrea.patch')
    if pa_ver < (12, 99, 1):
        subst_tree('src/pulseaudio-%s' % ctx.pkg_ver, ['*.h', '*.c'],
                   from_='#include <asoundlib.h>',
                   to='#include <alsa/asoundlib.h>')
    os.chdir('src/pulseaudio-%s' % ctx.pkg_ver)
    if pa_ver < (14, 99, 1):
        # workaround for "missing acolocal-1.15" and "missing automake-1.15" errors
        # on some systems; since we're not modifying any autotools stuff, it's safe
        # to replace corresponding commands with "true" command
        if os.path.exists('Makefile.in'):
            subst_files('Makefile.in', '@ACLOCAL@', 'true')
            subst_files('Makefile.in', '@AUTOMAKE@', 'true')
        execute(ctx, './configure --host=%s %s %s %s %s' % (
            ctx.toolchain,
            format_vars(ctx),
            format_flags(ctx, cflags='-w -fomit-frame-pointer -O2'),
            ' '.join([
                'LIBJSON_CFLAGS=" "',
                'LIBJSON_LIBS="-ljson-c"',
                'LIBSNDFILE_CFLAGS=" "',
                'LIBSNDFILE_LIBS="-lsndfile"',
            ]),
            ' '.join([
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
        os.chdir('builddir')
        mkpath('pcdir')
        generate_pc_files(ctx, 'pcdir')
        generate_meson_crossfile(ctx, 'pcdir', 'crossfile.txt')
        execute(ctx, 'meson .. %s' % (
            ' '.join([
                '--cross-file=crossfile.txt',
                '-Ddaemon=false',
                '-Ddoxygen=false',
                '-Dgcov=false',
                '-Dman=false',
                '-Dtests=false',
            ])))
        execute(ctx, 'ninja')
        execute(ctx, 'DESTDIR=../instdir ninja install')
        os.chdir('..')
        install_tree(ctx, 'instdir/usr/local/include/pulse',
                     os.path.join(ctx.pkg_inc_dir, 'pulse'),
                     include=['*.h'])
        install_files(ctx, 'builddir/src/pulse/libpulse.so', ctx.pkg_lib_dir)
        install_files(ctx, 'builddir/src/pulse/libpulse.so.0', ctx.pkg_rpath_dir)
        install_files(ctx, 'builddir/src/pulse/libpulse-simple.so', ctx.pkg_lib_dir)
        install_files(ctx, 'builddir/src/pulse/libpulse-simple.so.0', ctx.pkg_rpath_dir)
        install_files(ctx, 'builddir/src/libpulsecommon-*.so', ctx.pkg_lib_dir)
        install_files(ctx, 'builddir/src/libpulsecommon-*.so', ctx.pkg_rpath_dir)
elif ctx.pkg_name == 'sox':
    download(
        ctx,
        'https://downloads.sourceforge.net/project/sox/sox/%s/sox-%s.tar.gz' % (ctx.pkg_ver, ctx.pkg_ver),
        'sox-%s.tar.gz' % ctx.pkg_ver)
    unpack(
        ctx,
        'sox-%s.tar.gz' % ctx.pkg_ver,
        'sox-%s' % ctx.pkg_ver)
    os.chdir('src/sox-%s' % ctx.pkg_ver)
    execute(ctx, './configure --host=%s %s %s %s' % (
        ctx.toolchain,
        format_vars(ctx),
        format_flags(ctx, cflags='-fvisibility=hidden'),
        ' '.join([
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
elif ctx.pkg_name == 'openssl':
    download(
        ctx,
        'https://www.openssl.org/source/openssl-%s.tar.gz' % ctx.pkg_ver,
        'openssl-%s.tar.gz' % ctx.pkg_ver)
    unpack(
        ctx,
        'openssl-%s.tar.gz' % ctx.pkg_ver,
        'openssl-%s' % ctx.pkg_ver)
    os.chdir('src/openssl-%s' % ctx.pkg_ver)
    # see https://github.com/openssl/openssl/blob/master/INSTALL.md#configuration-options
    execute(ctx, '%s %s ./Configure %s %s %s' % (
        format_vars(ctx),
        format_flags(ctx),
        detect_openssl_platform(ctx.toolchain), # (should be the first argument)
        '--debug' if ctx.variant == 'debug' else '--release',
        ' '.join([
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
elif ctx.pkg_name == 'gengetopt':
    download(
        ctx,
        'ftp://ftp.gnu.org/gnu/gengetopt/gengetopt-%s.tar.gz' % ctx.pkg_ver,
        'gengetopt-%s.tar.gz' % ctx.pkg_ver)
    unpack(
        ctx,
        'gengetopt-%s.tar.gz' % ctx.pkg_ver,
        'gengetopt-%s' % ctx.pkg_ver)
    os.chdir('src/gengetopt-%s' % ctx.pkg_ver)
    execute(ctx, './configure', clear_env=True)
    execute_make(ctx, cpu_count=0) # -j is buggy for gengetopt
    install_files(ctx, 'src/gengetopt', ctx.pkg_bin_dir)
elif ctx.pkg_name == 'ragel':
    download(
        ctx,
        'https://www.colm.net/files/ragel/ragel-%s.tar.gz' % ctx.pkg_ver,
        'ragel-%s.tar.gz' % ctx.pkg_ver)
    unpack(
        ctx,
        'ragel-%s.tar.gz' % ctx.pkg_ver,
        'ragel-%s' % ctx.pkg_ver)
    os.chdir('src/ragel-%s' % ctx.pkg_ver)
    execute(ctx, './configure', clear_env=True)
    execute_make(ctx)
    install_files(ctx, 'ragel/ragel', ctx.pkg_bin_dir)
elif ctx.pkg_name == 'cpputest':
    download(
        ctx,
        'https://github.com/cpputest/cpputest/releases/download/v%s/cpputest-%s.tar.gz' % (
            ctx.pkg_ver, ctx.pkg_ver),
        'cpputest-%s.tar.gz' % ctx.pkg_ver)
    unpack(
        ctx,
        'cpputest-%s.tar.gz' % ctx.pkg_ver,
        'cpputest-%s' % ctx.pkg_ver)
    os.chdir('src/cpputest-%s' % ctx.pkg_ver)
    execute(ctx, './configure --host=%s %s %s %s' % (
            ctx.toolchain,
            format_vars(ctx),
            # disable warnings, since CppUTest uses -Werror and may fail to
            # build on old GCC versions
            format_flags(ctx, cflags='-w'),
            ' '.join([
                '--enable-static',
                # disable memory leak detection which is too hard to use properly
                '--disable-memory-leak-detection',
            ])))
    execute_make(ctx)
    install_tree(ctx, 'include', ctx.pkg_inc_dir)
    install_files(ctx, 'lib/libCppUTest.a', ctx.pkg_lib_dir)
elif ctx.pkg_name == 'google-benchmark':
    download(
        ctx,
        'https://github.com/google/benchmark/archive/v%s.tar.gz' % ctx.pkg_ver,
        'benchmark_v%s.tar.gz' % ctx.pkg_ver)
    unpack(
        ctx,
        'benchmark_v%s.tar.gz' % ctx.pkg_ver,
        'benchmark-%s' % ctx.pkg_ver)
    os.chdir('src/benchmark-%s' % ctx.pkg_ver)
    mkpath('build')
    os.chdir('build')
    execute_cmake(
        ctx,
        '..',
        args=[
            '-DBENCHMARK_ENABLE_GTEST_TESTS=OFF',
            '-DCMAKE_CXX_FLAGS=-w',
            ])
    execute_make(ctx)
    os.chdir('..')
    install_tree(ctx, 'include', ctx.pkg_inc_dir, include=['*.h'])
    install_files(ctx, 'build/src/libbenchmark.a', ctx.pkg_lib_dir)
# end of deps
else:
    die("unknown 3rdparty '{}'", ctx.pkg)

touch(ctx.commit_file)
