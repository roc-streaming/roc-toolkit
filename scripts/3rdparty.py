#! /usr/bin/python2
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

printdir = os.path.abspath('.')
devnull = open(os.devnull, 'w+')

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

def download_vendordir(url, path, log, vendordir):
    distfile = os.path.join(vendordir, os.path.basename(path))
    if not os.path.exists(distfile):
        raise
    print('[found vendored] %s' % os.path.basename(distfile))
    with open(distfile, 'rb') as rp:
        with open(path, 'wb') as wp:
            wp.write(rp.read())

def download_urlopen(url, path, log, vendordir):
    rp = urlopen(url)
    with open(path, 'wb') as wp:
        wp.write(rp.read())

def download_tool(url, path, log, vendordir, tool, cmd):
    print('[trying %s] %s' % (tool, url))
    with open(log, 'a+') as fp:
        print('>>> %s' % cmd, file=fp)
    if os.system(cmd) != 0:
        raise

def download_wget(url, path, log, vendordir):
    download_tool(url, path, log, vendordir, 'wget', 'wget "%s" --quiet -O "%s"' % (url, path))

def download_curl(url, path, log, vendordir):
    download_tool(url, path, log, vendordir, 'curl', 'curl -Ls "%s" -o "%s"' % (url, path))

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
    for fn in [download_vendordir, download_urlopen, download_curl, download_wget]:
        try:
            fn(url, path_tmp, log, vendordir)
            shutil.move(path_tmp, path_res)
            rm_emptydir('tmp')
            return
        except Exception as e:
            if fn == download_vendordir:
                print('[download] %s' % url)
            if fn == download_urlopen:
                error = e

    print("error: can't download '%s': %s" % (url, error), file=sys.stderr)
    exit(1)

def extract(filename, dirname):
    dirname_res = 'src/' + dirname
    dirname_tmp = 'tmp/' + dirname

    if os.path.exists(dirname_res):
        print('[found extracted] %s' % dirname)
        return

    print('[extract] %s' % filename)

    rmpath(dirname_res)
    rmpath(dirname_tmp)
    mkpath('tmp')

    tar = tarfile.open('src/'+filename, 'r')
    tar.extractall('tmp')
    tar.close()

    shutil.move(dirname_tmp, dirname_res)
    rm_emptydir('tmp')

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

def execute_cmake(srcdir, variant, toolchain, env, log, args=[]):
    compiler = getvar(env, 'CC', toolchain, 'gcc')
    sysroot = getsysroot(toolchain, compiler)

    args += [
        '-DCMAKE_FIND_ROOT_PATH=%s' % quote(sysroot),
        '-DCMAKE_SYSROOT=%s'        % quote(sysroot),
        '-DCMAKE_POSITION_INDEPENDENT_CODE=ON',
    ]

    if 'android' in toolchain:
        args += ['-DCMAKE_SYSTEM_NAME=Android']
        api = getandroidapi(compiler)
        if api:
            args += ['-DCMAKE_SYSTEM_VERSION=%s' % api]
        abi = getandroidabi(toolchain)
        if abi:
            args += ['-DCMAKE_ANDROID_ARCH_ABI=%s' % abi]

    if not 'OE_CMAKE_TOOLCHAIN_FILE' in os.environ: # workaround for yocto linux
        if not 'android' in toolchain: # workaround for android
            args += [
                '-DCMAKE_C_COMPILER=%s' % quote(compiler),
            ]
        args += [
            '-DCMAKE_LINKER=%s' % quote(getvar(env, 'CCLD', toolchain, 'gcc')),
            '-DCMAKE_AR=%s'     % quote(getvar(env, 'AR', toolchain, 'ar')),
            '-DCMAKE_RANLIB=%s' % quote(getvar(env, 'RANLIB', toolchain, 'ranlib')),
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

def freplace(path, from_, to):
    print('[patch] %s' % path)
    for line in fileinput.input(path, inplace=True):
        print(line.replace(from_, to), end='')

def freplace_tree(dirpath, filepats, from_, to):
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
                    freplace(filepath, from_, to)

def try_patch(dirname, patchurl, patchname, logfile, vendordir):
    if not try_execute('patch --version'):
        return
    download(patchurl, patchname, logfile, vendordir)
    execute('patch -p1 -N -d %s -i %s' % (
        'src/' + dirname,
        '../' + patchname), logfile, ignore_error=True)

def touch(path):
    open(path, 'w').close()

def getvar(env, var, toolchain, default):
    if var in env:
        return env[var]
    return '-'.join([s for s in [toolchain, default] if s])

def getsysroot(toolchain, compiler):
    if not toolchain:
        return ""

    if not compiler:
        compiler = '%s-gcc' % toolchain

    try:
        cmd = [compiler, '-print-sysroot']
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=devnull)
        sysroot = proc.stdout.read().strip()
        if os.path.isdir(sysroot):
            return sysroot
    except:
        pass

    if 'android' in toolchain:
        try:
            from distutils.spawn import find_executable
            path = find_executable(compiler)
            while True:
                parent = os.path.dirname(path)
                if parent == path:
                    break
                path = parent
                sysroot = os.path.join(path, 'sysroot')
                if os.path.isdir(sysroot):
                    return sysroot
        except:
            pass

    print("error: can't determine sysroot for '%s' toolchain" % toolchain, file=sys.stderr)
    exit(1)

