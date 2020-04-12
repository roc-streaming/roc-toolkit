import os

def AddPkgConfigLibs(env, libs):
    env.AppendUnique(LIBS=libs)

    if 'PKG_CONFIG_LIBS' not in env.Dictionary():
        env['PKG_CONFIG_LIBS'] = []
    env.AppendUnique(PKG_CONFIG_LIBS=libs)

def GeneratePkgConfig(env, build_dir, filename, prefix, libdir, name, desc, url, version):
    output_dir = env.Dir(build_dir).path
    target = os.path.join(output_dir, filename)
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

Libs: -L${libdir} %(libs)s
Cflags: -I${includedir}
""" % { 'prefix': prefix,
        'libdir': libdir,
        'name': name,
        'dependencies': ' '.join(dependencies),
        'libs': ' '.join(['-l%s' % lib for lib in libs]),
        'version': version,
        'desc': desc,
        'url': url }

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    f = open(target, 'w')
    f.write(src)
    f.close()
    return env.File(target)

def init(env):
    env.AddMethod(AddPkgConfigLibs, 'AddPkgConfigLibs')
    env.AddMethod(GeneratePkgConfig, 'GeneratePkgConfig')
