import os

def WrapLauncher(env, tool, launcher):
    return '"{launcher}" "{tool}"'.format(
        launcher=launcher, tool=tool)

def WrapClangDb(env, tool, build_dir):
    if not tool.startswith('"'):
        tool = '"{}"'.format(tool)

    return (
        '{python_cmd} scripts/scons_helpers/clangdb.py '
        '"{root_dir}" "{build_dir}" {tool}'.format(
            python_cmd=env.GetPythonExecutable(),
            root_dir=env.Dir('#').path,
            build_dir=env.Dir(build_dir).path,
            tool=tool
        )
    )

def init(env):
    env.AddMethod(WrapLauncher, 'WrapLauncher')
    env.AddMethod(WrapClangDb, 'WrapClangDb')
