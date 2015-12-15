import SCons.Script

import sys
import re

COLORS = {}
COMPACT = False

if sys.stdout.isatty():
    COLORS['cyan']   = '\033[0;36m'
    COLORS['purple'] = '\033[0;35m'
    COLORS['blue']   = '\033[0;34m'
    COLORS['green']  = '\033[0;32m'
    COLORS['yellow'] = '\033[0;33m'
    COLORS['red']    = '\033[0;31m'
    COLORS['end']    = '\033[0m'
else:
    for k in ['cyan', 'purple', 'blue', 'green', 'yellow', 'red', 'end']:
        COLORS[k] = ''

def _Subst(env, string, raw=1, target=None, source=None, conv=None, executor=None):
    raw = 1
    gvars = env.gvars()
    lvars = env.lvars()
    lvars['__env__'] = env
    if executor:
        lvars.update(executor.get_lvars())
    return SCons.Subst.scons_subst(string, env, raw, target, source, gvars, lvars, conv)

def Pretty(env, command, args, color):
    global COMPACT
    if COMPACT:
        return ' %s%8s%s   %s' % (COLORS[color], command, COLORS['end'], args)
    else:
        return '$CMDLINE'

def Init(env):
    global COMPACT

    for arg in sys.argv:
        if re.match('^-([^-].*)?Q.*$', arg):
            COMPACT = True

    env.AddMethod(Pretty, 'Pretty')

    if COMPACT:
        env.AddMethod(_Subst, 'subst_target_source')

        env['CCCOMSTR']       = env.Pretty('CC', '$SOURCE', 'blue')
        env['SHCCCOMSTR']     = env.Pretty('CC', '$SOURCE', 'blue')

        env['CXXCOMSTR']      = env.Pretty('CXX', '$SOURCE', 'blue')
        env['SHCXXCOMSTR']    = env.Pretty('CXX', '$SOURCE', 'blue')

        env['RCCOMSTR']       = env.Pretty('RC', '$SOURCE', 'red')
        env['ARCOMSTR']       = env.Pretty('AR', '$TARGET', 'red')
        env['SHLINKCOMSTR']   = env.Pretty('LD', '$TARGET', 'red')
        env['LDMODULECOMSTR'] = env.Pretty('LD', '$TARGET', 'red')
        env['LINKCOMSTR']     = env.Pretty('LD', '$TARGET', 'red')
        env['RANLIBCOMSTR']   = env.Pretty('RANLIB', '$TARGET', 'red')
        env['MTCOMSTR']       = env.Pretty('MT', '$TARGET', 'green')

        env['INSTALLSTR']     = env.Pretty('INSTALL', '$TARGET', 'yellow')
