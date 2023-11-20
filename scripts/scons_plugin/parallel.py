import SCons.Script
import sys
import re

def _cpu_count():
    try:
        import os
        return len(os.sched_getaffinity(0))
    except:
        pass

    try:
        import multiprocessing
        return multiprocessing.cpu_count()
    except:
        pass

    try:
        import os
        res = int(os.sysconf('SC_NPROCESSORS_ONLN'))
        if res > 0:
            return res
    except:
        pass

    try:
        import os
        res = int(os.environ['NUMBER_OF_PROCESSORS'])
        if res > 0:
            return res
    except:
        pass

    return 1

def init(env):
    for arg in sys.argv:
        if re.match(r'^(-j|--jobs=?)\d*$', arg):
            return
    SCons.Script.SetOption('num_jobs', _cpu_count())
