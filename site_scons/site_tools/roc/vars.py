import SCons.Script
import SCons.Util
import os

def _get_arg(env, var, default):
    ret = SCons.Script.ARGUMENTS.get(var, None)
    if ret is None:
        if var in os.environ:
            ret = os.environ[var]
    if ret is None:
        ret = default
    return ret

def HasArg(env, var):
    return _get_arg(env, var, None) is not None

def OverrideFromArg(env, var, names=[], default=None):
    if not names:
        names = [var]
    for name in names:
        v = _get_arg(env, var, default)
        if v is not None:
            env[var] = v
            break

def PrependFromArg(env, var, names=[], default=None):
    if not names:
        names = [var]
    for name in names:
        v = _get_arg(env, name, default)
        if v is not None:
            env.Prepend(**{var: v})
            break

def MergeVars(env, src_env):
    for k, v in src_env.Dictionary().items():
        if not k in env.Dictionary():
            env[k] = v
            continue

        if isinstance(v, SCons.Util.CLVar) or isinstance(v, list):
            if k == 'LIBS':
                env.AppendUnique(**{k: v})
            else:
                env.PrependUnique(**{k: v})
            continue

def init(env):
    env.AddMethod(HasArg, 'HasArg')
    env.AddMethod(OverrideFromArg, 'OverrideFromArg')
    env.AddMethod(PrependFromArg, 'PrependFromArg')
    env.AddMethod(MergeVars, 'MergeVars')
