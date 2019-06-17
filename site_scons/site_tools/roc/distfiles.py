import SCons.Script
import os
import shutil

def AddDistFile(env, prefix, subdir, target, hooks=[]):
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
            if os.path.isdir(dst):
                shutil.rmtree(dst)
            shutil.copytree(src, dst)
        elif os.path.islink(src):
            if os.path.isfile(dst):
                os.remove(dst)
            os.symlink(os.readlink(src), dst)
        else:
            shutil.copy(src, dst)

    def uninstall(target, source, env):
        if os.path.exists(dst):
            if os.path.isdir(dst):
                shutil.rmtree(dst)
            else:
                os.remove(dst)

    env.AlwaysBuild(env.Alias('install', [], [
        env.Action(install,
                   env.PrettyCommand('INSTALL', dst, 'yellow', 'install(%s)' % dst))
    ]))

    env.AlwaysBuild(env.Alias('uninstall', [], [
        env.Action(uninstall,
                   env.PrettyCommand('UNINSTALL', dst, 'red', 'uninstall(%s)' % dst))
    ]))

def AddDistAction(env, action):
    env.AlwaysBuild(env.Alias('install', [], action))

def init(env):
    env.AlwaysBuild(env.Alias('install', [], env.Action('')))
    env.AlwaysBuild(env.Alias('uninstall', [], env.Action('')))
    env.AddMethod(AddDistFile, 'AddDistFile')
    env.AddMethod(AddDistAction, 'AddDistAction')
