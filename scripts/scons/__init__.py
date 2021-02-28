import scons.arguments
import scons.commands
import scons.config
import scons.distfiles
import scons.filesystem
import scons.parallel
import scons.parsers
import scons.pkgconfig
import scons.pretty
import scons.tests
import scons.thirdparty
import scons.variables

# workaround for python3
import SCons.Subst
try:
    SCons.Subst.Literal.__hash__ = lambda self: hash(str(self))
except:
    pass

def generate(env):
    modules = [
        scons.arguments,
        scons.commands,
        scons.config,
        scons.distfiles,
        scons.filesystem,
        scons.parallel,
        scons.parsers,
        scons.pkgconfig,
        scons.pretty,
        scons.tests,
        scons.thirdparty,
        scons.variables,
    ]
    for m in modules:
        m.init(env)

def exists(env):
    return 1
