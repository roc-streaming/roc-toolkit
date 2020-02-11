import SCons.Script
import os.path
import fnmatch

try:
    from shlex import quote
except:
    from pipes import quote

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
        env, hostdir, compilerdir, toolchain, variant, versions, name,
        deps=[], includes=[], libs=['*']):
    vname = _versioned_thirdparty(env, name, versions)
    vdeps = []
    for dep in deps:
        vdeps.append(_versioned_thirdparty(env, dep, versions))

    envvars = [
        'CXX=%s'    % quote(env['CXX']),
        'CXXLD=%s'  % quote(env['CXXLD']),
        'CC=%s'     % quote(env['CC']),
        'CCLD=%s'   % quote(env['CCLD']),
        'AR=%s'     % quote(env['AR']),
        'RANLIB=%s' % quote(env['RANLIB']),
    ]

    if not os.path.exists(os.path.join(
        '3rdparty', hostdir, compilerdir, 'build', vname, 'commit')):
        if env.Execute(
            SCons.Action.CommandAction(
                '%s scripts/3rdparty.py %s vendor %s %s %s %s %s' % (
                    quote(env.PythonExecutable()),
                    quote(os.path.join("3rdparty", hostdir, compilerdir)),
                    quote(toolchain),
                    quote(variant),
                    quote(vname),
                    quote(':'.join(vdeps)),
                    ' '.join(envvars)),
                cmdstr = env.PrettyCommand(
                    'GET', '3rdparty/%s/%s/build/%s' % (
                        hostdir, compilerdir, vname), 'yellow'))):
            env.Die("can't make '%s', see '3rdparty/%s/%s/build/%s/build.log' for details" % (
                vname, hostdir, compilerdir, vname))

    env.ImportThridParty(
        hostdir, compilerdir, toolchain, versions, name, includes, libs)

def ImportThridParty(env, hostdir, compilerdir, toolchain, versions, name,
                     includes=[], libs=['*']):
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
            '#3rdparty/%s/%s/build/%s/include/%s' % (hostdir, compilerdir, vname, s)
        ])

    libdir = '#3rdparty/%s/%s/build/%s/lib' % (hostdir, compilerdir, vname)

    if os.path.isdir(env.Dir(libdir).abspath):
        env.Prepend(LIBPATH=[libdir])

        for lib in env.GlobRecursive(libdir, 'lib*'):
            if needlib(lib.path):
                env.Prepend(LIBS=[env.File(lib)])

def init(env):
    env.AddMethod(ParseThirdParties, 'ParseThirdParties')
    env.AddMethod(ThirdParty, 'ThirdParty')
    env.AddMethod(ImportThridParty, 'ImportThridParty')
