import os.path
import platform
import re

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
                with open('.git/{}'.format(head_ref)) as hrf:
                    return hrf.read().strip()[:10]
    except:
        return None

def ParseProjectVersion(env, path):
    return env.GetCommandOutput(
        '{python_cmd} scripts/scons_helpers/parse-version.py'.format(
            python_cmd=env.GetPythonExecutable()))

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
                r'(\b[0-9]+)-win32\b',
            ]

            full_text = env.GetCommandOutput(compiler + ' --version')

            for regex in version_formats:
                m = re.search(r'(?:LLVM|clang)\s+version\s+'+regex, full_text)
                if m:
                    return m.group(1)

            trunc_text = re.sub(r'\([^)]+\)', '', full_text)

            dump_text = env.GetCommandOutput(compiler + ' -dumpversion')

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
    text = env.GetCommandOutput(compiler + ' -v -E -')
    if not text:
        return None

    for line in text.splitlines():
        m = re.search(r'\bTarget:\s*(\S+)', line)
        if m:
            s = m.group(1)
            s = _fix_target(s)
            return s

    return None

def ParseCompilerType(env, compiler):
    cmds = [compiler + ' --version', compiler + ' -v -E -']
    for cmd in cmds:
        text = env.GetCommandOutput(cmd)
        if 'clang version' in text:
            return 'clang'
        if 'gcc version' in text:
            return 'gcc'

    basename = os.path.basename(compiler)
    if 'clang' in basename:
        return 'clang'
    if 'gcc' in basename or 'g++' in basename:
        return 'gcc'
    if basename in ['cc', 'c++']:
        return 'cc'

    return None

def ParseCompilerDirectory(env, compiler):
    text = env.GetCommandOutput(compiler + ' --version')
    if not text:
        return None

    for line in text.splitlines():
        m = re.search(r'\bInstalledDir:\s*(.*)', line)
        if m:
            return m.group(1)

    return None

def ParseLinkDirs(env, linker):
    text = env.GetCommandOutput(linker + ' -print-search-dirs')
    if not text:
        return []

    for line in text.splitlines():
        m = re.search(r'^\s*libraries:\s*=(.*)$', line)
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

def ParseMacosHost(env, host, macos_platform, macos_arch):
    if not 'darwin' in host:
        return host

    arch_list = ParseList(env, macos_arch, ['arm64', 'x86_64'])
    if len(arch_list) > 1:
        for arch in ['arm64', 'x86_64']:
            host = host.replace(arch, 'universal')

    host = host.replace('-pc-apple-', '-apple-macos{}-'.format(
        ParseMacosPlatform(env, host, macos_platform)))

    return host

def ParseMacosPlatform(env, host, macos_platform):
    if not 'darwin' in host:
        return None

    if macos_platform:
        return macos_platform

    text = env.GetCommandOutput('sw_vers')
    if text:
        for line in text.splitlines():
            if 'ProductVersion' in line:
                try:
                    return '.'.join(line.split()[-1].split('.')[:2])
                except:
                    pass

    return None

def ParseMacosArch(env, host, macos_arch):
    if not 'darwin' in host:
        return []

    arch_list = ParseList(env, macos_arch, ['arm64', 'x86_64'])
    if arch_list:
        return arch_list

    if 'universal' in host:
        return ['arm64', 'x86_64']

    for arch in ['arm64', 'x86_64']:
        if arch in host:
            return [arch]

    if 'arm' in platform.processor():
        return ['arm64']
    else:
        return ['x86_64']

def ParseList(env, s, all):
    if not s:
        return []
    ret = []
    for name in s.split(','):
        if name == 'all':
            for other_name in all:
                if not other_name in ret and other_name != 'all':
                    ret.append(other_name)
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
    env.AddMethod(ParseCompilerType, 'ParseCompilerType')
    env.AddMethod(ParseCompilerDirectory, 'ParseCompilerDirectory')
    env.AddMethod(ParseLinkDirs, 'ParseLinkDirs')
    env.AddMethod(ParseConfigGuess, 'ParseConfigGuess')
    env.AddMethod(ParseMacosHost, 'ParseMacosHost')
    env.AddMethod(ParseMacosPlatform, 'ParseMacosPlatform')
    env.AddMethod(ParseMacosArch, 'ParseMacosArch')
    env.AddMethod(ParseList, 'ParseList')
