import SCons.Script
import os
import re

def _is_test_enabled(testname):
    for target in ['test', testname]:
        if target in SCons.Script.COMMAND_LINE_TARGETS:
            return True

def _get_non_test_targets(env):
    if SCons.Script.COMMAND_LINE_TARGETS:
        for target in SCons.Script.COMMAND_LINE_TARGETS:
            if target == 'test':
                yield env.Dir('#')
            elif not re.match('^test/.+', target):
                yield target
    else:
        yield env.Dir('#')

def _with_timeout(env, cmd, timeout):
    return '%s scripts/wrappers/timeout.py %s %s' % (
        env.PythonExecutable(),
        timeout,
        cmd)

def AddTest(env, name, exe, cmd=None, timeout=5*60):
    testname = 'test/%s' % name

    if not _is_test_enabled(testname):
        return

    if not cmd:
        cmd = env.File(exe).path

    cmd = _with_timeout(env, cmd, timeout)

    comstr = env.PrettyCommand('TEST', name, 'green')
    target = env.Alias(testname, [], env.Action(cmd, comstr))

    # This target produces no files.
    env.AlwaysBuild(target)

    # This target depends on test executable that it should run.
    env.Depends(target, env.File(exe))

    # This target should be run after all build targets.
    for t in _get_non_test_targets(env):
        env.Requires(target, t)

    # This target should be run after all previous tests.
    for t in env['_ROC_TESTS']:
        env.Requires(target, t)

    # Add target to test list.
    env['_ROC_TESTS'] += [testname]

    # 'test' target depends on this target.
    env.Depends('test', target)

def init(env):
    env['_ROC_TESTS'] = []
    env.AlwaysBuild(env.Alias('test', [], env.Action('')))
    env.AddMethod(AddTest, 'AddTest')
