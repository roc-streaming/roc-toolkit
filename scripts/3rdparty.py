#! /usr/bin/python2
from __future__ import print_function
import sys
import os
import shutil
import fnmatch
import urllib2
import ssl
import tarfile
import fileinput
import subprocess

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
        archive = urllib2.urlopen(url)
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

def install(src, dst, ignore=None):
    print('[install] %s' % os.path.relpath(dst, printdir))
    if os.path.isdir(src):
        if ignore:
            ignore = shutil.ignore_patterns(*ignore)
        mkpath(os.path.dirname(dst))
        rmpath(dst)
        shutil.copytree(src, dst, ignore=ignore)
    else:
        mkpath(dst)
        shutil.copy(src, dst)

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

def getexports(workdir, deplist):
    inc=[]
    lib=[]
    for dep in deplist:
        inc += [os.path.join(workdir, 'build', dep, 'include')]
        lib += [os.path.join(workdir, 'build', dep, 'lib')]
    return 'CFLAGS="%s" LDFLAGS="%s"' % (
        ' '.join(['-I%s' % path for path in inc]),
        ' '.join(['-L%s' % path for path in lib]))

if len(sys.argv) != 6:
    print("error: usage: 3rdparty.py workdir toolchain variant package deplist",
          file=sys.stderr)
    exit(1)

workdir = os.path.abspath(sys.argv[1])
toolchain = sys.argv[2]
variant = sys.argv[3]
fullname = sys.argv[4]
deplist = sys.argv[5].split(':')

name, ver = fullname.split('-')

builddir = os.path.join(workdir, 'build', fullname)
libdir = os.path.join(workdir, 'lib')

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
    execute('./autogen.sh', logfile)
    execute('./configure --host=%s --with-pic --enable-static' % toolchain, logfile)
    execute('make -j', logfile)
    install('include', os.path.join(builddir, 'include'))
    install('.libs/libuv.a', os.path.join(builddir, 'lib'))
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
            # -fPIC should be set explicitly in older cmake versions
            # -ggdb is required for sanitizer backtrace
            '-DCMAKE_C_FLAGS_DEBUG:STRING="-fPIC -ggdb"',
        ]
    else:
        dist = 'bin/Release'
        args += [
            '-DCMAKE_BUILD_TYPE=Release',
            # disable debug symbols and logs
            '-DDEBUG:STRING=OFF',
            # -fPIC should be set explicitly in older cmake versions
            '-DCMAKE_C_FLAGS_RELEASE:STRING=-fPIC',
        ]
    execute('cmake .. ' + ' '.join(args), logfile)
    execute('make -j', logfile)
    os.chdir('..')
    install('src', os.path.join(builddir, 'include'),
            ignore=['*.c', '*.txt'])
    install('%s/libopenfec.a' % dist, os.path.join(builddir, 'lib'))
elif name == 'alsa':
    download(
      'ftp://ftp.alsa-project.org/pub/lib/alsa-lib-%s.tar.bz2' % ver,
        'alsa-lib-%s.tar.bz2' % ver)
    extract('alsa-lib-%s.tar.bz2' % ver,
            'alsa-lib-%s' % ver)
    os.chdir('alsa-lib-%s' % ver)
    execute('./configure --host=%s %s' % (
        toolchain, ' '.join([
            '--enable-shared',
            '--disable-static',
            '--disable-python',
        ])), logfile)
    execute('make -j', logfile)
    install('include/alsa',
            os.path.join(builddir, 'include', 'alsa'), ignore=['alsa'])
    install('src/.libs/libasound.so', os.path.join(builddir, 'lib'))
    install('src/.libs/libasound.so.2', libdir)
elif name == 'sox':
    download(
      'http://vorboss.dl.sourceforge.net/project/sox/sox/%s/sox-%s.tar.gz' % (ver, ver),
      'sox-%s.tar.gz' % ver)
    extract('sox-%s.tar.gz' % ver,
            'sox-%s' % ver)
    os.chdir('sox-%s' % ver)
    execute('%s LIBS="-lpthread -ldl -lrt" ./configure --host=%s %s' % (
        getexports(workdir, deplist), toolchain, ' '.join([
            '--enable-static',
            '--disable-shared',
            '--disable-openmp',
            '--without-id3tag',
            '--without-png',
        ])), logfile)
    execute('make -j', logfile)
    install('src/sox.h', os.path.join(builddir, 'include'))
    install('src/.libs/libsox.a', os.path.join(builddir, 'lib'))
elif name == 'gengetopt':
    download('ftp://ftp.gnu.org/gnu/gengetopt/gengetopt-%s.tar.gz' % ver,
             'gengetopt-%s.tar.gz' % ver)
    extract('gengetopt-%s.tar.gz' % ver,
            'gengetopt-%s' % ver)
    os.chdir('gengetopt-%s' % ver)
    execute('./configure', logfile)
    execute('make', logfile) # -j is buggy for gengetopt
    install('src/gengetopt', os.path.join(builddir, 'bin'))
elif name == 'cpputest':
    download(
        'https://raw.githubusercontent.com/cpputest/cpputest.github.io/' \
        'master/releases/cpputest-%s.tar.gz' % ver,
        'cpputest-%s.tar.gz' % ver)
    extract('cpputest-%s.tar.gz' % ver,
            'cpputest-%s' % ver)
    os.chdir('cpputest-%s' % ver)
    # disable memory leak detection which is too hard to use properly
    # disable warnings, since CppUTest uses -Werror and may fail to build on old GCC
    execute('CXXFLAGS="-w" '
        './configure --host=%s --enable-static --disable-memory-leak-detection' % (
            toolchain), logfile)
    execute('make -j', logfile)
    install('include', os.path.join(builddir, 'include'))
    install('lib/libCppUTest.a', os.path.join(builddir, 'lib'))
else:
    print("error: unknown 3rdparty '%s'" % fullname, file=sys.stderr)
    exit(1)

touch(os.path.join(builddir, 'commit'))
