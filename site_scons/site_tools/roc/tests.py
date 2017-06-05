import SCons.Script
import os
import re

def _IsTestEnabled(testname):
    for target in ['test', testname]:
        if target in SCons.Script.COMMAND_LINE_TARGETS:
            return True

def _GetNonTestTargets(env):
    if SCons.Script.COMMAND_LINE_TARGETS:
        for target in SCons.Script.COMMAND_LINE_TARGETS:
            if target == 'test':
                yield env.Dir('#')
            elif not re.match('^test/.+', target):
                yield target
    else:
        yield env.Dir('#')

def _WithTimeout(env, cmd, timeout):
    return '%s %s/wrappers/timeout.py %s %s' % (
        env.Python(),
        env.Dir(os.path.dirname(__file__)).path,
        timeout,
        cmd)

def AddTest(env, name, exe, cmd=None, timeout=5*60):
    testname = 'test/%s' % name

    if not _IsTestEnabled(testname):
        return

    if not cmd:
        cmd = env.File(exe).path

    cmd = _WithTimeout(env, cmd, timeout)

    comstr = env.Pretty('TEST', name, 'green')
    target = env.Alias(testname, [], env.Action(cmd, comstr))

    # This target produces no files.
    env.AlwaysBuild(target)

    # This target depends on test executable that it should run.
    env.Depends(target, env.File(exe))

    # This target should be run after all build targets.
    for t in _GetNonTestTargets(env):
        env.Requires(target, t)

    # This target should be run after all previous tests.
    for t in env['_ROC_TESTS']:
        env.Requires(target, t)

    # Add target to test list.
    env['_ROC_TESTS'] += [testname]

    # 'test' target depends on this target.
    env.Depends('test', target)

def Init(env):
    env['_ROC_TESTS'] = []

    env.AlwaysBuild(env.Alias('test', [], env.Action('')))
    env.AddMethod(AddTest, 'AddTest')
