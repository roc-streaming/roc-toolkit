from __future__ import print_function
import SCons.Script
import SCons.Util

import sys

def Die(env, fmt, *args):
    print('error: ' + (fmt % args).strip() + '\n', file=sys.stderr)
    SCons.Script.Exit(1)

def MergeEnvironment(env, src_env, exclude=[]):
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
    env.AddMethod(Die, 'Die')
    env.AddMethod(MergeEnvironment, 'MergeEnvironment')
