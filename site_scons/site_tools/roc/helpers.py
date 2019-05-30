from __future__ import print_function

import sys
import os
import os.path
import shutil
import fnmatch
import copy
import re
import subprocess
import hashlib

import SCons.Script
import SCons.Util
import SCons.SConf

def Die(env, fmt, *args):
    print('error: ' + (fmt % args).strip() + '\n', file=sys.stderr)
    SCons.Script.Exit(1)

def GlobDirs(env, pattern):
    for path in env.Glob(pattern):
        if os.path.isdir(path.srcnode().abspath):
            yield path

def RecursiveGlob(env, dirs, patterns, exclude=[]):
    if not isinstance(dirs, list):
        dirs = [dirs]

    if not isinstance(patterns, list):
        patterns = [patterns]

    if not isinstance(exclude, list):
        exclude = [exclude]

    matches = []

    for pattern in patterns:
        for root in dirs:
            for root, dirnames, filenames in os.walk(env.Dir(root).srcnode().abspath):
                for names in [dirnames, filenames]:
                    for name in fnmatch.filter(names, pattern):
                        cwd = env.Dir('.').srcnode().abspath

                        abspath = os.path.join(root, name)
                        relpath = os.path.relpath(abspath, cwd)

                        for ex in exclude:
                            if fnmatch.fnmatch(relpath, ex):
                                break
                            if fnmatch.fnmatch(os.path.basename(relpath), ex):
                                break
                        else:
                            if names is dirnames:
                                matches.append(env.Dir(relpath))
                            else:
                                matches.append(env.File(relpath))

    return matches

def HasArg(env, var):
    return GetArg(env, var) is not None

def GetArg(env, var):
    ret = SCons.Script.ARGUMENTS.get(var, None)
    if ret is None:
        if var in os.environ:
            ret = os.environ[var]
    return ret

def AppendVars(env, src_env):
    for k, v in src_env.Dictionary().items():
        if not (isinstance(v, SCons.Util.CLVar) or isinstance(v, list)):
            continue
        env.AppendUnique(**{k: v})

def Which(env, prog):
    def getenv(name, default):
        if name in env['ENV']:
            return env['ENV'][name]
        return os.environ.get(name, default)
    result = []
    exts = filter(None, getenv('PATHEXT', '').split(os.pathsep))
    path = getenv('PATH', None)
    if path is None:
        return []
    for p in getenv('PATH', '').split(os.pathsep):
        p = os.path.join(p, prog)
        if os.access(p, os.X_OK):
            result.append(p)
        for e in exts:
            pext = p + e
            if os.access(pext, os.X_OK):
                result.append(pext)
    return result

def Python(env):
    base = os.path.basename(sys.executable)
    path = env.Which(base)
    if path and path[0] == sys.executable:
        return base
    else:
        return sys.executable

def CommandOutput(env, command):
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

def ToolVersion(env, command):
    text = env.CommandOutput(command)
    if not text:
        return None

    m = re.search(r'(\b[0-9][0-9.]+\b)', text)
    if not m:
        return None

    return m.group(1)

def CompilerVersion(env, compiler):
    def getverstr():
        try:
            version_formats = [
                r'(\b[0-9]+\.[0-9]+\.[0-9]+\b)',
                r'(\b[0-9]+\.[0-9]+\b)',
            ]

            full_text = env.CommandOutput([compiler, '--version'])

            for regex in version_formats:
                m = re.search(r'(?:LLVM|clang)\s+version\s+'+regex, full_text)
                if m:
                    return m.group(1)

            trunc_text = re.sub(r'\([^)]+\)', '', full_text)

            dump_text = env.CommandOutput([compiler, '-dumpversion'])

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

def CompilerTarget(env, compiler):
    text = env.CommandOutput([compiler, '-v', '-E', '-'])
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

def LLVMDir(env, version):
    suffixes = []
    for n in [3, 2, 1]:
        v = '.'.join(map(str, version[:n]))
        suffixes += [
            '-' + v,
            '/' + v,
        ]
    suffixes += ['']
    for s in suffixes:
        llvmdir = '/usr/lib/llvm' + s
        if os.path.isdir(llvmdir):
            return llvmdir

