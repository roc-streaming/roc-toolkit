#! /usr/bin/python2
from __future__ import print_function

import sys
import os
import shutil
import glob
import fnmatch
import ssl
import tarfile
import fileinput
import subprocess

try:
    from urllib.request import urlopen
except ImportError:
    from urllib2 import urlopen

printdir = os.path.abspath('.')

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

def download(url, path):
    print('[download] %s' % url)
    rmpath(path)
    try:
        # workaround for SSL certificate error
        ssl._create_default_https_context = ssl._create_unverified_context
    except:
        pass
    try:
        archive = urlopen(url)
    except Exception as e:
        print("error: can't download '%s': %s" % (url, e.reason[1]), file=sys.stderr)
        exit(1)
    with open(path, 'wb') as fp:
        fp.write(archive.read())

def extract(path, dirname):
    print('[extract] %s' % path)
    rmpath(dirname)
    tar = tarfile.open(path, 'r')
    tar.extractall('.')
    tar.close()

def execute(cmd, log):
    print('[execute] %s' % cmd)
    with open(log, 'a+') as fp:
        print('>>> %s' % cmd, file=fp)
    if os.system('%s >>%s 2>&1' % (cmd, log)) != 0:
        exit(1)

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

def freplace(path, pat, to):
    print('[patch] %s' % path)
    for line in fileinput.input(path, inplace=True):
        print(line.replace(pat, to), end='')

def touch(path):
    open(path, 'w').close()

def getsysroot(toolchain):
    if not toolchain:
        return ""
    try:
        cmd = ['%s-gcc' % toolchain, '-print-sysroot']
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        return proc.stdout.read().strip()
    except:
        print("error: can't execute '%s'" % ' '.join(cmd), file=sys.stderr)
        exit(1)

def isgnu(toolchain):
    if toolchain:
        cmd = ['%s-ld' % toolchain, '-v']
    else:
        cmd = ['ld', '-v']
    try:
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        return 'GNU' in proc.stdout.read().strip()
    except:
        return False

def makeflags(workdir, toolchain, deplist, cflags='', ldflags='', variant=''):
    incdirs=[]
    libdirs=[]

    for dep in deplist:
        incdirs += [os.path.join(workdir, 'build', dep, 'include')]
        libdirs += [os.path.join(workdir, 'build', dep, 'lib')]

    cflags = ([cflags] if cflags else []) + ['-I%s' % path for path in incdirs]
    ldflags = ['-L%s' % path for path in libdirs] + ([ldflags] if ldflags else [])

    gnu = isgnu(toolchain)

    if variant == 'debug':
        if gnu:
            cflags += ['-ggdb']
        else:
            cflags += ['-g']
    elif variant == 'release':
        cflags += ['-O2']

    if gnu:
        ldflags += ['-Wl,-rpath-link=%s' % path for path in libdirs]

    return ' '.join([
        'CXXFLAGS="%s"' % ' '.join(cflags),
        'CFLAGS="%s"' % ' '.join(cflags),
        'LDFLAGS="%s"' % ' '.join(ldflags),
    ])

if len(sys.argv) != 6:
    print("error: usage: 3rdparty.py workdir toolchain variant package deplist",
          file=sys.stderr)
    exit(1)

workdir = os.path.abspath(sys.argv[1])
toolchain = sys.argv[2]
variant = sys.argv[3]
fullname = sys.argv[4]
deplist = sys.argv[5].split(':')

name, ver = fullname.split('-', 1)

builddir = os.path.join(workdir, 'build', fullname)
rpathdir = os.path.join(workdir, 'rpath')

logfile = os.path.join(builddir, 'build.log')

rmpath(os.path.join(builddir, 'commit'))
mkpath(os.path.join(builddir, 'src'))
os.chdir(os.path.join(builddir, 'src'))

if name == 'uv':
    download('http://dist.libuv.org/dist/v%s/libuv-v%s.tar.gz' % (ver, ver),
             'libuv-v%s.tar.gz' % ver)
    extract('libuv-v%s.tar.gz' % ver,
            'libuv-v%s' % ver)
    os.chdir('libuv-v%s' % ver)
    freplace('include/uv.h', '__attribute__((visibility("default")))', '')
    execute('./autogen.sh', logfile)
    execute('./configure --host=%s %s %s' % (
        toolchain,
        makeflags(workdir, toolchain, [], cflags='-fvisibility=hidden'),
        ' '.join([
            '--with-pic',
            '--enable-static',
        ])), logfile)
    execute('make -j', logfile)
    install_tree('include', os.path.join(builddir, 'include'))
    install_files('.libs/libuv.a', os.path.join(builddir, 'lib'))