def getandroidabi(toolchain):
    try:
        ta = toolchain.split('-')[0]
    except:
        return
    if ta == 'arm':
        return 'armeabi-v7a'
    if ta == 'aarch64':
        return 'arm64-v8a'
    if ta == 'i686':
        return 'x86'
    if ta == 'x86_64':
        return 'x86_64'
    return ta

def getandroidapi(compiler):
    try:
        cmd = [compiler, '-dM', '-E', '-']
        proc = subprocess.Popen(cmd, stdin=devnull, stdout=subprocess.PIPE, stderr=devnull)
        for line in proc.stdout.read().splitlines():
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
            out = str(proc.stdout.read().strip())
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

    for dep in deplist:
        incdirs += [os.path.join(workdir, 'build', dep, 'include')]
        libdirs += [os.path.join(workdir, 'build', dep, 'lib')]

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
        ldflags += ['-Wl,-rpath-link=%s' % path for path in libdirs]

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

if len(sys.argv) < 7:
    print("error: usage: 3rdparty.py workdir vendordir toolchain variant package deplist [env]",
          file=sys.stderr)
    exit(1)

workdir = os.path.abspath(sys.argv[1])
vendordir = os.path.abspath(sys.argv[2])
toolchain = sys.argv[3]
variant = sys.argv[4]
fullname = sys.argv[5]
deplist = sys.argv[6].split(':')
envlist = sys.argv[7:]

env = dict()
for e in envlist:
    k, v = e.split('=', 1)
    env[k] = v

name, ver = fullname.split('-', 1)

builddir = os.path.join(workdir, 'build', fullname)
rpathdir = os.path.join(workdir, 'rpath')

logfile = os.path.join(builddir, 'build.log')

rmpath(os.path.join(builddir, 'commit'))
mkpath(os.path.join(builddir, 'src'))

os.chdir(os.path.join(builddir))

if name == 'libuv':
    download('http://dist.libuv.org/dist/v%s/libuv-v%s.tar.gz' % (ver, ver),
             'libuv-v%s.tar.gz' % ver,
             logfile,
             vendordir)
    extract('libuv-v%s.tar.gz' % ver,
            'libuv-v%s' % ver)
    os.chdir('src/libuv-v%s' % ver)
    freplace('include/uv.h', '__attribute__((visibility("default")))', '')
    if 'android' in toolchain:
        mkpath('build')
        os.chdir('build')
        execute_cmake('..', variant, toolchain, env, logfile, args=[
            '-DLIBUV_BUILD_TESTS=OFF',
            ])
        execute_make(logfile)
        shutil.copy('libuv_a.a', 'libuv.a')
        os.chdir('..')
        install_files('build/libuv.a', os.path.join(builddir, 'lib'))
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
        install_files('.libs/libuv.a', os.path.join(builddir, 'lib'))
    install_tree('include', os.path.join(builddir, 'include'))
elif name == 'libunwind':
    download(
        'http://download.savannah.nongnu.org/releases/libunwind/libunwind-%s.tar.gz' % ver,
        'libunwind-%s.tar.gz' % ver,
        logfile,
        vendordir)
    extract('libunwind-%s.tar.gz' % ver,
            'libunwind-%s' % ver)
    os.chdir('src/libunwind-%s' % ver)
    execute('./configure --host=%s %s %s %s' % (
        toolchain,
        makeenv(envlist),
        makeflags(workdir, toolchain, env, deplist, cflags='-fPIC', variant=variant),
        ' '.join([
            '--enable-static',
            '--disable-shared',
            '--disable-coredump',
            '--disable-ptrace',
            '--disable-setjmp',
            '--disable-minidebuginfo',
           ])), logfile)
    execute_make(logfile)
    install_files('include/*.h', os.path.join(builddir, 'include'))
    install_files('src/.libs/libunwind.a', os.path.join(builddir, 'lib'))
