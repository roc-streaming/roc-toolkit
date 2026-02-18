from __future__ import print_function
import SCons.Script
import SCons.Util
import collections
import copy
import sys

# Formats message, prints to stderr, and exits with error.
def Die(env, fmt, *args):
    env.ColorPrint('red', 'error: {}\n', fmt.format(*args).strip())
    SCons.Script.Exit(1)

# Formats message and prints to stderr.
def Warn(env, fmt, *args):
    env.ColorPrint('yellow', 'warning: {}\n', fmt.format(*args).strip())

# Returns a deep clone of env.
# Workaround for buggy behavior of env.Clone() in recent scons.
def DeepClone(env):
    cloned_env = env.Clone()

    # fixup after env.Clone()
    for key in env.Dictionary():
        if not cloned_env[key] is env[key]:
            # already copied
            continue
        if callable(cloned_env[key]) \
          or isinstance(cloned_env[key], SCons.Util.Unbuffered):
            # not copyable
            continue
        if isinstance(cloned_env[key], (int, float, complex, bool, str, tuple)) \
          or cloned_env[key] is None:
            # immutable
            continue

        try:
            # try to do deep copy
            cloned_value = copy.deepcopy(env[key])
            cloned_env[key] = cloned_value
        except:
            pass

    return cloned_env

# env.MergeFrom(other_env) merges configuration from other_env into env.
def MergeFrom(dst_env, src_env, exclude=[]):
    for key, src_val in src_env.Dictionary().items():
        if key in exclude:
            continue

        if isinstance(src_val, SCons.Util.CLVar) \
          or isinstance(src_val, collections.deque) \
          or isinstance(src_val, list):
            if key in dst_env.Dictionary():
                for item in src_val:
                    if item in dst_env[key]:
                        dst_env[key].remove(item)
            if 'FLAGS' in key:
                dst_env.Append(**{key: src_val})
            else:
                dst_env.AppendUnique(**{key: src_val})
        else:
            if key not in dst_env.Dictionary():
                dst_env[key] = src_val

def init(env):
    env.AddMethod(Die, 'Die')
    env.AddMethod(Warn, 'Warn')
    env.AddMethod(DeepClone, 'DeepClone')
    env.AddMethod(MergeFrom, 'MergeFrom')
