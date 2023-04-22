from __future__ import print_function

import sys
import os
import os.path
import re
import shutil
import glob
import fnmatch
import ssl
import tarfile
import fileinput
import subprocess
import multiprocessing

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

#
# Variables for helpers.
#

printdir = os.path.abspath('.')
devnull = open(os.devnull, 'w+')

#
# Helpers.
#

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

def fetch_vendored(url, path, log, vendordir):
    for subdir, _, _ in os.walk(vendordir):
        distfile = os.path.join(subdir, os.path.basename(path))
        if os.path.exists(distfile):
            print('[found vendored] %s' % os.path.basename(distfile))
            with open(distfile, 'rb') as rp:
                with open(path, 'wb') as wp:
                    wp.write(rp.read())
                    return
    raise

def fetch_urlopen(url, path, log, vendordir):
    rp = urlopen(url)
    with open(path, 'wb') as wp:
        wp.write(rp.read())

def fetch_tool(url, path, log, vendordir, tool, cmd):
    print('[trying %s] %s' % (tool, url))
    with open(log, 'a+') as fp:
        print('>>> %s' % cmd, file=fp)
    if os.system(cmd) != 0:
        raise

def fetch_wget(url, path, log, vendordir):
    fetch_tool(url, path, log, vendordir, 'wget', 'wget "%s" --quiet -O "%s"' % (url, path))

def fetch_curl(url, path, log, vendordir):
    fetch_tool(url, path, log, vendordir, 'curl', 'curl -Ls "%s" -o "%s"' % (url, path))

def download(url, name, log, vendordir):
    path_res = 'src/' + name
    path_tmp = 'tmp/' + name

    if os.path.exists(path_res):
        print('[found downloaded] %s' % name)
        return

    rmpath(path_res)
    rmpath(path_tmp)
    mkpath('tmp')

    error = None
    for fn in [fetch_vendored, fetch_urlopen, fetch_curl, fetch_wget]:
        try:
            fn(url, path_tmp, log, vendordir)
            shutil.move(path_tmp, path_res)
            rm_emptydir('tmp')
            return
        except Exception as e:
            if fn == fetch_vendored:
                print('[download] %s' % url)
            if fn == fetch_urlopen:
                error = e

    print("error: can't download '%s': %s" % (url, error), file=sys.stderr)
    exit(1)

def unpack(filename, dirname):
    dirname_res = 'src/' + dirname
    dirname_tmp = 'tmp/' + dirname

    if os.path.exists(dirname_res):
        print('[found unpacked] %s' % dirname)
        return

    print('[unpack] %s' % filename)

    rmpath(dirname_res)
    rmpath(dirname_tmp)
    mkpath('tmp')

    tar = tarfile.open('src/'+filename, 'r')
    tar.extractall('tmp')
    tar.close()

    shutil.move(dirname_tmp, dirname_res)
    rm_emptydir('tmp')

def which(tool):
    proc = subprocess.Popen(['which', tool], stdout=subprocess.PIPE, stderr=devnull)
    out = read_stdout(proc).strip()
    if out:
        return out

def try_execute(cmd):
    return subprocess.call(
        cmd, stdout=devnull, stderr=subprocess.STDOUT, shell=True) == 0

def execute(cmd, log, ignore_error=False, clear_env=False):
    print('[execute] %s' % cmd)

    with open(log, 'a+') as fp:
        print('>>> %s' % cmd, file=fp)

    env = None
    if clear_env:
        env = {'HOME': os.environ['HOME'], 'PATH': os.environ['PATH']}

    code = subprocess.call('%s >>%s 2>&1' % (cmd, log), shell=True, env=env)
    if code != 0:
        if ignore_error:
            with open(log, 'a+') as fp:
                print('command exited with status %s' % code, file=fp)
        else:
            exit(1)

def execute_make(log, cpu_count=None):
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

    execute(' '.join(cmd), log)