def ClangDB(env, build_dir, compiler):
    return '%s %s/wrappers/clangdb.py "%s" "%s" "%s"' % (
        env.Python(),
        env.Dir(os.path.dirname(__file__)).path,
        env.Dir('#').path,
        env.Dir(build_dir).path,
        compiler)

def Doxygen(env, build_dir='', html_dir=None, config='', sources=[], werror=False):
    target = os.path.join(env.Dir(build_dir).path, '.done')

    if not env.Which(env['DOXYGEN']):
        env.Die("doxygen not found in PATH (looked for '%s')" % env['DOXYGEN'])

    dirs = [env.Dir(build_dir).path]
    if html_dir:
        dirs += [env.Dir(html_dir).path]

    env.Command(target, sources + [config], SCons.Action.CommandAction(
        '%s %s/wrappers/doc.py %s %s %s %s %s %s %s' % (
            env.Python(),
            env.Dir(os.path.dirname(__file__)).path,
            env.Dir('#').path,
            env.Dir(os.path.dirname(config)).path,
            ':'.join(dirs),
            target,
            int(werror or 0),
            env['DOXYGEN'],
            env.File(config).name),
        cmdstr = env.Pretty('DOXYGEN', build_dir, 'purple')))

    return target

def Sphinx(env, output_type, build_dir, output_dir, source_dir, sources, werror=False):
    target = os.path.join(env.Dir(build_dir).path, source_dir, '.done.'+output_type)

    if not env.Which(env['SPHINX_BUILD']):
        env.Die("sphinx-build not found in PATH (looked for '%s')" % env['SPHINX_BUILD'])

    env.Command(target, sources, SCons.Action.CommandAction(
        '%s %s/wrappers/doc.py %s %s %s %s %s %s -q -b %s -d %s %s %s' % (
            env.Python(),
            env.Dir(os.path.dirname(__file__)).path,
            env.Dir('#').path,
            env.Dir('#').path,
            output_dir,
            target,
            int(werror or 0),
            env['SPHINX_BUILD'],
            output_type,
            env.Dir(os.path.join(build_dir, source_dir)).path,
            env.Dir(source_dir).path,
            env.Dir(output_dir).path),
        cmdstr = env.Pretty('SPHINX', output_dir, 'purple')))

    return env.File(target)

def GenGetOpt(env, source, ver):
    if 'GENGETOPT' in env.Dictionary():
        gengetopt = env['GENGETOPT']

    else:
        gengetopt = 'gengetopt'

    if isinstance(gengetopt, str):
        if not env.Which(gengetopt):
            env.Die("gengetopt not found in PATH (looked for '%s')" % gengetopt)
    else:
        gengetopt = env.File(gengetopt).path

    source = env.File(source)
    source_name = os.path.splitext(os.path.basename(source.path))[0]
    target = [
        os.path.join(str(source.dir), source_name + '.c'),
        os.path.join(str(source.dir), source_name + '.h'),
    ]

    env.Command(target, source, SCons.Action.CommandAction(
        '%s -i %s -F %s --output-dir=%s --set-version=%s' % (
            gengetopt,
            source.srcnode().path,
            source_name,
            os.path.dirname(source.path),
            ver),
        cmdstr = env.Pretty('GGO', '$SOURCE', 'purple')))

    ret = env.Object(target[0])

    # Workaround to avoid rebuilding returned object file twice.
    #
    # What happens here:
    #  1. When invoked first time, SCons generates .c from .ggo. When
    #     SCons scans .c files for #includes, .c doesn't exist yet,
    #     thus it's not scanned.
    #  2. When invoked second time, SCons scans genarated .c file and
    #     notes that is depends on generated .h file. Since it's new
    #     dependency for this file, SCons rebuilds it.
    #
    # We force implicit dependency .c -> .h, so that it's present even
    # if SCons didn't scan .c file yet, and no new dependencies will be
    # introduced on next invocation.
    ret[0].add_to_implicit([env.File(target[1])])

    return ret

def ParseSanitizers(env, s, sanitizers):
    if not s:
        return []
    ret = []
    for name in s.split(','):
        if name == 'all':
            for name in sanitizers:
                if not name in ret:
                    ret.append(name)
        else:
            if not name in ret:
                ret.append(name)
    return ret

