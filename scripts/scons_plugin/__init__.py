import scons_plugin.arguments
import scons_plugin.commands
import scons_plugin.config
import scons_plugin.distfiles
import scons_plugin.filesystem
import scons_plugin.parallel
import scons_plugin.parsers
import scons_plugin.pkgconfig
import scons_plugin.pretty
import scons_plugin.tests
import scons_plugin.thirdparty
import scons_plugin.variables

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
        scons_plugin.filesystem,
        scons_plugin.parallel,
        scons_plugin.parsers,
        scons_plugin.pkgconfig,
        scons_plugin.pretty,
        scons_plugin.tests,
        scons_plugin.thirdparty,
        scons_plugin.variables,
    ]
    for m in modules:
        m.init(env)

def exists(env):
    return 1