elif name == 'openfec':
    download(
      'https://github.com/roc-project/openfec/archive/v%s.tar.gz' % ver,
      'openfec_v%s.tar.gz' % ver)
    extract('openfec_v%s.tar.gz' % ver,
            'openfec-%s' % ver)
    os.chdir('openfec-%s' % ver)
    os.mkdir('build')
    os.chdir('build')
    args = [
        '-DCMAKE_C_COMPILER=%s' % '-'.join([s for s in [toolchain, 'gcc'] if s]),
        '-DCMAKE_FIND_ROOT_PATH=%s' % getsysroot(toolchain),
        '-DCMAKE_POSITION_INDEPENDENT_CODE=ON',
        '-DBUILD_STATIC_LIBS=ON',
        ]
    if variant == 'debug':
        dist = 'bin/Debug'
        args += [
            '-DCMAKE_BUILD_TYPE=Debug',
            # enable debug symbols and logs
            '-DDEBUG:STRING=ON',
            # -ggdb is required for sanitizer backtrace
            # -fPIC should be set explicitly in older cmake versions
            '-DCMAKE_C_FLAGS_DEBUG:STRING="-ggdb -fPIC -fvisibility=hidden"',
        ]
    else:
        dist = 'bin/Release'
        args += [
            '-DCMAKE_BUILD_TYPE=Release',
            # disable debug symbols and logs
            '-DDEBUG:STRING=OFF',
            # -fPIC should be set explicitly in older cmake versions
            '-DCMAKE_C_FLAGS_RELEASE:STRING="-fPIC -fvisibility=hidden"',
        ]
    execute('cmake .. ' + ' '.join(args), logfile)
    execute('make -j', logfile)
    os.chdir('..')
    install_tree('src', os.path.join(builddir, 'include'), match=['*.h'])
    install_files('%s/libopenfec.a' % dist, os.path.join(builddir, 'lib'))
elif name == 'alsa':
    download(
      'ftp://ftp.alsa-project.org/pub/lib/alsa-lib-%s.tar.bz2' % ver,
        'alsa-lib-%s.tar.bz2' % ver)
    extract('alsa-lib-%s.tar.bz2' % ver,
            'alsa-lib-%s' % ver)
    os.chdir('alsa-lib-%s' % ver)
    execute('./configure --host=%s %s' % (
        toolchain,
        ' '.join([
            '--enable-shared',
            '--disable-static',
            '--disable-python',
        ])), logfile)
    execute('make -j', logfile)
    install_tree('include/alsa',
            os.path.join(builddir, 'include', 'alsa'),
            ignore=['alsa'])
    install_files('src/.libs/libasound.so', os.path.join(builddir, 'lib'))
    install_files('src/.libs/libasound.so.*', rpathdir)
elif name == 'ltdl':
    download(
      'ftp://ftp.gnu.org/gnu/libtool/libtool-%s.tar.gz' % ver,
        'libtool-%s.tar.gz' % ver)
    extract('libtool-%s.tar.gz' % ver,
            'libtool-%s' % ver)
    os.chdir('libtool-%s' % ver)
    execute('./configure --host=%s %s' % (
        toolchain,
        ' '.join([
            '--enable-shared',
            '--disable-static',
        ])), logfile)
    execute('make -j', logfile)
    install_files('libltdl/ltdl.h', os.path.join(builddir, 'include'))
    install_tree('libltdl/libltdl', os.path.join(builddir, 'include', 'libltdl'))
    install_files('libltdl/.libs/libltdl.so', os.path.join(builddir, 'lib'))
    install_files('libltdl/.libs/libltdl.so.*', rpathdir)
elif name == 'json':
    download(
      'https://github.com/json-c/json-c/archive/json-c-%s.tar.gz' % ver,
        'json-%s.tar.gz' % ver)
    extract('json-%s.tar.gz' % ver,
            'json-c-json-c-%s' % ver)
    os.chdir('json-c-json-c-%s' % ver)
    execute('%s ./configure --host=%s %s %s' % (
        ' '.join([
            # disable rpl_malloc and rpl_realloc
            'ac_cv_func_malloc_0_nonnull=yes',
            'ac_cv_func_realloc_0_nonnull=yes',
        ]),
        toolchain,
        makeflags(workdir, toolchain, [], cflags='-fPIC -fvisibility=hidden'),
        ' '.join([
            '--enable-static',
            '--disable-shared',
        ])), logfile)
    execute('make', logfile) # -j is buggy for json-c
    install_tree('.', os.path.join(builddir, 'include'), match=['*.h'])
    install_files('.libs/libjson.a', os.path.join(builddir, 'lib'))
    install_files('.libs/libjson-c.a', os.path.join(builddir, 'lib'))