def ParseThirdParties(env, s):
    ret = dict()
    if s:
        for t in s.split(','):
            tokens = t.split(':', 1)

            name = tokens[0]
            ver = None
            if name != 'all' and len(tokens) == 2:
                ver = tokens[1]

            ret[name] = ver
    return ret.items()

def _versioned(env, name, versions):
    if not name in versions:
        env.Die("unknown 3rdparty '%s'" % name)
    return name + '-' + versions[name]

def ThirdParty(
        env, host, toolchain, variant, versions, name, deps=[], includes=[], libs=['*']):
    vname = _versioned(env, name, versions)
    vdeps = []
    for dep in deps:
        vdeps.append(_versioned(env, dep, versions))

    if not os.path.exists(os.path.join('3rdparty', host, 'build', vname, 'commit')):
        if env.Execute(
            SCons.Action.CommandAction(
                '%s scripts/3rdparty.py "3rdparty/%s" "vendor" """%s" "%s" "%s" "%s"' % (
                    env.Python(),
                    host,
                    toolchain,
                    variant,
                    vname,
                    ':'.join(vdeps)),
                cmdstr = env.Pretty('GET', '%s/%s' % (host, vname), 'yellow'))):
            env.Die("can't make '%s', see '3rdparty/%s/build/%s/build.log' for details" % (
                vname, host, vname))

    env.ImportThridParty(
        host, toolchain, versions, name, includes, libs)

def ImportThridParty(env, host, toolchain, versions, name, includes=[], libs=['*']):
    def needlib(lib):
        for name in libs:
            if fnmatch.fnmatch(os.path.basename(lib), 'lib%s.*' % name):
                return True
        return False

    vname = _versioned(env, name, versions)

    if not includes:
        includes = ['']

    for s in includes:
        env.Prepend(CPPPATH=[
            '#3rdparty/%s/build/%s/include/%s' % (host, vname, s)
        ])

    libdir = '#3rdparty/%s/build/%s/lib' % (host, vname)

    if os.path.isdir(env.Dir(libdir).abspath):
        env.Prepend(LIBPATH=[libdir])

        for lib in env.RecursiveGlob(libdir, 'lib*'):
            if needlib(lib.path):
                env.Prepend(LIBS=[env.File(lib)])

def DeleteFile(env, path):
    path = env.File(path).path
    def rmfile(target, source, env):
        if os.path.exists(path):
            os.remove(path)
    return env.Action(rmfile, env.Pretty('RM', path, 'red', 'rm(%s)' % path))

def DeleteDir(env, path):
    path = env.Dir(path).path
    def rmtree(target, source, env):
        if os.path.exists(path):
            shutil.rmtree(path)
    return env.Action(rmtree, env.Pretty('RM', path, 'red', 'rm(%s)' % path))

def TryParseConfig(env, cmd):
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

def CheckLibWithHeaderExpr(context, libs, headers, language, expr):
    if not isinstance(headers, list):
        headers = [headers]

    if not isinstance(libs, list):
        libs = [libs]

    name = libs[0]
    libs = [l for l in libs if not l in context.env['LIBS']]

    suffix = '.%s' % language
    includes = '\n'.join(['#include <%s>' % h for h in ['stdio.h'] + headers])
    src = """
%s

int main() {
    printf("%%d\\n", (int)(%s));
    return 0;
}
""" % (includes, expr)

    context.Message("Checking for %s library %s... " % (
        language.upper(), name))

    # Workaround for a SCons bug.
    # RunProg uses a global incrementing counter for temporary .c file names. The
    # file name depends on the number of invocations of that function, but not on
    # the file contents. When the user subsequently invokes scons with different
    # options, the sequence of file contents passed to RunProg may vary. However,
    # RunProg may incorrectly use cached results from a previous run saved for
    # different file contents but the same invocation number. To prevent this, we
    # monkey patch its global counter with a hashsum of the file contents.
    SCons.SConf._ac_build_counter = int(hashlib.md5(src.encode()).hexdigest(), 16)
    err, out = context.RunProg(src, suffix)

    if not err and out.strip() != '0':
        context.Result('yes')
        context.env.Append(LIBS=libs)
        return True
    else:
        context.Result('no')
        return False