elif name == 'libatomic_ops':
    download(
        'https://github.com/ivmai/libatomic_ops/releases/download/v%s/libatomic_ops-%s.tar.gz' % (
            ver, ver),
        'libatomic_ops-%s.tar.gz' % ver,
        logfile,
        vendordir)
    extract('libatomic_ops-%s.tar.gz' % ver,
            'libatomic_ops-%s' % ver)
    os.chdir('src/libatomic_ops-%s' % ver)
    execute('./configure --host=%s %s %s %s' % (
        toolchain,
        makeenv(envlist),
        makeflags(workdir, toolchain, env, deplist, cflags='-fPIC', variant=variant),
        ' '.join([
            '--enable-static',
            '--disable-shared',
            '--disable-docs',
           ])), logfile)
    execute_make(logfile)
    install_tree('src', os.path.join(builddir, 'include'), match=['*.h'])
    install_files('src/.libs/libatomic_ops.a', os.path.join(builddir, 'lib'))
elif name == 'openfec':
    if variant == 'debug':
        dist = 'bin/Debug'
    else:
        dist = 'bin/Release'
    download(
      'https://github.com/roc-project/openfec/archive/v%s.tar.gz' % ver,
      'openfec_v%s.tar.gz' % ver,
        logfile,
        vendordir)
    extract('openfec_v%s.tar.gz' % ver,
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
    install_tree('src', os.path.join(builddir, 'include'), match=['*.h'])
    install_files('%s/libopenfec.a' % dist, os.path.join(builddir, 'lib'))
elif name == 'alsa':
    download(
      'ftp://ftp.alsa-project.org/pub/lib/alsa-lib-%s.tar.bz2' % ver,
        'alsa-lib-%s.tar.bz2' % ver,
        logfile,
        vendordir)
    extract('alsa-lib-%s.tar.bz2' % ver,
            'alsa-lib-%s' % ver)
    os.chdir('src/alsa-lib-%s' % ver)
    execute('./configure --host=%s %s %s' % (
        toolchain,
        makeenv(envlist),
        ' '.join([
            '--enable-shared',
            '--disable-static',
            '--disable-python',
        ])), logfile)
    execute_make(logfile)
    install_tree('include/alsa',
            os.path.join(builddir, 'include', 'alsa'),
            ignore=['alsa'])
    install_files('src/.libs/libasound.so', os.path.join(builddir, 'lib'))
    install_files('src/.libs/libasound.so.*', rpathdir)
elif name == 'ltdl':
    download(
      'ftp://ftp.gnu.org/gnu/libtool/libtool-%s.tar.gz' % ver,
        'libtool-%s.tar.gz' % ver,
        logfile,
        vendordir)
    extract('libtool-%s.tar.gz' % ver,
            'libtool-%s' % ver)
    os.chdir('src/libtool-%s' % ver)
    execute('./configure --host=%s %s %s' % (
        toolchain,
        makeenv(envlist),
        ' '.join([
            '--enable-shared',
            '--disable-static',
        ])), logfile)
    execute_make(logfile)
    install_files('libltdl/ltdl.h', os.path.join(builddir, 'include'))
    install_tree('libltdl/libltdl', os.path.join(builddir, 'include', 'libltdl'))
    install_files('libltdl/.libs/libltdl.so', os.path.join(builddir, 'lib'))
    install_files('libltdl/.libs/libltdl.so.*', rpathdir)
elif name == 'json':
    download(
      'https://github.com/json-c/json-c/archive/json-c-%s.tar.gz' % ver,
        'json-%s.tar.gz' % ver,
        logfile,
        vendordir)
    extract('json-%s.tar.gz' % ver,
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
    install_tree('.', os.path.join(builddir, 'include'), match=['*.h'])
    install_files('.libs/libjson.a', os.path.join(builddir, 'lib'))
    install_files('.libs/libjson-c.a', os.path.join(builddir, 'lib'))
elif name == 'sndfile':
    download(
      'http://www.mega-nerd.com/libsndfile/files/libsndfile-%s.tar.gz' % ver,
        'libsndfile-%s.tar.gz' % ver,
        logfile,
        vendordir)
    extract('libsndfile-%s.tar.gz' % ver,
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
            '--enable-static',
            '--disable-shared',
            '--disable-external-libs',
        ])), logfile)
    execute_make(logfile)
    install_files('src/sndfile.h', os.path.join(builddir, 'include'))
    install_files('src/.libs/libsndfile.a', os.path.join(builddir, 'lib'))
