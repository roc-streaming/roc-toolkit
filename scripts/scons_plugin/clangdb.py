import os

def GetClangDbWriter(env, tool, build_dir):
    return '%s scripts/scons_helpers/clangdb.py "%s" "%s" "%s"' % (
        env.GetPythonExecutable(),
        env.Dir('#').path,
        env.Dir(build_dir).path,
        tool)

def init(env):
    env.AddMethod(GetClangDbWriter, 'GetClangDbWriter')