def CheckLibWithHeaderUniq(context, libs, headers, language):
    return CheckLibWithHeaderExpr(context, libs, headers, language, '1')

def CheckProg(context, prog, name=''):
    if name:
        name += ' '
    context.Message("Checking for %sexecutable %s... " % (name, prog))

    if context.env.Which(prog):
        context.Result('yes')
        return True
    else:
        context.Result('no')
        return False

def FindTool(context, var, toolchain, version, commands):
    env = context.env

    if env.HasArg(var):
        CheckProg(context, env[var], name=var)
        return

    for tool_cmd in commands:
        if isinstance(tool_cmd, list):
            tool_name = tool_cmd[0]
            tool_flags = tool_cmd[1:]
        else:
            tool_name = tool_cmd
            tool_flags = []

        if not toolchain:
            tool = tool_name
        else:
            tool = '%s-%s' % (toolchain, tool_name)

        if version:
            search_versions = [
                version[:3],
                version[:2],
                version[:1],
            ]

            default_ver = env.CompilerVersion(tool)

            if default_ver and default_ver[:len(version)] == version:
                search_versions += [default_ver]

            for ver in reversed(sorted(set(search_versions))):
                versioned_tool = '%s-%s' % (tool, '.'.join(map(str, ver)))
                if env.Which(versioned_tool):
                    tool = versioned_tool
                    break

        if env.Which(tool):
            env[var] = tool
            if tool_flags:
                env['%sFLAGS' % var] = ' '.join(tool_flags)
            break
    else:
        env.Die("can't detect %s: looked for any of: %s" % (
            var,
            ', '.join([' '.join(c) if isinstance(c, list) else c for c in commands])))

    CheckProg(context, env[var], name=var)

    if version:
        actual_ver = env.CompilerVersion(env[var])
        if actual_ver:
            actual_ver = actual_ver[:len(version)]

        if actual_ver != version:
            env.Die(
                "can't detect %s: '%s' not found in PATH, '%s' version is %s" % (
                    var,
                    '%s-%s' % (tool, '.'.join(map(str, version))),
                    env[var],
                    '.'.join(map(str, actual_ver))))

def Init(env):
    env.AddMethod(Die, 'Die')
    env.AddMethod(GlobDirs, 'GlobDirs')
    env.AddMethod(RecursiveGlob, 'RecursiveGlob')
    env.AddMethod(HasArg, 'HasArg')
    env.AddMethod(GetArg, 'GetArg')
    env.AddMethod(AppendVars, 'AppendVars')
    env.AddMethod(Which, 'Which')
    env.AddMethod(Python, 'Python')
    env.AddMethod(CommandOutput, 'CommandOutput')
    env.AddMethod(ToolVersion, 'ToolVersion')
    env.AddMethod(CompilerVersion, 'CompilerVersion')
    env.AddMethod(CompilerTarget, 'CompilerTarget')
    env.AddMethod(LLVMDir, 'LLVMDir')
    env.AddMethod(ClangDB, 'ClangDB')
    env.AddMethod(Doxygen, 'Doxygen')
    env.AddMethod(Sphinx, 'Sphinx')
    env.AddMethod(GenGetOpt, 'GenGetOpt')
    env.AddMethod(ParseSanitizers, 'ParseSanitizers')
    env.AddMethod(ParseThirdParties, 'ParseThirdParties')
    env.AddMethod(ThirdParty, 'ThirdParty')
    env.AddMethod(ImportThridParty, 'ImportThridParty')
    env.AddMethod(DeleteFile, 'DeleteFile')
    env.AddMethod(DeleteDir, 'DeleteDir')
    env.AddMethod(TryParseConfig, 'TryParseConfig')
    env.CustomTests = {
        'CheckLibWithHeaderExpr': CheckLibWithHeaderExpr,
        'CheckLibWithHeaderUniq': CheckLibWithHeaderUniq,
        'CheckProg': CheckProg,
        'FindTool': FindTool,
    }
