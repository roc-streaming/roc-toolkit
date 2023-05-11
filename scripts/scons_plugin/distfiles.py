import SCons.Script
import os
import shutil

def GetDistPath(env, instdir, *args):
    if env.HasArgument('DESTDIR'):
        seps = os.sep+os.altsep if os.altsep else os.sep
        instdir = os.path.join(
            env['DESTDIR'],
            os.path.splitdrive(instdir)[1].lstrip(seps))

    return os.path.join(*([instdir] + list(args)))

def AddDistFile(env, instdir, target):
    if isinstance(target, list):
        target = target[0]

    if isinstance(target, str):
        try:
            target = env.Dir(target)
        except:
            target = env.File(target)

    src = target.path
    dst = env.GetDistPath(instdir, target.name)

    def _install(target, source, env):
        if os.path.isdir(src):
            if os.path.isdir(dst):
                shutil.rmtree(dst)
            shutil.copytree(src, dst)
        elif os.path.islink(src):
            if os.path.isfile(dst):
                os.remove(dst)
            os.symlink(os.readlink(src), dst)
        else:
            par = os.path.dirname(dst)
            if not os.path.exists(par):
                os.makedirs(par)
            shutil.copy(src, dst)

    def _uninstall(target, source, env):
        if os.path.exists(dst):
            if os.path.isdir(dst):
                shutil.rmtree(dst)
            else:
                os.remove(dst)

    env.AlwaysBuild(env.Alias('install', [], [
        env.Action(_install,
                   env.PrettyCommand('INSTALL', dst, 'yellow', 'install({})'.format(dst)))
    ]))

    env.AlwaysBuild(env.Alias('uninstall', [], [
        env.Action(_uninstall,
                   env.PrettyCommand('UNINSTALL', dst, 'red', 'uninstall({})'.format(dst)))
    ]))

def AddDistAction(env, action):
    env.AlwaysBuild(env.Alias('install', [], action))

def init(env):
    env.AlwaysBuild(env.Alias('install', [], env.Action('')))
    env.AlwaysBuild(env.Alias('uninstall', [], env.Action('')))
    env.AddMethod(GetDistPath, 'GetDistPath')
    env.AddMethod(AddDistFile, 'AddDistFile')
    env.AddMethod(AddDistAction, 'AddDistAction')
