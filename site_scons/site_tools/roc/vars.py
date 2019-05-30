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

def OverrideFromArg(env, var, default=None):
    v = _get_arg(env, var, default)
    if v is not None:
        env[var] = v

def PrependFromArg(env, var, args=[], default=None):
    if not args:
        args = [var]
    for arg in args:
        v = _get_arg(env, arg, default)
        if v is not None:
            env.Prepend(**{tvar: env.GetArg(var)})
            break

def AppendVars(env, src_env):
    for k, v in src_env.Dictionary().items():
        if not (isinstance(v, SCons.Util.CLVar) or isinstance(v, list)):
            continue
        env.AppendUnique(**{k: v})

def init(env):
    env.AddMethod(HasArg, 'HasArg')
    env.AddMethod(OverrideFromArg, 'OverrideFromArg')
    env.AddMethod(PrependFromArg, 'PrependFromArg')
    env.AddMethod(AppendVars, 'AppendVars')