elif name == 'pulseaudio':
    download(
      'https://freedesktop.org/software/pulseaudio/releases/pulseaudio-%s.tar.gz' % ver,
        'pulseaudio-%s.tar.gz' % ver,
        logfile,
        vendordir)
    extract('pulseaudio-%s.tar.gz' % ver,
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
        freplace_tree('src/pulseaudio-%s' % ver, ['*.h', '*.c'],
                      '#include <asoundlib.h>',
                      '#include <alsa/asoundlib.h>')
    os.chdir('src/pulseaudio-%s' % ver)
    execute('./configure --host=%s %s %s %s %s' % (
        toolchain,
        makeenv(envlist),
        makeflags(workdir, toolchain, env, deplist, cflags='-w -fomit-frame-pointer -O2'),
        ' '.join([
            'LIBJSON_CFLAGS=" "',
            'LIBJSON_LIBS="-ljson-c -ljson"',
            'LIBSNDFILE_CFLAGS=" "',
            'LIBSNDFILE_LIBS="-lsndfile"',
        ]),
        ' '.join([
            '--enable-shared',
            '--disable-static',
            '--disable-tests',
            '--disable-manpages',
            '--disable-orc',
            '--disable-webrtc-aec',
            '--without-caps',
        ])), logfile)
    execute_make(logfile)
    install_files('config.h', os.path.join(builddir, 'include'))
    install_tree('src/pulse', os.path.join(builddir, 'include', 'pulse'),
                 match=['*.h'])
    install_tree('src/pulsecore', os.path.join(builddir, 'include', 'pulsecore'),
                 match=['*.h'])
    install_files('src/.libs/libpulse.so', os.path.join(builddir, 'lib'))
    install_files('src/.libs/libpulse.so.0', rpathdir)
    install_files('src/.libs/libpulse-simple.so', os.path.join(builddir, 'lib'))
    install_files('src/.libs/libpulse-simple.so.0', rpathdir)
    install_files('src/.libs/libpulsecore-*.so', os.path.join(builddir, 'lib'))
    install_files('src/.libs/libpulsecore-*.so', rpathdir)
    install_files('src/.libs/libpulsecommon-*.so', os.path.join(builddir, 'lib'))
    install_files('src/.libs/libpulsecommon-*.so', rpathdir)
elif name == 'sox':
    download(
      'https://downloads.sourceforge.net/project/sox/sox/%s/sox-%s.tar.gz' % (ver, ver),
      'sox-%s.tar.gz' % ver,
        logfile,
        vendordir)
    extract('sox-%s.tar.gz' % ver,
            'sox-%s' % ver)
    os.chdir('src/sox-%s' % ver)
    execute('./configure --host=%s %s %s %s' % (
        toolchain,
        makeenv(envlist),
        makeflags(workdir, toolchain, env, deplist, cflags='-fvisibility=hidden', variant=variant),
        ' '.join([
            '--enable-static',
            '--disable-shared',
            '--disable-openmp',
            '--without-id3tag',
            '--without-png',
            '--without-ao',
            '--without-opus',
        ])), logfile)
    execute_make(logfile)
    install_files('src/sox.h', os.path.join(builddir, 'include'))
    install_files('src/.libs/libsox.a', os.path.join(builddir, 'lib'))
elif name == 'gengetopt':
    download('ftp://ftp.gnu.org/gnu/gengetopt/gengetopt-%s.tar.gz' % ver,
             'gengetopt-%s.tar.gz' % ver,
             logfile,
             vendordir)
    extract('gengetopt-%s.tar.gz' % ver,
            'gengetopt-%s' % ver)
    os.chdir('src/gengetopt-%s' % ver)
    execute('./configure', logfile, clear_env=True)
    execute_make(logfile, cpu_count=0) # -j is buggy for gengetopt
    install_files('src/gengetopt', os.path.join(builddir, 'bin'))
elif name == 'ragel':
    download('https://www.colm.net/files/ragel/ragel-%s.tar.gz' % ver,
             'ragel-%s.tar.gz' % ver,
             logfile,
             vendordir)
    extract('ragel-%s.tar.gz' % ver,
            'ragel-%s' % ver)
    os.chdir('src/ragel-%s' % ver)
    execute('./configure', logfile, clear_env=True)
    execute_make(logfile)
    install_files('ragel/ragel', os.path.join(builddir, 'bin'))
elif name == 'cpputest':
    download(
        'https://raw.githubusercontent.com/cpputest/cpputest.github.io/' \
        'master/releases/cpputest-%s.tar.gz' % ver,
        'cpputest-%s.tar.gz' % ver,
        logfile,
        vendordir)
    extract('cpputest-%s.tar.gz' % ver,
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
    install_tree('include', os.path.join(builddir, 'include'))
    install_files('lib/libCppUTest.a', os.path.join(builddir, 'lib'))
else:
    print("error: unknown 3rdparty '%s'" % fullname, file=sys.stderr)
    exit(1)

touch(os.path.join(builddir, 'commit'))
