import SCons.Script
import sys
import re

_COLORS = {}
_COMPACT = False

def _init_colors():
    global _COLORS

    if sys.stdout.isatty():
        _COLORS['cyan']   = '\033[0;36m'
        _COLORS['purple'] = '\033[0;35m'
        _COLORS['blue']   = '\033[0;34m'
        _COLORS['green']  = '\033[0;32m'
        _COLORS['yellow'] = '\033[0;33m'
        _COLORS['red']    = '\033[0;31m'
        _COLORS['end']    = '\033[0m'
    else:
        for k in ['cyan', 'purple', 'blue', 'green', 'yellow', 'red', 'end']:
           _COLORS[k] = ''

def _init_compact():
    global _COMPACT

    for arg in sys.argv:
        if re.match('^-([^-].*)?Q.*$', arg):
            _COMPACT = True
            break

def _init_pretty(env):
    global _COMPACT

    if not _COMPACT:
        return

    def _subst(env, string, raw=1, target=None, source=None, conv=None, executor=None):
        raw = 1
        gvars = env.gvars()
        lvars = env.lvars()
        lvars['__env__'] = env
        if executor:
            lvars.update(executor.get_lvars())
        return SCons.Subst.scons_subst(string, env, raw, target, source, gvars, lvars, conv)

    env.AddMethod(_subst, 'subst_target_source')

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
    global _COMPACT

    if _COMPACT:
        return ' {}{:>8s}{}   {}'.format(_COLORS[color], command, _COLORS['end'], args)
    elif cmdline:
        return cmdline
    else:
        return '$CMDLINE'

def init(env):
    env.AddMethod(PrettyCommand, 'PrettyCommand')
    _init_colors()
    _init_compact()
    _init_pretty(env)
