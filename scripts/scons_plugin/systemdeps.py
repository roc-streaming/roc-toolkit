import os
import textwrap

# Python 2 compatibility.
try:
    from shlex import quote
except:
    from pipes import quote

# Generate .pc file for roc.
# This function uses information accumulated by:
#  - AddPkgConfigDependency - list of .pc files of system dependencies
#  - AddManualDependency - list of libs and search paths of system dependencies
#    (for cases when system dependency does not have .pc file)
# All this information is included into resulting .pc file.
# To work properly, GeneratePkgConfig should be called after aforementioned functions.
def GeneratePkgConfig(env, build_dir, filename, prefix, libdir, name, desc, url, version):
    target = os.path.join(build_dir, filename)

    dep_list = env.get('_DEPS_PCFILES', [])

    lib_list = \
      ['roc'] + \
      env.get('_DEPS_LIBS', [])

    libdir_list = \
      [libdir] + \
      env.get('_DEPS_LIBPATH', [])

    incdir_list = \
      [prefix+'/include'] + \
      env.get('_DEPS_CPPPATH', [])

    src = textwrap.dedent("""\
        prefix={prefix}
        exec_prefix=${{prefix}}

        Name: {name}
        Description: {desc}
        URL: {url}
        Version: {version}
        Requires: {deps}

        Libs: {libdirs} {libs}
        Cflags: {incdirs}
    """).format(
        prefix=prefix,
        name=name,
        desc=desc,
        url=url,
        version=version,
        deps=' '.join(dep_list),
        libs=' '.join(['-l'+s for s in lib_list]),
        libdirs=' '.join(['-L'+quote(s) for s in libdir_list]),
        incdirs=' '.join(['-I'+quote(s) for s in incdir_list]))

    def write_file(target, source, env):
        f = open(target[0].path, 'w')
        f.write(src)
        f.close()

    env.Command(target, [], [
        env.Action(write_file, env.PrettyCommand('GEN', env.File(target).path, 'purple')),
        ])

    return env.File(target)

# Add system dependency manually. Used as fallback when AddPkgConfigDependency did not
# work or can't be used.
# If libs arg is provided, libs are added to linker options.
# If prefix arg is provided, and it defines a dir which has include and lib subdirs,
# those subdirs are added to the headers and library search paths.
# This function also automatically informs GeneratePkgConfig that correspondings
# paths and libs should be added to .pc file.
def AddManualDependency(env, libs=[], prefix=None, exclude_from_pc=False):
    def _append_var(var, values):
        if var not in env.Dictionary():
            env[var] = []
        env.AppendUnique(**{var: values})

    if libs:
        _append_var('LIBS', libs)
        if not exclude_from_pc:
            _append_var('_DEPS_LIBS', libs)

    if prefix:
        lib_dir = os.path.join(prefix, 'lib')
        if os.path.isdir(lib_dir):
            _append_var('LIBPATH', [lib_dir])
            if not exclude_from_pc:
                _append_var('_DEPS_LIBPATH', [lib_dir])

        inc_dir = os.path.join(prefix, 'include')
        if os.path.isdir(inc_dir):
            _append_var('CPPPATH', [inc_dir])
            if not exclude_from_pc:
                _append_var('_DEPS_CPPPATH', [inc_dir])

# Find and return path to the package (its install prefix directory) of the latest version
# of `pkg_name` package installed by brew. If not found, None is returned.
def FindBrewPackage(env, pkg_name):
    if not env.Which('brew'):
        return None

    brew_prefix = env.GetCommandOutput('brew --prefix')
    if not brew_prefix:
        return None

    # if pkg-config is not from brew, then don't try to use brew packages implicitly,
    # that's likely not what the user wants
    pkg_config = env.get('PKG_CONFIG', None)
    if not pkg_config or \
       not env.Which(pkg_config) or \
       not os.path.abspath(env.Which(pkg_config)[0]).startswith(
           os.path.abspath(brew_prefix)+os.sep):
        return None

    # $(brew --prefix)/opt/some_pkg should be a link to the latest version of some_pkg,
    # e.g. ../Cellar/some_pkg@1.2/1.2.3/
    pkg_prefix = os.path.join(brew_prefix, 'opt', pkg_name)
    if not os.path.isdir(pkg_prefix):
        return None

    return pkg_prefix

def init(env):
    env.AddMethod(GeneratePkgConfig, 'GeneratePkgConfig')
    env.AddMethod(AddManualDependency, 'AddManualDependency')
    env.AddMethod(FindBrewPackage, 'FindBrewPackage')