elif name == 'sndfile':
    download(
      'http://www.mega-nerd.com/libsndfile/files/libsndfile-%s.tar.gz' % ver,
        'libsndfile-%s.tar.gz' % ver)
    extract('libsndfile-%s.tar.gz' % ver,
            'libsndfile-%s' % ver)
    os.chdir('libsndfile-%s' % ver)
    execute('./configure --host=%s %s %s' % (
        toolchain,
        makeflags(workdir, toolchain, [], cflags='-fPIC -fvisibility=hidden'),
        ' '.join([
            '--enable-static',
            '--disable-shared',
            '--disable-external-libs',
        ])), logfile)
    execute('make -j', logfile)
    install_files('src/sndfile.h', os.path.join(builddir, 'include'))
    install_files('src/.libs/libsndfile.a', os.path.join(builddir, 'lib'))
elif name == 'pulseaudio':
    download(
      'https://freedesktop.org/software/pulseaudio/releases/pulseaudio-%s.tar.gz' % ver,
        'pulseaudio-%s.tar.gz' % ver)
    extract('pulseaudio-%s.tar.gz' % ver,
            'pulseaudio-%s' % ver)
    os.chdir('pulseaudio-%s' % ver)
    execute('./configure --host=%s %s %s %s' % (
        toolchain,
        makeflags(workdir, toolchain, deplist),
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
            '--disable-webrtc-aec',
            '--without-caps',
        ])), logfile)
    execute('make -j', logfile)
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
      'http://vorboss.dl.sourceforge.net/project/sox/sox/%s/sox-%s.tar.gz' % (ver, ver),
      'sox-%s.tar.gz' % ver)
    extract('sox-%s.tar.gz' % ver,
            'sox-%s' % ver)
    os.chdir('sox-%s' % ver)
    execute('./configure --host=%s %s %s' % (
        toolchain,
        makeflags(workdir, toolchain, deplist, cflags='-fvisibility=hidden', variant=variant),
        ' '.join([
            '--enable-static',
            '--disable-shared',
            '--disable-openmp',
            '--without-id3tag',
            '--without-png',
            '--without-ao',
        ])), logfile)
    execute('make -j', logfile)
    install_files('src/sox.h', os.path.join(builddir, 'include'))
    install_files('src/.libs/libsox.a', os.path.join(builddir, 'lib'))
elif name == 'gengetopt':
    download('ftp://ftp.gnu.org/gnu/gengetopt/gengetopt-%s.tar.gz' % ver,
             'gengetopt-%s.tar.gz' % ver)
    extract('gengetopt-%s.tar.gz' % ver,
            'gengetopt-%s' % ver)
    os.chdir('gengetopt-%s' % ver)
    execute('./configure', logfile)
    execute('make', logfile) # -j is buggy for gengetopt
    install_files('src/gengetopt', os.path.join(builddir, 'bin'))
elif name == 'cpputest':
    download(
        'https://raw.githubusercontent.com/cpputest/cpputest.github.io/' \
        'master/releases/cpputest-%s.tar.gz' % ver,
        'cpputest-%s.tar.gz' % ver)
    extract('cpputest-%s.tar.gz' % ver,
            'cpputest-%s' % ver)
    os.chdir('cpputest-%s' % ver)
    execute('./configure --host=%s %s %s' % (
            toolchain,
            # disable warnings, since CppUTest uses -Werror and may fail to
            # build on old GCC versions
            makeflags(workdir, toolchain, [], cflags='-w'),
            ' '.join([
                '--enable-static',
                # disable memory leak detection which is too hard to use properly
                '--disable-memory-leak-detection',
            ])), logfile)
    execute('make -j', logfile)
    install_tree('include', os.path.join(builddir, 'include'))
    install_files('lib/libCppUTest.a', os.path.join(builddir, 'lib'))
else:
    print("error: unknown 3rdparty '%s'" % fullname, file=sys.stderr)
    exit(1)

touch(os.path.join(builddir, 'commit'))
