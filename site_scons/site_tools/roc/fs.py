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

def GlobFiles(env, pattern):
    ret = []
    for path in env.Glob(pattern):
        if os.path.isfile(path.srcnode().abspath):
            ret.append(path)
    return ret

def GlobDirs(env, pattern):
    ret = []
    for path in env.Glob(pattern):
        if os.path.isdir(path.srcnode().abspath):
            ret.append(path)
    return ret

def getenv(env, name, default):
    if name in env['ENV']:
        return env['ENV'][name]
    return os.environ.get(name, default)

def which(env, prog, mode, searchpath):
    result = []
    exts = list(filter(None, getenv(env, 'PATHEXT', '').split(os.pathsep)))
    for p in searchpath.split(os.pathsep):
        p = os.path.join(p, prog)
        if os.access(p, mode):
            result.append(p)
        for e in exts:
            pext = p + e
            if os.access(pext, mode):
                result.append(pext)
    return result

def Which(env, prog, prepend_path=[]):
    if os.access(prog, os.X_OK):
        return [prog]

    searchpath = getenv(env, 'PATH', os.defpath)
    if prepend_path:
        searchpath = os.pathsep.join(prepend_path) + os.pathsep + searchpath

    paths = []
    try:
        path = shutil.which(prog, os.X_OK, searchpath)
        if path:
            paths = [path]
    except:
        paths = which(env, prog, os.X_OK, searchpath)

    return paths

def init(env):
    env.AddMethod(GlobRecursive, 'GlobRecursive')
    env.AddMethod(GlobFiles, 'GlobFiles')
    env.AddMethod(GlobDirs, 'GlobDirs')
    env.AddMethod(Which, 'Which')
