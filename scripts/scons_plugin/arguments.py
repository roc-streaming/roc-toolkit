import SCons.Script
import os

def _find_argument(env, arg):
    ret = SCons.Script.ARGUMENTS.get(arg, None)
    if ret is None:
        if arg in os.environ:
            ret = os.environ[arg]
    return ret

def _mark_known_argument(env, arg):
    if '_KNOWN_ARGS' not in env.Dictionary():
        env['_KNOWN_ARGS'] = set()
    env['_KNOWN_ARGS'].add(arg)

def _is_known_argument(env, arg):
    if '_KNOWN_ARGS' in env.Dictionary():
        return arg in env['_KNOWN_ARGS']
    return False

def HasArgument(env, arg):
    return _is_known_argument(env, arg) or _find_argument(env, arg) is not None

def OverrideFromArgument(env, arg, names=[], default=None):
    if not names:
        names = [arg]
    for name in names:
        v = _find_argument(env, name)
        if v is not None:
            _mark_known_argument(env, arg)
        else:
            v = default
        if v is not None:
            env[arg] = v
            break

def PrependFromArgument(env, arg, names=[], default=None):
    if not names:
        names = [arg]
    for name in names:
        v = _find_argument(env, name)
        if v is not None:
            _mark_known_argument(env, arg)
        else:
            v = default
        if v is not None:
            env.Prepend(**{arg: v})
            break

def init(env):
    env.AddMethod(HasArgument, 'HasArgument')
    env.AddMethod(OverrideFromArgument, 'OverrideFromArgument')
    env.AddMethod(PrependFromArgument, 'PrependFromArgument')