def execute_cmake(srcdir, variant, toolchain, env, log, args=None):
    if not args:
        args = []

    compiler = getvar(env, 'CC', toolchain, 'gcc')
    sysroot = getsysroot(toolchain, compiler)

    need_sysroot = bool(sysroot)
    need_tools = True

    # cross-compiling for yocto linux
    if 'OE_CMAKE_TOOLCHAIN_FILE' in os.environ:
        need_tools = False

    # cross-compiling for android
    if 'android' in toolchain:
        args += ['-DCMAKE_SYSTEM_NAME=Android']

        api = detect_android_api(compiler)
        abi = detect_android_abi(toolchain)

        toolchain_file = find_android_toolchain_file(compiler)

        if toolchain_file:
            need_sysroot = False
            need_tools = False
            args += [quote('-DCMAKE_TOOLCHAIN_FILE=%s' % toolchain_file)]
            if api:
                args += ['-DANDROID_NATIVE_API_LEVEL=%s' % api]
            if abi:
                args += ['-DANDROID_ABI=%s' % abi]
        else:
            sysroot = find_android_sysroot(compiler)
            need_sysroot = bool(sysroot)
            need_tools = True
            if api:
                args += ['-DCMAKE_SYSTEM_VERSION=%s' % api]
            if abi:
                args += ['-DCMAKE_ANDROID_ARCH_ABI=%s' % abi]

    if need_sysroot:
        args += [
            '-DCMAKE_FIND_ROOT_PATH=%s' % quote(sysroot),
            '-DCMAKE_SYSROOT=%s'        % quote(sysroot),
        ]

    if need_tools:
        if not 'android' in toolchain:
            args += [
                '-DCMAKE_C_COMPILER=%s' % quote(getpath(compiler)),
            ]
        args += [
            '-DCMAKE_LINKER=%s' % quote(getpath(getvar(env, 'CCLD', toolchain, 'gcc'))),
            '-DCMAKE_AR=%s'     % quote(getpath(getvar(env, 'AR', toolchain, 'ar'))),
            '-DCMAKE_RANLIB=%s' % quote(getpath(getvar(env, 'RANLIB', toolchain, 'ranlib'))),
        ]

    cc_flags = [
        '-fPIC', # -fPIC should be set explicitly in older cmake versions
        '-fvisibility=hidden', # hide private symbols
    ]

    if variant == 'debug':
        cc_flags += [
            '-ggdb', # -ggdb is required for sanitizer backtrace
        ]
        args += [
            '-DCMAKE_BUILD_TYPE=Debug',
            '-DCMAKE_C_FLAGS_DEBUG:STRING=%s' % quote(' '.join(cc_flags)),
        ]
    else:
        args += [
            '-DCMAKE_BUILD_TYPE=Release',
            '-DCMAKE_C_FLAGS_RELEASE:STRING=%s' % quote(' '.join(cc_flags)),
        ]

    args += [
        '-DCMAKE_POSITION_INDEPENDENT_CODE=ON',
    ]

    execute('cmake ' + srcdir + ' ' + ' '.join(args), log)

def install_tree(src, dst, match=None, ignore=None):
    print('[install] %s' % os.path.relpath(dst, printdir))

    def match_patterns(src, names):
        ignorenames = []
        for n in names:
            if os.path.isdir(os.path.join(src, n)):
                continue
            matched = False
            for m in match:
                if fnmatch.fnmatch(n, m):
                    matched = True
                    break
            if not matched:
                ignorenames.append(n)
        return set(ignorenames)

    if match:
        ignorefn = match_patterns
    elif ignore:
        ignorefn = shutil.ignore_patterns(*ignore)
    else:
        ignorefn = None

    mkpath(os.path.dirname(dst))
    rmpath(dst)
    shutil.copytree(src, dst, ignore=ignorefn)

def install_files(src, dst):
    print('[install] %s' % os.path.join(
        os.path.relpath(dst, printdir),
        os.path.basename(src)))

    for f in glob.glob(src):
        mkpath(dst)
        shutil.copy(f, dst)

def replace_files(path, from_, to):
    print('[patch] %s' % path)
    for line in fileinput.input(path, inplace=True):
        print(line.replace(from_, to), end='')

def replace_tree(dirpath, filepats, from_, to):
    def match(path):
        try:
            with open(path) as fp:
                for line in fp:
                    if from_ in line:
                        return True
        except:
            pass

    for filepat in filepats:
        for root, dirnames, filenames in os.walk(dirpath):
            for filename in fnmatch.filter(filenames, filepat):
                filepath = os.path.join(root, filename)
                if match(filepath):
                    replace_files(filepath, from_, to)

def try_patch(dirname, patchurl, patchname, logfile, vendordir):
    if not try_execute('patch --version'):
        return
    download(patchurl, patchname, logfile, vendordir)
    execute('patch -p1 -N -d %s -i %s' % (
        'src/' + dirname,
        '../' + patchname), logfile, ignore_error=True)

def touch(path):
    open(path, 'w').close()

def traverse_parents(path, search_file):
    while True:
        parent = os.path.dirname(path)
        if parent == path:
            break # root
        path = parent
        child_path = os.path.join(path, search_file)
        if os.path.exists(child_path):
            return child_path

def read_stdout(proc):
    out = proc.stdout.read()
    try:
        out = out.decode()
    except:
        pass
    try:
        out = str(out)
    except:
        pass
    return out

def getvar(env, var, toolchain, default):
    if var in env:
        return env[var]
    return '-'.join([s for s in [toolchain, default] if s])

def getpath(tool):
    if '/' in tool:
        return tool
    p = which(tool)
    if not p:
        return tool
    return p

def getsysroot(toolchain, compiler):
    if not toolchain:
        return ""

    if not compiler:
        compiler = '%s-gcc' % toolchain

    try:
        cmd = [compiler, '-print-sysroot']
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=devnull)
        sysroot = read_stdout(proc).strip()
        if os.path.isdir(sysroot):
            return sysroot
    except:
        pass

    return None

