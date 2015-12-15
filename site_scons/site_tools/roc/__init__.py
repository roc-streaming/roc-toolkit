import roc.colors
import roc.helpers
import roc.tests
import roc.parallel

def generate(env):
    colors.Init(env)
    helpers.Init(env)
    tests.Init(env)
    parallel.Init(env)

def exists(env):
    return 1
