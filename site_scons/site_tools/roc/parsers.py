import re
import os
import subprocess

def _command_output(env, command):
    try:
        with open(os.devnull, 'w') as null:
            proc = subprocess.Popen(command,
                                    stdin=null,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT,
                                    env=env['ENV'])
            lines = [s.decode() for s in proc.stdout.readlines()]
            output = str(' '.join(lines).strip())
            proc.terminate()
            return output
    except:
        return None

def ParseVersion(env, command):
    text = _command_output(env, command)
    if not text:
        return None

    m = re.search(r'(\b[0-9][0-9.]+\b)', text)
    if not m:
        return None

    return m.group(1)

def ParseCompilerVersion(env, compiler):
    def getverstr():
        try:
            version_formats = [
                r'(\b[0-9]+\.[0-9]+\.[0-9]+\b)',
                r'(\b[0-9]+\.[0-9]+\b)',
            ]

            full_text = _command_output(env, [compiler, '--version'])

            for regex in version_formats:
                m = re.search(r'(?:LLVM|clang)\s+version\s+'+regex, full_text)
                if m:
                    return m.group(1)

            trunc_text = re.sub(r'\([^)]+\)', '', full_text)

            dump_text = _command_output(env, [compiler, '-dumpversion'])

            for text in [dump_text, trunc_text, full_text]:
                for regex in version_formats:
                    m = re.search(regex, text)
                    if m:
                        return m.group(1)

            return None
        except:
            pass

    ver = getverstr()
    if ver:
        return tuple(map(int, ver.split('.')))
    else:
        return None

def ParseCompilerTarget(env, compiler):
    text = _command_output(env, [compiler, '-v', '-E', '-'])
    if not text:
        return None

    for line in text.splitlines():
        m = re.search(r'\bTarget:\s*(\S+)', line)
        if m:
            parts = m.group(1).split('-')
            # "system" defaults to "pc" on recent config.guess versions
            # use the same newer format for all compilers
            if len(parts) == 3:
                parts = [parts[0]] + ['pc'] + parts[1:]
            elif len(parts) == 4:
                if parts[1] == 'unknown':
                    parts[1] = 'pc'
            return '-'.join(parts)

    return None

def ParsePkgConfig(env, cmd):
    if 'PKG_CONFIG' in env.Dictionary():
        pkg_config = env['PKG_CONFIG']
    elif env.Which('pkg-config'):
        pkg_config = 'pkg-config'
    else:
        return False

    try:
        env.ParseConfig('%s %s' % (pkg_config, cmd))
        return True
    except:
        return False

def ParseList(env, s, all):
    if not s:
        return []
    ret = []
    for name in s.split(','):
        if name == 'all':
            for name in all:
                if not name in ret:
                    ret.append(name)
        else:
            if not name in ret:
                ret.append(name)
    return ret

def init(env):
    env.AddMethod(ParseVersion, 'ParseVersion')
    env.AddMethod(ParseCompilerVersion, 'ParseCompilerVersion')
    env.AddMethod(ParseCompilerTarget, 'ParseCompilerTarget')
    env.AddMethod(ParsePkgConfig, 'ParsePkgConfig')
    env.AddMethod(ParseList, 'ParseList')
