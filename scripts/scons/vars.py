import SCons.Script
import SCons.Util
import os

def _find_overridden(env, var):
    ret = SCons.Script.ARGUMENTS.get(var, None)
    if ret is None:
        if var in os.environ:
            ret = os.environ[var]
    return ret

def _mark_overridden(env, var):
    env.AppendUnique(**{'_OVERRIDDEN_ARGS':var})

def _marked_overridden(env, var):
    if '_OVERRIDDEN_ARGS' in env.Dictionary():
        return var in env['_OVERRIDDEN_ARGS']
    return False

def HasArg(env, var):
    return _marked_overridden(env, var) or _find_overridden(env, var) is not None

def OverrideFromArg(env, var, names=[], default=None):
    if not names:
        names = [var]
    for name in names:
        v = _find_overridden(env, name)
        if v is not None:
            _mark_overridden(env, var)
        else:
            v = default
        if v is not None:
            env[var] = v
            break

def PrependFromArg(env, var, names=[], default=None):
    if not names:
        names = [var]
    for name in names:
        v = _find_overridden(env, name)
        if v is not None:
            _mark_overridden(env, var)
        else:
            v = default
        if v is not None:
            env.Prepend(**{var: v})
            break

def MergeVars(env, src_env, exclude=[]):
    for k, v in src_env.Dictionary().items():
        if k in exclude:
            continue

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
