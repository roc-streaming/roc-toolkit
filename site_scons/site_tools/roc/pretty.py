from __future__ import print_function
import SCons.Script
import sys
import re

_Colors = {}
_Compact = False

def _init_colors():
    global _Colors

    if sys.stdout.isatty():
        _Colors['cyan']   = '\033[0;36m'
        _Colors['purple'] = '\033[0;35m'
        _Colors['blue']   = '\033[0;34m'
        _Colors['green']  = '\033[0;32m'
        _Colors['yellow'] = '\033[0;33m'
        _Colors['red']    = '\033[0;31m'
        _Colors['end']    = '\033[0m'
    else:
        for k in ['cyan', 'purple', 'blue', 'green', 'yellow', 'red', 'end']:
           _Colors[k] = ''

def _init_compact():
    global _Compact

    for arg in sys.argv:
        if re.match('^-([^-].*)?Q.*$', arg):
            _Compact = True
            break

def _init_pretty(env):
    global _Compact

    if not _Compact:
        return

    def subst(env, string, raw=1, target=None, source=None, conv=None, executor=None):
        raw = 1
        gvars = env.gvars()
        lvars = env.lvars()
        lvars['__env__'] = env
        if executor:
            lvars.update(executor.get_lvars())
        return SCons.Subst.scons_subst(string, env, raw, target, source, gvars, lvars, conv)

    env.AddMethod(subst, 'subst_target_source')

    env['CCCOMSTR']       = env.PrettyCommand('CC', '$SOURCE', 'blue')
    env['SHCCCOMSTR']     = env.PrettyCommand('CC', '$SOURCE', 'blue')

    env['CXXCOMSTR']      = env.PrettyCommand('CXX', '$SOURCE', 'blue')
    env['SHCXXCOMSTR']    = env.PrettyCommand('CXX', '$SOURCE', 'blue')

    env['RCCOMSTR']       = env.PrettyCommand('RC', '$SOURCE', 'red')
    env['ARCOMSTR']       = env.PrettyCommand('AR', '$TARGET', 'red')
    env['SHLINKCOMSTR']   = env.PrettyCommand('LD', '$TARGET', 'red')
    env['LDMODULECOMSTR'] = env.PrettyCommand('LD', '$TARGET', 'red')
    env['LINKCOMSTR']     = env.PrettyCommand('LD', '$TARGET', 'red')
    env['RANLIBCOMSTR']   = env.PrettyCommand('RANLIB', '$TARGET', 'red')
    env['MTCOMSTR']       = env.PrettyCommand('MT', '$TARGET', 'green')

    env['INSTALLSTR']     = env.PrettyCommand('INSTALL', '$TARGET', 'yellow')

def PrettyCommand(env, command, args, color, cmdline=None):
    global _Compact

    if _Compact:
        return ' %s%8s%s   %s' % (_Colors[color], command, _Colors['end'], args)
    elif cmdline:
        return cmdline
    else:
        return '$CMDLINE'

def Die(env, fmt, *args):
    print('error: ' + (fmt % args).strip() + '\n', file=sys.stderr)
    SCons.Script.Exit(1)

def init(env):
    env.AddMethod(PrettyCommand, 'PrettyCommand')
    env.AddMethod(Die, 'Die')
    _init_colors()
    _init_compact()
    _init_pretty(env)
