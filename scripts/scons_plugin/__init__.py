import scons_plugin.arguments
import scons_plugin.commands
import scons_plugin.config
import scons_plugin.distfiles
import scons_plugin.osutils
import scons_plugin.parallel
import scons_plugin.parsers
import scons_plugin.pretty
import scons_plugin.sconsutils
import scons_plugin.systemdeps
import scons_plugin.tests
import scons_plugin.thirdparty
import scons_plugin.wrapper

# workaround for python3
import SCons.Subst
try:
    SCons.Subst.Literal.__hash__ = lambda self: hash(str(self))
except:
    pass

def generate(env):
    modules = [
        scons_plugin.arguments,
        scons_plugin.commands,
        scons_plugin.config,
        scons_plugin.distfiles,
        scons_plugin.osutils,
        scons_plugin.parallel,
        scons_plugin.parsers,
        scons_plugin.pretty,
        scons_plugin.sconsutils,
        scons_plugin.systemdeps,
        scons_plugin.tests,
        scons_plugin.thirdparty,
        scons_plugin.wrapper,
    ]
    for m in modules:
        m.init(env)

def exists(env):
    return 1
