import roc.commands
import roc.config
import roc.distfiles
import roc.fs
import roc.parallel
import roc.parsers
import roc.pretty
import roc.tests
import roc.thirdparty
import roc.vars

# workaround for python3
import SCons.Subst
try:
    SCons.Subst.Literal.__hash__ = lambda self: hash(str(self))
except:
    pass

def generate(env):
    modules = [
        commands,
        config,
        distfiles,
        fs,
        parallel,
        parsers,
        pretty,
        tests,
        thirdparty,
        vars,
    ]
    for m in modules:
        m.init(env)

def exists(env):
    return 1
