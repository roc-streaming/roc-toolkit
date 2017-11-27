import roc.pretty
import roc.helpers
import roc.tests
import roc.parallel

# workaround for python3
import SCons.Subst
try:
    SCons.Subst.Literal.__hash__ = lambda self: hash(str(self))
except:
    pass

def generate(env):
    pretty.Init(env)
    helpers.Init(env)
    tests.Init(env)
    parallel.Init(env)

def exists(env):
    return 1
