#! /usr/bin/python2
from __future__ import print_function
import sys
import os
import shutil
import fnmatch
import urllib2
import tarfile
import fileinput

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
    print('>>> downloading %s' % url)
    rmpath(path)
    archive = urllib2.urlopen(url)
    with open(path, 'wb') as fp:
        fp.write(archive.read())

def extract(path, dirname):
    print('>>> extracting %s' % path)
    rmpath(dirname)
    with tarfile.open(path, 'r') as tar:
        tar.extractall('.')

def execute(cmd):
    print('>>> executing %s' % cmd)
    if os.system(cmd) != 0:
        exit(1)

def install(src, dst, ignore=None):
    print('>>> installing %s' % dst)
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
    print('>>> patching %s' % path)
    for line in fileinput.input(path, inplace=True):
        print(line.replace(pat, to), end='')

def touch(path):
    open(path, 'w').close()

if len(sys.argv) != 4
    print("usage: 3rdparty.py workdir toolchain name")
    exit(1)

workdir = os.path.abspath(sys.argv[1])
toolchain = sys.argv[2]
fullname = sys.argv[3]

name, ver = fullname.split('-')

rmpath(os.path.join(workdir, '%s.done' % fullname))
mkpath(os.path.join(workdir, fullname, 'src'))
os.chdir(os.path.join(workdir, fullname, 'src'))

if name == 'uv':
    download('https://github.com/libuv/libuv/archive/v%s.tar.gz' % ver,
             'libuv-%s.tar.gz' % ver)
    extract('libuv-%s.tar.gz' % ver,
            'libuv-%s' % ver)
    os.chdir('libuv-%s' % ver)
    execute('./autogen.sh')
    execute('./configure --enable-static --target=%s')
    execute('make -j')
    install('include', os.path.join(workdir, fullname, 'include'))
    install('.libs/libuv.a', os.path.join(workdir, fullname, 'lib'))
elif name == 'openfec':
    download(
      'http://openfec.org/files/openfec_v%s.tgz' % ver.replace('.', '_'),
      'openfec_v%s.tar.gz' % ver)
    extract('openfec_v%s.tar.gz' % ver,
            'openfec_v%s' % ver)
    os.chdir('openfec_v%s' % ver)
    freplace('CMakeLists.txt', '-O4', '-O2')
    freplace('src/CMakeLists.txt', 'SHARED', 'STATIC')
    os.mkdir('build')
    os.chdir('build')
    execute('cmake .. ' + ' '.join([
        '-DCMAKE_SYSTEM_NAME=%s' % toolchain,
        '-DCMAKE_BUILD_TYPE=Release',
        '-DCMAKE_C_FLAGS_RELEASE:STRING="-O2 -w"',
        '-DCMAKE_RULE_MESSAGES=OFF',
        '-DDEBUG:STRING=OFF',
        ]))
    execute('make -j')
    os.chdir('..')
    install('src', os.path.join(workdir, fullname, 'include'),
            ignore=['*.c', '*.txt'])
    install('bin/Release/libopenfec.a', os.path.join(workdir, fullname, 'lib'))
elif name == 'sox':
    download(
      'http://vorboss.dl.sourceforge.net/project/sox/sox/%s/sox-%s.tar.gz' % (ver, ver),
      'sox-%s.tar.gz' % ver)
    extract('sox-%s.tar.gz' % ver,
            'sox-%s' % ver)
    os.chdir('sox-%s' % ver)
    freplace('configure', 'return gsm_create', 'gsm_not_found_hacked')
    execute(' ./configure --enable-static --disable-shared --target=%s %s' % (
        toolchain,
        ' '.join([
            '--disable-openmp',
            '--without-libltdl',
            '--without-magic',
            '--without-png',
            '--without-ladspa',
            '--without-mad',
            '--without-id3tag',
            '--without-lame',
            '--without-twolame',
            '--without-oggvorbis',
            '--without-opus',
            '--without-flac',
            '--without-amrwb',
            '--without-amrnb',
            '--without-wavpack',
            '--without-sndio',
            '--without-ao',
            '--without-sndfile',
            '--without-oss',
            '--without-mp3',
            '--without-lpc10',
            '--without-pulseaudio',
            '--without-gsm',
    ])))
    execute('make -j')
    install('src/sox.h', os.path.join(workdir, fullname, 'include'))
    install('src/.libs/libsox.a', os.path.join(workdir, fullname, 'lib'))
elif name == 'cpputest':
    download(
        'https://raw.githubusercontent.com/cpputest/cpputest.github.io/' \
        'master/releases/cpputest-%s.tar.gz' % ver,
        'cpputest-%s.tar.gz' % ver)
    extract('cpputest-%s.tar.gz' % ver,
            'cpputest-%s' % ver)
    os.chdir('cpputest-%s' % ver)
    execute('./configure --enable-static --disable-memory-leak-detection --target=%s')
    execute('make -j')
    install('include', os.path.join(workdir, fullname, 'include'))
    install('lib/libCppUTest.a', os.path.join(workdir, fullname, 'lib'))
else:
    print("unknown 3rdparty '%s'" % fullname)
    exit(1)

touch(os.path.join(workdir, '%s.done' % fullname))

print('>>> done')
