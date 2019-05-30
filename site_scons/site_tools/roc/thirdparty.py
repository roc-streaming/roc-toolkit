import SCons.Script
import os.path
import fnmatch

def _versioned_thirdparty(env, name, versions):
    if not name in versions:
        env.Die("unknown 3rdparty '%s'" % name)
    return name + '-' + versions[name]

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

def ThirdParty(
        env, host, toolchain, variant, versions, name, deps=[], includes=[], libs=['*']):
    vname = _versioned_thirdparty(env, name, versions)
    vdeps = []
    for dep in deps:
        vdeps.append(_versioned_thirdparty(env, dep, versions))

    if not os.path.exists(os.path.join('3rdparty', host, 'build', vname, 'commit')):
        if env.Execute(
            SCons.Action.CommandAction(
                '%s scripts/3rdparty.py "3rdparty/%s" "vendor" """%s" "%s" "%s" "%s"' % (
                    env.PythonExecutable(),
                    host,
                    toolchain,
                    variant,
                    vname,
                    ':'.join(vdeps)),
                cmdstr = env.PrettyCommand('GET', '%s/%s' % (host, vname), 'yellow'))):
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

    vname = _versioned_thirdparty(env, name, versions)

    if not includes:
        includes = ['']

    for s in includes:
        env.Prepend(CPPPATH=[
            '#3rdparty/%s/build/%s/include/%s' % (host, vname, s)
        ])

    libdir = '#3rdparty/%s/build/%s/lib' % (host, vname)

    if os.path.isdir(env.Dir(libdir).abspath):
        env.Prepend(LIBPATH=[libdir])

        for lib in env.GlobRecursive(libdir, 'lib*'):
            if needlib(lib.path):
                env.Prepend(LIBS=[env.File(lib)])

def init(env):
    env.AddMethod(ParseThirdParties, 'ParseThirdParties')
    env.AddMethod(ThirdParty, 'ThirdParty')
    env.AddMethod(ImportThridParty, 'ImportThridParty')
