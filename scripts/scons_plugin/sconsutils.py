from __future__ import print_function
import SCons.Script
import SCons.Util
import sys

# Formats message, prints to stderr, and exits with error.
def Die(env, fmt, *args):
    print('error: ' + (fmt % args).strip() + '\n', file=sys.stderr)
    SCons.Script.Exit(1)

# env.MergeFrom(other_env) merges configuration from other_env into env
def MergeFrom(dst_env, src_env, exclude=[]):
    for key, src_val in src_env.Dictionary().items():
        if key in exclude:
            continue

        if isinstance(src_val, SCons.Util.CLVar) or isinstance(src_val, list):
            if key in dst_env.Dictionary():
                for item in src_val:
                    if item in dst_env[key]:
                        dst_env[key].remove(item)
            dst_env.AppendUnique(**{key: src_val})
        else:
            if key not in dst_env.Dictionary():
                dst_env[key] = src_val

def init(env):
    env.AddMethod(Die, 'Die')
    env.AddMethod(MergeFrom, 'MergeFrom')
