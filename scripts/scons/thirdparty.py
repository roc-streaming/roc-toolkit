import os
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
        env, build_root, toolchain, variant, versions, name,
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

    project_root = env.Dir('#').srcnode().abspath
    build_root = env.Dir(build_root).abspath
    thirdparty_dir = os.path.join(build_root, 'build', vname)

    if not os.path.exists(os.path.join(thirdparty_dir, 'commit')):
        save_cwd = os.getcwd()
        os.chdir(project_root)

        if env.Execute(
            '%s scripts/build/3rdparty.py %s 3rdparty/distfiles %s %s %s %s %s' % (
                quote(env.PythonExecutable()),
                quote(os.path.relpath(build_root, project_root)),
                quote(toolchain),
                quote(variant),
                quote(vname),
                quote(':'.join(vdeps)),
                ' '.join(envvars)),
            cmdstr = env.PrettyCommand(
                'GET', os.path.relpath(thirdparty_dir, project_root), 'yellow')):

            logfile = os.path.join(thirdparty_dir, 'build.log')
            message = "can't make '%s', see '%s' for details" % (
                vname, os.path.relpath(logfile, project_root))

            if os.environ.get('CI', '') in ['1', 'true']:
                try:
                    with open(logfile) as fp:
                        message += "\n\n" + fp.read()
                except:
                    pass

            env.Die('%s', message)

        os.chdir(save_cwd)

    env.ImportThridParty(
        build_root, toolchain, versions, name, includes, libs)

def ImportThridParty(env, build_root, toolchain, versions, name,
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
            '%s/build/%s/include/%s' % (build_root, vname, s)
        ])

    libdir = '%s/build/%s/lib' % (build_root, vname)

    if os.path.isdir(env.Dir(libdir).abspath):
        env.Prepend(LIBPATH=[libdir])

        for lib in env.GlobRecursive(env.Dir(libdir).abspath, 'lib*'):
            if needlib(lib.path):
                env.Prepend(LIBS=[env.File(lib)])

def init(env):
    env.AddMethod(ParseThirdParties, 'ParseThirdParties')
    env.AddMethod(ThirdParty, 'ThirdParty')
    env.AddMethod(ImportThridParty, 'ImportThridParty')
