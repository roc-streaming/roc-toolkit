import SCons.Script
import os
import shutil

def AddDistfile(env, prefix, subdir, target):
    if isinstance(target, list):
        target = target[0]

    if isinstance(target, str):
        try:
            target = env.Dir(target)
        except:
            target = env.File(target)

    src = target.path
    dst = os.path.join(prefix, subdir, target.name)

    def install(target, source, env):
        if os.path.isdir(src):
            shutil.rmtree(dst)
            shutil.copytree(src, dst)
        else:
            shutil.copy(src, dst)

    def uninstall(target, source, env):
        if os.path.exists(dst):
            if os.path.isdir(dst):
                shutil.rmtree(dst)
            else:
                os.remove(dst)

    env.AlwaysBuild(env.Alias('install', [], [
        env.Action(install, env.Pretty('INSTALL', dst, 'yellow', 'install(%s)' % dst))
    ]))

    env.AlwaysBuild(env.Alias('uninstall', [], [
        env.Action(uninstall, env.Pretty('UNINSTALL', dst, 'red', 'uninstall(%s)' % dst))
    ]))

def Init(env):
    env.AlwaysBuild(env.Alias('install', [], env.Action('')))
    env.AlwaysBuild(env.Alias('uninstall', [], env.Action('')))

    env.AddMethod(AddDistfile, 'AddDistfile')
