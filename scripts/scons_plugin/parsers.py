import re
import os.path

def _fix_target(s):
    # on redhat and fedora, "system" and "os" parts are different
    # convert it to common form
    s = s.replace('-redhat-linux', '-linux-gnu')
    parts = s.split('-')
    if len(parts) == 3:
        # on some distros "system" part is omitted
        # convert it to common form
        parts = [parts[0]] + ['pc'] + parts[1:]
    elif len(parts) == 4:
        # on older config.guess versions "system" part defaulted to
        # "unknown" instead of "pc"; convert it to modern form
        if parts[1] == 'unknown':
            parts[1] = 'pc'
    s =  '-'.join(parts)
    return s

def ParseGitHead(env):
    try:
        with open('.git/HEAD') as hf:
            head_ref = hf.read().strip().split()[-1]
            pattern = re.compile(r'\b[0-9a-f]{40}\b')
            is_sha = re.match(pattern, head_ref)
            if is_sha:
                return head_ref[:10]
            else:
                with open('.git/%s' % (head_ref)) as hrf:
                    return hrf.read().strip()[:10]
    except:
        return None

def ParseProjectVersion(env, path):
    return env.GetCommandOutput(
        '%s scripts/scons_helpers/parse-version.py' % env.GetPythonExecutable())

def ParseToolVersion(env, command):
    text = env.GetCommandOutput(command)
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

            full_text = env.GetCommandOutput('%s --version' % compiler)

            for regex in version_formats:
                m = re.search(r'(?:LLVM|clang)\s+version\s+'+regex, full_text)
                if m:
                    return m.group(1)

            trunc_text = re.sub(r'\([^)]+\)', '', full_text)

            dump_text = env.GetCommandOutput('%s -dumpversion' % compiler)

            for text in [dump_text, trunc_text, full_text]:
                for regex in version_formats:
                    m = re.search(regex, text)
                    if m:
                        return m.group(1)

            raise Exception("Invalid compiler version.")
        except Exception as e:
            print(e)
            raise e

    ver = getverstr()
    if ver:
        return tuple(map(int, ver.split('.')))
    else:
        return None

def ParseCompilerTarget(env, compiler):
    text = env.GetCommandOutput('%s -v -E -' % compiler)
    if not text:
        return None

    for line in text.splitlines():
        m = re.search(r'\bTarget:\s*(\S+)', line)
        if m:
            s = m.group(1)
            s = _fix_target(s)
            return s

    return None

def ParseCompilerDirectory(env, compiler):
    text = env.GetCommandOutput('%s --version' % compiler)
    if not text:
        return None

    for line in text.splitlines():
        m = re.search(r'\bInstalledDir:\s*(.*)', line)
        if m:
            return m.group(1)

    return None

def ParseLinkDirs(env, linker):
    text = env.GetCommandOutput('%s -print-search-dirs' % linker)
    if not text:
        return []

    for line in text.splitlines():
        m = re.search('^\s*libraries:\s*=(.*)$', line)
        if m:
            ret = []
            for libdir in m.group(1).split(':'):
                libdir = os.path.abspath(libdir)
                if os.path.isdir(libdir):
                    ret.append(libdir)
            return ret

    return []

def ParseConfigGuess(env, cmd):
    text = env.GetCommandOutput(cmd)
    if not text:
        return None

    if not re.match(r'^\S+-\S+$', text):
        return None

    text = _fix_target(text)
    return text

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
    env.AddMethod(ParseGitHead, 'ParseGitHead')
    env.AddMethod(ParseProjectVersion, 'ParseProjectVersion')
    env.AddMethod(ParseToolVersion, 'ParseToolVersion')
    env.AddMethod(ParseCompilerVersion, 'ParseCompilerVersion')
    env.AddMethod(ParseCompilerTarget, 'ParseCompilerTarget')
    env.AddMethod(ParseCompilerDirectory, 'ParseCompilerDirectory')
    env.AddMethod(ParseLinkDirs, 'ParseLinkDirs')
    env.AddMethod(ParseConfigGuess, 'ParseConfigGuess')
    env.AddMethod(ParseList, 'ParseList')