def find_android_toolchain_file(compiler):
    compiler_exe = which(compiler)
    if compiler_exe:
        return traverse_parents(compiler_exe, 'build/cmake/android.toolchain.cmake')

def find_android_sysroot(compiler):
    compiler_exe = which(compiler)
    if compiler_exe:
        return traverse_parents(compiler_exe, 'sysroot')

def detect_android_abi(toolchain):
    try:
        ta = toolchain.split('-')[0]
    except:
        return
    if ta == 'arm' or ta == 'armv7a':
        return 'armeabi-v7a'
    if ta == 'aarch64':
        return 'arm64-v8a'
    if ta == 'i686':
        return 'x86'
    if ta == 'x86_64':
        return 'x86_64'
    return ta

def detect_android_api(compiler):
    try:
        cmd = [compiler, '-dM', '-E', '-']
        proc = subprocess.Popen(cmd, stdin=devnull, stdout=subprocess.PIPE, stderr=devnull)
        for line in read_stdout(proc).splitlines():
            m = re.search(r'__ANDROID_API__\s+(\d+)', line)
            if m:
                return m.group(1)
    except:
        pass

def checkfamily(env, toolchain, family):
    if family == 'gcc':
        keys = ['GNU', 'gnu', 'gcc', 'g++']
    elif family == 'clang':
        keys = ['clang']

    def _checktool(toolchain, tool):
        if toolchain:
            tool = '%s-%s' % (toolchain, tool)
        try:
            proc = subprocess.Popen([tool, '-v'],
                                        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            out = str(read_stdout(proc).strip())
            for k in keys:
                if k in out:
                    return True
        except:
            pass
        return False

    for var in ['CC', 'CCLD', 'CXX', 'CXXLD']:
        if var in env:
            if not _checktool('', env[var]):
                return False

    if not 'gnu' in toolchain:
        if 'CC' not in env:
            for tool in ['cc', 'gcc', 'clang']:
                if _checktool(toolchain, tool):
                    break
            else:
                return False

        if 'CCLD' not in env:
            for tool in ['ld', 'gcc', 'clang']:
                if _checktool(toolchain, tool):
                    break
            else:
                return False

        if 'CXX' not in env or 'CXXLD' not in env:
            for tool in ['g++', 'clang++']:
                if _checktool(toolchain, tool):
                    break
            else:
                return False

    return True

def makeflags(workdir, toolchain, env, deplist, cflags='', ldflags='', variant='', pthread=False):
    incdirs=[]
    libdirs=[]
    rpathdirs=[]

    for dep in deplist:
        incdirs += [os.path.join(workdir, dep, 'include')]
        libdirs += [os.path.join(workdir, dep, 'lib')]
        rpathdirs += [os.path.join(workdir, dep, 'rpath')]

    cflags = ([cflags] if cflags else []) + ['-I%s' % path for path in incdirs]
    ldflags = ['-L%s' % path for path in libdirs] + ([ldflags] if ldflags else [])

    is_android = 'android' in toolchain
    is_gnu = checkfamily(env, toolchain, 'gcc')
    is_clang = checkfamily(env, toolchain, 'clang')

    if variant == 'debug':
        if is_gnu or is_clang:
            cflags += ['-ggdb']
        else:
            cflags += ['-g']
    elif variant == 'release':
        cflags += ['-O2']

    if pthread and not is_android:
        if is_gnu or is_clang:
            cflags += ['-pthread']
        if is_gnu:
            ldflags += ['-pthread']
        else:
            ldflags += ['-lpthread']

    if is_gnu:
        ldflags += ['-Wl,-rpath-link=%s' % path for path in rpathdirs]

    return ' '.join([
        'CXXFLAGS=%s' % quote(' '.join(cflags)),
        'CFLAGS=%s'   % quote(' '.join(cflags)),
        'LDFLAGS=%s'  % quote(' '.join(ldflags)),
    ])

def makeenv(envlist):
    ret = []
    for e in envlist:
        ret.append(quote(e))
    return ' '.join(ret)

# https://mesonbuild.com/Reference-tables.html
def gen_crossfile(env, toolchain, pc_dir, cross_file):
    print('[generate] %s' % cross_file)

    def meson_string(s):
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

    def meson_flags(flags):
        return '[%s]' % ', '.join(map(meson_string, flags))

    if toolchain:
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
            if s in toolchain:
                system = s
                break

        for s in reversed(sorted(cpu_list, key=len)):
            if s in toolchain:
                cpu_family = s

                if s == 'aarch64':
                    cpu = 'armv8'
                elif s == 'arm':
                    cpu = 'armv5'
                elif s == 'x86':
                    cpu = 'i686'
                else:
                    cpu = s

                if s+'le' in toolchain or s+'el' in toolchain:
                    endian = 'little'
                elif s+'be' in toolchain or s+'eb' in toolchain:
                    endian = 'big'
                elif s in 'wasm32 wasm64 x86 x86_64'.split():
                    endian = 'little'
                else:
                    endian = 'big'

                break

    cflags = []
    ldflags = []

    for dep in deplist:
        cflags.append('-I%s' % os.path.join(workdir, dep, 'include'))
        ldflags.append('-L%s' % os.path.join(workdir, dep, 'lib'))

    pkg_config = None
    if 'PKG_CONFIG' in env:
        pkg_config = env['PKG_CONFIG']
    elif which('pkg-config'):
        pkg_config = 'pkg-config'

    with open(cross_file, 'w') as fp:
        fp.write("[binaries]\n")
        fp.write("c = %s\n" % meson_string(env['CC']))
        fp.write("cpp = %s\n" % meson_string(env['CXX']))
        fp.write("ar = %s\n" % meson_string(env['AR']))
        if pkg_config:
            fp.write("pkgconfig = %s\n" % meson_string(pkg_config))
        fp.write("\n")

        if pc_dir:
            fp.write("[built-in options]\n")
            fp.write("pkg_config_path = %s\n" % meson_string(os.path.abspath(pc_dir)))
            fp.write("\n")

        if cflags or ldflags:
            fp.write("[properties]\n")
            fp.write("c_args = %s\n" % meson_flags(cflags))
            fp.write("c_link_args = %s\n" % meson_flags(ldflags))
            fp.write("cpp_args = %s\n" % meson_flags(cflags))
            fp.write("cpp_link_args = %s\n" % meson_flags(ldflags))
            fp.write("\n")

        if toolchain:
            fp.write("[host_machine]\n")
            fp.write("system = %s\n" % meson_string(system))
            fp.write("cpu_family = %s\n" % meson_string(cpu_family))
            fp.write("cpu = %s\n" % meson_string(cpu))
            fp.write("endian = %s\n" % meson_string(endian))
            fp.write("\n")

def gen_pcfile(dep, pc_dir):
    name, ver = splitver(dep)
    pc_file = os.path.join(pc_dir, name) + '.pc'
    with open(pc_file, 'w') as fp:
        print('[generate] %s' % pc_file)
        fp.write('Name: %s\n' % name)
        fp.write('Description: %s\n' % name)
        fp.write('Version: %s\n' % ver)
        fp.write('Cflags: -I%s\n' % os.path.join(workdir, dep, 'include'))
        fp.write('Libs: -L%s' % os.path.join(workdir, dep, 'lib'))
        for lib in glob.glob(os.path.join(workdir, dep, 'lib', 'lib*')):
            lib = os.path.basename(lib)
            lib = re.sub('^lib', '', lib)
            lib = re.sub('\.[a-z]+$', '', lib)
            fp.write(' -l%s' % lib)
        fp.write('\n')

def splitver(dep):
    m = re.match('^(.*?)-([0-9][a-z0-9.-]+)$', dep)
    if not m:
        print("error: can't determine version of '%s'" % dep, file=sys.stderr)
        exit(1)
    return m.group(1), m.group(2)

def splitenv(envlist):
    env = dict()
    for e in envlist:
        k, v = e.split('=', 1)
        env[k] = v
    return env

# Guess platform argument for OpenSSL's Configure script basing on toolchain string.
# (see `./Configure LIST` for a list of all platforms and their format)
# See also openssl/Configurations/[0-9]*.conf
def openssl_get_platform(toolchain):
    """
    >>> openssl_get_platform('x86_64-pc-linux-gnu')
    'linux-x86_64'
    >>> openssl_get_platform('aarch64-linux-gnu')
    'linux-aarch64'
    >>> openssl_get_platform('mips-openwrt-linux-uclibc')
    'linux-mips32'
    >>> openssl_get_platform('arm-linux-gnueabihf')
    'linux-generic32'
    >>> openssl_get_platform('arm-bcm2708hardfp-linux-gnueabi')
    'linux-generic32'
    >>> openssl_get_platform('armv7a-linux-androideabi33')
    'android-arm'
    >>> openssl_get_platform('mips64-linux-musl')
    'linux64-mips64'
    >>> openssl_get_platform('arm-linux-androideabi')
    'android-arm'
    >>> openssl_get_platform('aarch64-linux-android')
    'android-arm64'
    >>> openssl_get_platform('aarch64-linux-android29')
    'android-arm64'
    >>> openssl_get_platform('i686-linux-android')
    'android-x86'
    >>> openssl_get_platform('x86_64-linux-android')
    'android-x86_64'
    >>> openssl_get_platform('i686-pc-linux-gnu')
    'linux-x86'

    # other edge cases
    >>> openssl_get_platform('mingw-w64-x86_64')
    'mingw64'
    >>> openssl_get_platform('x86_64-pc-cygwin')
    'Cygwin-x86_64'
    >>> openssl_get_platform('')
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

#
# Main.
#

# for tests run `python scripts/scons_helpers/build-3rdparty.py run-tests`
if len(sys.argv) == 2 and sys.argv[1] == 'run-tests':
    import doctest
    doctest.testmod()
    exit(0)

if len(sys.argv) < 7:
    print("error: usage: 3rdparty.py workdir vendordir toolchain variant package deplist [env]",
          file=sys.stderr)
    exit(1)

workdir = os.path.abspath(sys.argv[1])
vendordir = os.path.abspath(sys.argv[2])

# Build toolchain, e.g. 'x86_64-pc-linux-gnu'; may be empty string.
#
# Our agreements for arch component:
# - aarch64             -- ARM 64 bit
# - arm*                -- ARM 32 bit
# - mipsel              -- MIPS little endian
# - mips (without 'el') -- MIPS big endian
toolchain = sys.argv[3]

variant = sys.argv[4]            # 'debug' or 'release' variant should be built
fullname = sys.argv[5]           # package's name and version, e.g. 'openssl-3.0.7-rc1'
deplist = sys.argv[6].split(':') # list of 3rdparty dependencies names, e.g. 'ltdl:json-c'
envlist = sys.argv[7:]           # build env variables, e.g. 'CC=gcc CXX=g++ LDFLAGS=...'

if variant not in ['debug', 'release']:
    print("error: argument 'variant' should be 'debug' or 'release'")
    exit(1)

env = splitenv(envlist)
name, ver = splitver(fullname)

build_dir = os.path.join(workdir, fullname)
src_dir = os.path.join(build_dir, 'src')
bin_dir = os.path.join(build_dir, 'bin')
lib_dir = os.path.join(build_dir, 'lib')
inc_dir = os.path.join(build_dir, 'include')
rpath_dir = os.path.join(build_dir, 'rpath')

logfile = os.path.join(build_dir, 'build.log')
commit_build_finished_file = os.path.join(build_dir, 'commit')

rmpath(commit_build_finished_file)
mkpath(src_dir)

os.chdir(build_dir)

#
# Recipes for particular targets.
#

if name == 'libuv':
    download('https://dist.libuv.org/dist/v%s/libuv-v%s.tar.gz' % (ver, ver),
             'libuv-v%s.tar.gz' % ver,
             logfile,
             vendordir)
    unpack('libuv-v%s.tar.gz' % ver,
            'libuv-v%s' % ver)
    os.chdir('src/libuv-v%s' % ver)
    replace_files('include/uv.h', '__attribute__((visibility("default")))', '')
    if 'android' in toolchain:
        mkpath('build')
        os.chdir('build')
        execute_cmake('..', variant, toolchain, env, logfile, args=[
            '-DLIBUV_BUILD_TESTS=OFF',
            '-DANDROID_PLATFORM=android-21',
            ])
        execute_make(logfile)
        shutil.copy('libuv_a.a', 'libuv.a')
        os.chdir('..')
        install_files('build/libuv.a', lib_dir)
    else:
        execute('./autogen.sh', logfile)
        execute('./configure --host=%s %s %s %s' % (
            toolchain,
            makeenv(envlist),
            makeflags(workdir, toolchain, env, [], cflags='-fvisibility=hidden'),
            ' '.join([
                '--with-pic',
                '--enable-static',
            ])), logfile)
        execute_make(logfile)
        install_files('.libs/libuv.a', lib_dir)
    install_tree('include', inc_dir)
elif name == 'libunwind':
    download(
        'http://download.savannah.nongnu.org/releases/libunwind/libunwind-%s.tar.gz' % ver,
        'libunwind-%s.tar.gz' % ver,
        logfile,
        vendordir)
    unpack('libunwind-%s.tar.gz' % ver,
            'libunwind-%s' % ver)
    os.chdir('src/libunwind-%s' % ver)
    execute('./configure --host=%s %s %s %s' % (
        toolchain,
        makeenv(envlist),
        makeflags(workdir, toolchain, env, deplist, cflags='-fcommon -fPIC', variant=variant),
        ' '.join([
            '--disable-coredump',
            '--disable-minidebuginfo',
            '--disable-ptrace',
            '--disable-setjmp',
            '--disable-shared',
            '--enable-static',
           ])), logfile)
    execute_make(logfile)
    install_files('include/*.h', inc_dir)
    install_files('src/.libs/libunwind.a', lib_dir)
elif name == 'libatomic_ops':
    download(
        'https://github.com/ivmai/libatomic_ops/releases/download/v%s/libatomic_ops-%s.tar.gz' % (
            ver, ver),
        'libatomic_ops-%s.tar.gz' % ver,
        logfile,
        vendordir)
    unpack('libatomic_ops-%s.tar.gz' % ver,
            'libatomic_ops-%s' % ver)
    os.chdir('src/libatomic_ops-%s' % ver)
    execute('./configure --host=%s %s %s %s' % (
        toolchain,
        makeenv(envlist),
        makeflags(workdir, toolchain, env, deplist, cflags='-fPIC', variant=variant),
        ' '.join([
            '--disable-docs',
            '--disable-shared',
            '--enable-static',
           ])), logfile)
    execute_make(logfile)
    install_tree('src', inc_dir, match=['*.h'])
    install_files('src/.libs/libatomic_ops.a', lib_dir)
elif name == 'openfec':
    if variant == 'debug':
        dist = 'bin/Debug'
    else:
        dist = 'bin/Release'
    download(
      'https://github.com/roc-streaming/openfec/archive/v%s.tar.gz' % ver,
      'openfec_v%s.tar.gz' % ver,
        logfile,
        vendordir)
    unpack('openfec_v%s.tar.gz' % ver,
            'openfec-%s' % ver)
    os.chdir('src/openfec-%s' % ver)
    mkpath('build')
    os.chdir('build')
    execute_cmake('..', variant, toolchain, env, logfile, args=[
        '-DBUILD_STATIC_LIBS=ON',
        '-DDEBUG:STRING=%s' % ('ON' if variant == 'debug' else 'OFF'),
        ])
    execute_make(logfile)
    os.chdir('..')
    install_tree('src', inc_dir, match=['*.h'])
    install_files('%s/libopenfec.a' % dist, lib_dir)
elif name == 'speexdsp':
    if ver.split('.', 1) > ['1', '2'] and (
            not re.match('^1.2[a-z]', ver) or ver == '1.2rc3'):
        speex = 'speexdsp'
    else:
        speex = 'speex'
    download('http://downloads.xiph.org/releases/speex/%s-%s.tar.gz' % (speex, ver),
            '%s-%s.tar.gz' % (speex, ver),
            logfile,
            vendordir)
    unpack('%s-%s.tar.gz' % (speex, ver),
            '%s-%s' % (speex, ver))
    os.chdir('src/%s-%s' % (speex, ver))
    execute('./configure --host=%s %s %s %s' % (
        toolchain,
        makeenv(envlist),
        makeflags(workdir, toolchain, env, deplist, cflags='-fPIC', variant=variant),
        ' '.join([
            '--disable-examples',
            '--disable-shared',
            '--enable-static',
           ])), logfile)
    execute_make(logfile)
    install_tree('include', inc_dir)
    install_files('lib%s/.libs/libspeexdsp.a' % speex, lib_dir)
elif name == 'alsa':
    download(
      'ftp://ftp.alsa-project.org/pub/lib/alsa-lib-%s.tar.bz2' % ver,
        'alsa-lib-%s.tar.bz2' % ver,
        logfile,
        vendordir)
    unpack('alsa-lib-%s.tar.bz2' % ver,
            'alsa-lib-%s' % ver)
    os.chdir('src/alsa-lib-%s' % ver)
    execute('./configure --host=%s %s %s' % (
        toolchain,
        makeenv(envlist),
        ' '.join([
            '--disable-python',
            '--disable-static',
            '--enable-shared',
        ])), logfile)
    execute_make(logfile)
    install_tree('include/alsa',
            os.path.join(inc_dir, 'alsa'),
            ignore=['alsa'])
    install_files('src/.libs/libasound.so', lib_dir)
    install_files('src/.libs/libasound.so.*', rpath_dir)
elif name == 'ltdl':
    download(
      'ftp://ftp.gnu.org/gnu/libtool/libtool-%s.tar.gz' % ver,
        'libtool-%s.tar.gz' % ver,
        logfile,
        vendordir)
    unpack('libtool-%s.tar.gz' % ver,
            'libtool-%s' % ver)
    os.chdir('src/libtool-%s' % ver)
    execute('./configure --host=%s %s %s' % (
        toolchain,
        makeenv(envlist),
        ' '.join([
            '--disable-static',
            '--enable-shared',
        ])), logfile)
    execute_make(logfile)
    install_files('libltdl/ltdl.h', inc_dir)
    install_tree('libltdl/libltdl', os.path.join(inc_dir, 'libltdl'))
    install_files('libltdl/.libs/libltdl.so', lib_dir)
    install_files('libltdl/.libs/libltdl.so.*', rpath_dir)
elif name == 'json-c':
    download(
      'https://github.com/json-c/json-c/archive/json-c-%s.tar.gz' % ver,
        'json-c-%s.tar.gz' % ver,
        logfile,
        vendordir)
    unpack('json-c-%s.tar.gz' % ver,
            'json-c-json-c-%s' % ver)
    os.chdir('src/json-c-json-c-%s' % ver)
    execute('%s --host=%s %s %s %s' % (
        ' '.join(filter(None, [
            # workaround for outdated config.sub
            'ac_cv_host=%s' % toolchain if toolchain else '',
            # disable rpl_malloc and rpl_realloc
            'ac_cv_func_malloc_0_nonnull=yes',
            'ac_cv_func_realloc_0_nonnull=yes',
            # configure
            './configure',
        ])),
        toolchain,
        makeenv(envlist),
        makeflags(workdir, toolchain, env, [], cflags='-w -fPIC -fvisibility=hidden'),
        ' '.join([
            '--enable-static',
            '--disable-shared',
        ])), logfile)
    execute_make(logfile, cpu_count=0) # -j is buggy for json-c
    install_tree('.', inc_dir, match=['*.h'])
    install_files('.libs/libjson-c.a', lib_dir)
elif name == 'sndfile':
    download(
      'http://www.mega-nerd.com/libsndfile/files/libsndfile-%s.tar.gz' % ver,
        'libsndfile-%s.tar.gz' % ver,
        logfile,
        vendordir)
    unpack('libsndfile-%s.tar.gz' % ver,
            'libsndfile-%s' % ver)
    os.chdir('src/libsndfile-%s' % ver)
    execute('%s --host=%s %s %s %s' % (
        ' '.join(filter(None, [
            # workaround for outdated config.sub
            'ac_cv_host=%s' % toolchain if toolchain else '',
            # configure
            './configure',
        ])),
        toolchain,
        makeenv(envlist),
        # explicitly enable -pthread because libtool doesn't add it on some platforms
        makeflags(workdir, toolchain, env, [], cflags='-fPIC -fvisibility=hidden', pthread=True),
        ' '.join([
            '--disable-external-libs',
            '--disable-shared',
            '--enable-static',
        ])), logfile)
    execute_make(logfile)
    install_files('src/sndfile.h', inc_dir)
    install_files('src/.libs/libsndfile.a', lib_dir)
elif name == 'pulseaudio':
    download(
      'https://freedesktop.org/software/pulseaudio/releases/pulseaudio-%s.tar.gz' % ver,
        'pulseaudio-%s.tar.gz' % ver,
        logfile,
        vendordir)
    unpack('pulseaudio-%s.tar.gz' % ver,
            'pulseaudio-%s' % ver)
    pa_ver = tuple(map(int, ver.split('.')))
    if (8, 99, 1) <= pa_ver < (11, 99, 1):
        try_patch(
            'pulseaudio-%s' % ver,
            'https://bugs.freedesktop.org/attachment.cgi?id=136927',
            '0001-memfd-wrappers-only-define-memfd_create-if-not-alrea.patch',
            logfile,
            vendordir)
    if pa_ver < (12, 99, 1):
        replace_tree('src/pulseaudio-%s' % ver, ['*.h', '*.c'],
                      '#include <asoundlib.h>',
                      '#include <alsa/asoundlib.h>')
    os.chdir('src/pulseaudio-%s' % ver)
    if pa_ver < (14, 99, 1):
        # workaround for "missing acolocal-1.15" and "missing automake-1.15" errors
        # on some systems; since we're not modifying any autotools stuff, it's safe
        # to replace corresponding commands with "true" command
        if os.path.exists('Makefile.in'):
            replace_files('Makefile.in', '@ACLOCAL@', 'true')
            replace_files('Makefile.in', '@AUTOMAKE@', 'true')
        execute('./configure --host=%s %s %s %s %s' % (
            toolchain,
            makeenv(envlist),
            makeflags(workdir, toolchain, env, deplist, cflags='-w -fomit-frame-pointer -O2'),
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
            ])), logfile)
        execute_make(logfile)
        install_files('config.h', inc_dir)
        install_tree('src/pulse', os.path.join(inc_dir, 'pulse'),
                     match=['*.h'])
        install_files('src/.libs/libpulse.so', lib_dir)
        install_files('src/.libs/libpulse.so.0', rpath_dir)
        install_files('src/.libs/libpulse-simple.so', lib_dir)
        install_files('src/.libs/libpulse-simple.so.0', rpath_dir)
        install_files('src/.libs/libpulsecommon-*.so', lib_dir)
        install_files('src/.libs/libpulsecommon-*.so', rpath_dir)
    else:
        mkpath('builddir')
        os.chdir('builddir')
        mkpath('pc')
        for dep in deplist:
            gen_pcfile(dep, 'pc')
        gen_crossfile(env, toolchain, 'pc', 'crossfile.txt')
        execute('meson .. %s' % (
            ' '.join([
                '--cross-file=crossfile.txt',
                '-Ddaemon=false',
                '-Ddoxygen=false',
                '-Dgcov=false',
                '-Dman=false',
                '-Dtests=false',
            ])), logfile)
        execute('ninja', logfile)
        execute('DESTDIR=../instdir ninja install', logfile)
        os.chdir('..')
        install_tree('instdir/usr/local/include/pulse',
                     os.path.join(inc_dir, 'pulse'),
                     match=['*.h'])
        install_files('builddir/src/pulse/libpulse.so', lib_dir)
        install_files('builddir/src/pulse/libpulse.so.0', rpath_dir)
        install_files('builddir/src/pulse/libpulse-simple.so', lib_dir)
        install_files('builddir/src/pulse/libpulse-simple.so.0', rpath_dir)
        install_files('builddir/src/libpulsecommon-*.so', lib_dir)
        install_files('builddir/src/libpulsecommon-*.so', rpath_dir)
elif name == 'sox':
    download(
      'https://downloads.sourceforge.net/project/sox/sox/%s/sox-%s.tar.gz' % (ver, ver),
      'sox-%s.tar.gz' % ver,
        logfile,
        vendordir)
    unpack('sox-%s.tar.gz' % ver,
            'sox-%s' % ver)
    os.chdir('src/sox-%s' % ver)
    execute('./configure --host=%s %s %s %s' % (
        toolchain,
        makeenv(envlist),
        makeflags(workdir, toolchain, env, deplist, cflags='-fvisibility=hidden', variant=variant),
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
        ])), logfile)
    execute_make(logfile)
    install_files('src/sox.h', inc_dir)
    install_files('src/.libs/libsox.a', lib_dir)
elif name == 'openssl':
    archive = 'openssl-{}.tar.gz'.format(ver)
    dir = 'openssl-' + ver
    url = 'https://www.openssl.org/source/' + archive

    # options
    # (see https://github.com/openssl/openssl/blob/master/INSTALL.md for more details)
    opts = []
    # platform (should be the first argument)
    platform = openssl_get_platform(toolchain)
    if platform:
        opts.append(platform)
    if variant == 'debug':
        opts.append('--debug')
    else:
        opts.append('--release')
    # --cross-compile-prefix is not needed as we already have CXX variable set
    # lib-specific features:
    opts += [
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
    ]

    # build
    download(url, archive, logfile, vendordir)
    unpack(archive, dir)
    os.chdir('src/' + dir)
    # see https://github.com/openssl/openssl/blob/master/INSTALL.md#configuration-options
    # for more options:
    execute(
        makeenv(envlist) +
        ' ' + makeflags(workdir, toolchain, env, deplist, variant=variant) +
        ' ./Configure ' +
        ' '.join(opts),
        logfile)
    execute_make(logfile)
    install_tree('include', inc_dir)
    install_files('libssl.a', lib_dir)
    install_files('libcrypto.a', lib_dir)
elif name == 'gengetopt':
    download('ftp://ftp.gnu.org/gnu/gengetopt/gengetopt-%s.tar.gz' % ver,
             'gengetopt-%s.tar.gz' % ver,
             logfile,
             vendordir)
    unpack('gengetopt-%s.tar.gz' % ver,
            'gengetopt-%s' % ver)
    os.chdir('src/gengetopt-%s' % ver)
    execute('./configure', logfile, clear_env=True)
    execute_make(logfile, cpu_count=0) # -j is buggy for gengetopt
    install_files('src/gengetopt', bin_dir)
elif name == 'ragel':
    download('https://www.colm.net/files/ragel/ragel-%s.tar.gz' % ver,
             'ragel-%s.tar.gz' % ver,
             logfile,
             vendordir)
    unpack('ragel-%s.tar.gz' % ver,
            'ragel-%s' % ver)
    os.chdir('src/ragel-%s' % ver)
    execute('./configure', logfile, clear_env=True)
    execute_make(logfile)
    install_files('ragel/ragel', bin_dir)
elif name == 'cpputest':
    download(
        'https://github.com/cpputest/cpputest/releases/download/v%s/cpputest-%s.tar.gz' % (
            ver, ver),
        'cpputest-%s.tar.gz' % ver,
        logfile,
        vendordir)
    unpack('cpputest-%s.tar.gz' % ver,
            'cpputest-%s' % ver)
    os.chdir('src/cpputest-%s' % ver)
    execute('./configure --host=%s %s %s %s' % (
            toolchain,
            makeenv(envlist),
            # disable warnings, since CppUTest uses -Werror and may fail to
            # build on old GCC versions
            makeflags(workdir, toolchain, env, [], cflags='-w'),
            ' '.join([
                '--enable-static',
                # disable memory leak detection which is too hard to use properly
                '--disable-memory-leak-detection',
            ])), logfile)
    execute_make(logfile)
    install_tree('include', inc_dir)
    install_files('lib/libCppUTest.a', lib_dir)
elif name == 'google-benchmark':
    download(
        'https://github.com/google/benchmark/archive/v%s.tar.gz' % ver,
        'benchmark_v%s.tar.gz' % ver,
        logfile,
        vendordir)
    unpack('benchmark_v%s.tar.gz' % ver,
            'benchmark-%s' % ver)
    os.chdir('src/benchmark-%s' % ver)
    mkpath('build')
    os.chdir('build')
    execute_cmake('..', variant, toolchain, env, logfile, args=[
        '-DBENCHMARK_ENABLE_GTEST_TESTS=OFF',
        '-DCMAKE_CXX_FLAGS=-w',
        ])
    execute_make(logfile)
    os.chdir('..')
    install_tree('include', inc_dir, match=['*.h'])
    install_files('build/src/libbenchmark.a', lib_dir)
# end of deps
else:
    print("error: unknown 3rdparty '%s'" % fullname, file=sys.stderr)
    exit(1)

touch(commit_build_finished_file)
