import os

def AddPkgConfigLibs(env, libs):
    env.AppendUnique(LIBS=libs)

    if 'PKG_CONFIG_LIBS' not in env.Dictionary():
        env['PKG_CONFIG_LIBS'] = []
    env.AppendUnique(PKG_CONFIG_LIBS=libs)

def GeneratePkgConfig(env, build_dir, filename, prefix, libdir, name, desc, url, version):
<<<<<<< HEAD
    output_dir = env.Dir(build_dir).path
    target = os.path.join(output_dir, filename)
=======
    target = os.path.join(build_dir, filename)
>>>>>>> d8f74d5d3fb22f41808e9a1d19ad46742ca33476
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

<<<<<<< HEAD
Libs: -L${libdir} %(libs)s
=======
Libs: -L${libdir} -lroc %(libs)s
>>>>>>> d8f74d5d3fb22f41808e9a1d19ad46742ca33476
Cflags: -I${includedir}
""" % { 'prefix': prefix,
        'libdir': libdir,
        'name': name,
        'dependencies': ' '.join(dependencies),
        'libs': ' '.join(['-l%s' % lib for lib in libs]),
        'version': version,
        'desc': desc,
        'url': url }

<<<<<<< HEAD
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    f = open(target, 'w')
    f.write(src)
    f.close()
=======
    def write_file(target, source, env):
        f = open(target[0].path, 'w')
        f.write(src)
        f.close()

    env.Command(target, [], [
        env.Action(write_file, env.PrettyCommand('GEN', env.File(target).path, 'purple')),
        ])

>>>>>>> d8f74d5d3fb22f41808e9a1d19ad46742ca33476
    return env.File(target)

def init(env):
    env.AddMethod(AddPkgConfigLibs, 'AddPkgConfigLibs')
    env.AddMethod(GeneratePkgConfig, 'GeneratePkgConfig')
