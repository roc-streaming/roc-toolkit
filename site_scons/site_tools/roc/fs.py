import os
import os.path
import shutil
import fnmatch

def GlobRecursive(env, dirs, patterns, exclude=[]):
    if not isinstance(dirs, list):
        dirs = [dirs]

    if not isinstance(patterns, list):
        patterns = [patterns]

    if not isinstance(exclude, list):
        exclude = [exclude]

    matches = []

    for pattern in patterns:
        for root in dirs:
            for root, dirnames, filenames in os.walk(env.Dir(root).srcnode().abspath):
                for names in [dirnames, filenames]:
                    for name in fnmatch.filter(names, pattern):
                        cwd = env.Dir('.').srcnode().abspath

                        abspath = os.path.join(root, name)
                        relpath = os.path.relpath(abspath, cwd)

                        for ex in exclude:
                            if fnmatch.fnmatch(relpath, ex):
                                break
                            if fnmatch.fnmatch(os.path.basename(relpath), ex):
                                break
                        else:
                            if names is dirnames:
                                matches.append(env.Dir(relpath))
                            else:
                                matches.append(env.File(relpath))

    return matches

def GlobDirs(env, pattern):
    for path in env.Glob(pattern):
        if os.path.isdir(path.srcnode().abspath):
            yield path

def getenv(env, name, default):
    if name in env['ENV']:
        return env['ENV'][name]
    return os.environ.get(name, default)

def which(env, prog, mode):
    result = []
    exts = filter(None, getenv(env, 'PATHEXT', '').split(os.pathsep))
    path = getenv(env, 'PATH', None)
    if path is None:
        return []
    for p in getenv(env, 'PATH', '').split(os.pathsep):
        p = os.path.join(p, prog)
        if os.access(p, mode):
            result.append(p)
        for e in exts:
            pext = p + e
            if os.access(pext, mode):
                result.append(pext)
    return result

def Which(env, prog):
    try:
        path = shutil.which(prog, os.X_OK, getenv(env, 'PATH', os.defpath))
        if path:
            return [path]
        else:
            return []
    except:
        return which(env, prog, os.X_OK)

def init(env):
    env.AddMethod(GlobRecursive, 'GlobRecursive')
    env.AddMethod(GlobDirs, 'GlobDirs')
    env.AddMethod(Which, 'Which')
