import SCons.SConf
import os.path
import hashlib

def CheckLibWithHeaderExpr(context, libs, headers, language, expr):
    if not isinstance(headers, list):
        headers = [headers]

    if not isinstance(libs, list):
        libs = [libs]

    name = libs[0]
    libs = [l for l in libs if not l in context.env['LIBS']]

    suffix = '.%s' % language.lower()
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

def CheckLibWithHeaderUniq(context, libs, headers, language, run):
    if run:
        return context.CheckLibWithHeaderExpr(libs, headers, language, '1')
    else:
        return context.CheckLibWithHeader(libs, headers, language)

def CheckProg(context, prog):
    context.Message("Checking for executable %s... " % prog)

    path = context.env.Which(prog)
    if path:
        context.Result(path[0])
        return True
    else:
        context.Result('not found')
        return False

def FindTool(context, var, toolchain, version, commands, prepend_path=[]):
    env = context.env

    context.Message("Looking for %s executable... " % var)

    if env.HasArg(var):
        context.Result(env[var])
        return True

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

            default_ver = env.ParseCompilerVersion(tool)

            if default_ver and default_ver[:len(version)] == version:
                search_versions += [default_ver]

            for ver in reversed(sorted(set(search_versions))):
                versioned_tool = '%s-%s' % (tool, '.'.join(map(str, ver)))
                if env.Which(versioned_tool, prepend_path):
                    tool = versioned_tool
                    break

        tool_path = env.Which(tool, prepend_path)
        if tool_path:
            env[var] = tool_path[0]
            if tool_flags:
                env['%sFLAGS' % var] = ' '.join(tool_flags)
            break
    else:
        env.Die("can't detect %s: looked for any of: %s" % (
            var,
            ', '.join([' '.join(c) if isinstance(c, list) else c for c in commands])))

    if version:
        actual_ver = env.ParseCompilerVersion(env[var])
        if actual_ver:
            actual_ver = actual_ver[:len(version)]

        if actual_ver != version:
            env.Die(
                "can't detect %s: '%s' not found in PATH, '%s' version is %s" % (
                    var,
                    '%s-%s' % (tool, '.'.join(map(str, version))),
                    env[var],
                    '.'.join(map(str, actual_ver))))

    message = env[var]
    realpath = os.path.realpath(env[var])
    if realpath != env[var]:
        message += ' (%s)' % realpath

    context.Result(message)
    return True

def FindLLVMDir(context, version):
    context.Message(
        "Checking for PATH for llvm %s... " % '.'.join(map(str, version)))

    def macos_dirs():
        return [
        '/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin',
        '/Library/Developer/CommandLineTools/usr/bin',
        ]

    def linux_dirs():
        suffixes = []
        for n in [3, 2, 1]:
            v = '.'.join(map(str, version[:n]))
            suffixes += [
                '-' + v,
                '/' + v,
            ]
        suffixes += ['']
        ret = []
        for s in suffixes:
            ret.append('/usr/lib/llvm%s/bin' % s)
        return ret

    for llvmdir in macos_dirs() + linux_dirs():
        if os.path.isdir(llvmdir):
            context.env['ENV']['PATH'] += ':' + llvmdir
            context.Result(llvmdir)
            return True

    context.Result('not found')
    return True

def init(env):
    env.CustomTests = {
        'CheckLibWithHeaderExpr': CheckLibWithHeaderExpr,
        'CheckLibWithHeaderUniq': CheckLibWithHeaderUniq,
        'CheckProg': CheckProg,
        'FindTool': FindTool,
        'FindLLVMDir': FindLLVMDir,
    }
