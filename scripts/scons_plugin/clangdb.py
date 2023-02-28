import os

def GetClangDbWriter(env, tool, build_dir):
    return (
        '{python_cmd} scripts/scons_helpers/clangdb.py '
        '"{root_dir}" "{build_dir}" "{compiler}"'.format(
            python_cmd=env.GetPythonExecutable(),
            root_dir=env.Dir('#').path,
            build_dir=env.Dir(build_dir).path,
            compiler=tool
        )
    )

def init(env):
    env.AddMethod(GetClangDbWriter, 'GetClangDbWriter')
