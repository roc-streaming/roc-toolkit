import roc.pretty
import roc.helpers
import roc.tests
import roc.parallel

def generate(env):
    pretty.Init(env)
    helpers.Init(env)
    tests.Init(env)
    parallel.Init(env)

def exists(env):
    return 1
