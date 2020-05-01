import SCons.Script
import os
import re

def _is_test_enabled(kind, testname):
    for target in [kind, testname]:
        if target in SCons.Script.COMMAND_LINE_TARGETS:
            return True

def _get_non_test_targets(env):
    if SCons.Script.COMMAND_LINE_TARGETS:
        for target in SCons.Script.COMMAND_LINE_TARGETS:
            if target == 'test' or target == 'bench':
                yield env.Dir('#')
            elif not re.match('^(test|bench)/.+', target):
                yield target
    else:
        yield env.Dir('#')

def _with_timeout(env, cmd, timeout):
    return '%s scripts/wrappers/timeout.py %s %s' % (
        env.PythonExecutable(),
        timeout,
        cmd)

def _add_test(env, kind, name, exe, cmd, timeout):
    varname = '_ROC_%s_TARGETS' % kind.upper()
    testname = '%s/%s' % (kind, name)

    if not _is_test_enabled(kind, testname):
        return

    if not cmd:
        cmd = env.File(exe).path

    if timeout is not None:
        cmd = _with_timeout(env, cmd, timeout)

    comstr = env.PrettyCommand(kind.upper(), name, 'green' if kind == 'test' else 'cyan')
    target = env.Alias(testname, [], env.Action(cmd, comstr))

    # This target produces no files.
    env.AlwaysBuild(target)

    # This target depends on test executable that it should run.
    env.Depends(target, env.File(exe))

    # This target should be run after all build targets.
    for t in _get_non_test_targets(env):
        env.Requires(target, t)

    # This target should be run after all previous tests.
    for t in env[varname]:
        env.Requires(target, t)

    # Benchmarks should be run after all tests.
    if kind == 'bench':
        for t in env['_ROC_TEST_TARGETS']:
            env.Requires(target, t)
    else:
        for t in env['_ROC_BENCH_TARGETS']:
            env.Requires(t, target)

    # Add target to test list.
    env[varname] += [testname]

    # 'test' target depends on this target.
    env.Depends(kind, target)

def AddTest(env, name, exe, cmd=None, timeout=5*60):
    _add_test(env, 'test', name, exe, cmd, timeout)

def AddBench(env, name, exe, cmd=None, timeout=None):
    _add_test(env, 'bench', name, exe, cmd, timeout)

def init(env):
    env['_ROC_TEST_TARGETS'] = []
    env['_ROC_BENCH_TARGETS'] = []

    env.AlwaysBuild(env.Alias('test', [], env.Action('')))
    env.AlwaysBuild(env.Alias('bench', [], env.Action('')))

    env.Requires('bench', 'test')

    env.AddMethod(AddTest, 'AddTest')
    env.AddMethod(AddBench, 'AddBench')
