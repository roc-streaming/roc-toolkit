import os
import os.path
import fnmatch

try:
    from shlex import quote
except:
    from pipes import quote

def _get_versioned_thirdparty(env, name, versions):
    if not name in versions:
        env.Die("unknown 3rdparty '%s'" % name)
    return name + '-' + versions[name]

def _build_thirdparty(env, build_root, toolchain, variant, versions, name, deps, includes, libs):
    versioned_name = _get_versioned_thirdparty(env, name, versions)

    versioned_deps = []
    for dep in deps:
        versioned_deps.append(_get_versioned_thirdparty(env, dep, versions))

    env_vars = [
        'CXX=%s'    % quote(env['CXX']),
        'CXXLD=%s'  % quote(env['CXXLD']),
        'CC=%s'     % quote(env['CC']),
        'CCLD=%s'   % quote(env['CCLD']),
        'AR=%s'     % quote(env['AR']),
        'RANLIB=%s' % quote(env['RANLIB']),
    ]

    if 'PKG_CONFIG' in env.Dictionary():
        env_vars += ['PKG_CONFIG=%s' % quote(env['PKG_CONFIG'])]

    project_root = env.Dir('#').srcnode().abspath
    build_root = env.Dir(build_root).abspath
    thirdparty_dir = os.path.join(build_root, versioned_name)

    if not os.path.exists(os.path.join(thirdparty_dir, 'commit')):
        saved_cwd = os.getcwd()
        os.chdir(project_root)

        if env.Execute(
            '%s scripts/scons_helpers/build-3rdparty.py %s 3rdparty/_distfiles %s %s %s %s %s' % (
                quote(env.GetPythonExecutable()),
                quote(os.path.relpath(build_root, project_root)),
                quote(toolchain),
                quote(variant),
                quote(versioned_name),
                quote(':'.join(versioned_deps)),
                ' '.join(env_vars)),
            cmdstr = env.PrettyCommand(
                'BUILD', os.path.relpath(thirdparty_dir, project_root), 'yellow')):

            logfile = os.path.join(thirdparty_dir, 'build.log')
            message = "can't make '%s', see '%s' for details" % (
                versioned_name, os.path.relpath(logfile, project_root))

            if os.environ.get('CI', '') in ['1', 'true']:
                try:
                    with open(logfile) as fp:
                        message += "\n\n" + fp.read()
                except:
                    pass

            env.Die('%s', message)

        os.chdir(saved_cwd)

def _import_thridparty(env, build_root, toolchain, variant, versions, name, deps, includes, libs):
    def needlib(lib):
        for name in libs:
            if fnmatch.fnmatch(os.path.basename(lib), 'lib%s.*' % name):
                return True
        return False

    versioned_name = _get_versioned_thirdparty(env, name, versions)

    if not includes:
        includes = ['']

    for s in includes:
        incdir  ='%s/%s/include' % (build_root, versioned_name)
        if s:
            incdir += '/' + s

        if os.path.isdir(env.Dir(incdir).abspath):
            env.Prepend(CPPPATH=[incdir])

    libdir = '%s/%s/lib' % (build_root, versioned_name)
    rpathdir = '%s/%s/rpath' % (build_root, versioned_name)

    if os.path.isdir(env.Dir(libdir).abspath):
        env.Prepend(LIBPATH=[libdir])

        if env['ROC_PLATFORM'] == 'linux':
            env.Prepend(LINKFLAGS=[
                '-Wl,-rpath-link,%s' % env.Dir(rpathdir).path,
            ])

        for lib in env.GlobRecursive(env.Dir(libdir).abspath, 'lib*'):
            if needlib(lib.path):
                env.Prepend(LIBS=[env.File(lib)])
                env.Append(_THIRDPARTY_LIBS=[lib.abspath])

def ParseThirdPartyList(env, s):
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

def BuildThirdParty(
        env, build_root, toolchain, variant, versions, name,
        deps=[], includes=[], libs=['*']):
    _build_thirdparty(
        env, build_root, toolchain, variant, versions, name, deps, includes, libs)

    _import_thridparty(
        env, build_root, toolchain, variant, versions, name, deps, includes, libs)

def GetThirdPartyExecutable(env, build_root, versions, name, exe_name):
    return env.File(
        '%s/%s/bin/%s%s' % (
            build_root,
            _get_versioned_thirdparty(env, name, versions),
            exe_name,
            env['PROGSUFFIX']))

def GetThirdPartyStaticLibs(env):
    all_libs = env.get('_THIRDPARTY_LIBS', [])
    static_libs = []

    for lib in all_libs:
        if lib.endswith(env['LIBSUFFIX']):
            static_libs.append(lib)

    return static_libs

def init(env):
    env.AddMethod(ParseThirdPartyList, 'ParseThirdPartyList')
    env.AddMethod(BuildThirdParty, 'BuildThirdParty')
    env.AddMethod(GetThirdPartyExecutable, 'GetThirdPartyExecutable')
    env.AddMethod(GetThirdPartyStaticLibs, 'GetThirdPartyStaticLibs')
