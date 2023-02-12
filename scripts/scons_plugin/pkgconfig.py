import os

def AddPkgConfigLibs(env, libs):
    env.AppendUnique(LIBS=libs)

    if 'PKG_CONFIG_LIBS' not in env.Dictionary():
        env['PKG_CONFIG_LIBS'] = []
    env.AppendUnique(PKG_CONFIG_LIBS=libs)

def GeneratePkgConfig(env, build_dir, filename, prefix, libdir, name, desc, url, version):
    target = os.path.join(build_dir, filename)
    dependencies = env.get('PKG_CONFIG_DEPS', [])
    libs = env.get('PKG_CONFIG_LIBS', [])

    src = """prefix=%(prefix)s
exec_prefix=${prefix}
libdir=%(libdir)s
includedir=${prefix}/include

Name: %(name)s
Requires: %(dependencies)s
Version: %(version)s
Description: %(desc)s
URL: %(url)s

Libs: -L${libdir} -lroc %(libs)s
Cflags: -I${includedir}
""" % { 'prefix': prefix,
        'libdir': libdir,
        'name': name,
        'dependencies': ' '.join(dependencies),
        'libs': ' '.join(['-l%s' % lib for lib in libs]),
        'version': version,
        'desc': desc,
        'url': url }

    def write_file(target, source, env):
        f = open(target[0].path, 'w')
        f.write(src)
        f.close()

    env.Command(target, [], [
        env.Action(write_file, env.PrettyCommand('GEN', env.File(target).path, 'purple')),
        ])

    return env.File(target)

# Find and return path to the pkgconfig directory of the latest version of `lib_name`
# library installed by brew.
#
# When `required` = True error is raised unless the search was successful. Otherwise in
# case of any error None is returned.
def FindBrewLibrary(env, lib_name, required=True):
    if not env.Which('brew'):
        if required:
            env.Die('brew not found')
        else:
            return None

    # $(brew --prefix)/opt/some_lib should be a link to the latest version of some_lib,
    # e.g. ../Cellar/some_lib@1.2/1.2.3/
    pkg_config_path = os.path.join(
        env.GetCommandOutput('brew --prefix'), 'opt', lib_name, 'lib', 'pkgconfig')

    if not os.path.isdir(pkg_config_path):
        if required:
            env.Die('pkgconfig directory for library {} not found in {}'.
                format(lib_name, pkg_config_path))
        else:
            return None
    return pkg_config_path

def init(env):
    env.AddMethod(AddPkgConfigLibs, 'AddPkgConfigLibs')
    env.AddMethod(FindBrewLibrary, 'FindBrewLibrary')
    env.AddMethod(GeneratePkgConfig, 'GeneratePkgConfig')
