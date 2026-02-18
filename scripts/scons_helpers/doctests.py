import doctest
import importlib.util
import os
import os.path
import sys
import time
import unittest

class CompactTextTestResult(unittest.TextTestResult):

    def __init__(self, stream, descriptions, verbosity):
        super().__init__(stream, descriptions, verbosity)

        if os.name == 'posix' and sys.stdout.isatty():
            self.RED = '\033[0;31m'
            self.GREEN = '\033[0;32m'
            self.YELLOW = '\033[0;33m'
            self.BLUE = '\033[0;34m'
            self.RESET = '\033[0m'
        else:
            self.RED = ''
            self.GREEN = ''
            self.YELLOW = ''
            self.BLUE = ''
            self.RESET = ''

    def getDescription(self, test):
        return test.shortDescription().replace('Doctest: ', '')

    def addSuccess(self, test):
        self.stream.write(self.GREEN)
        super().addSuccess(test)
        self.stream.write(self.RESET)

    def addError(self, test, err):
        self.stream.write(self.RED)
        super().addError(test, err)
        self.stream.write(self.RESET)

    def addFailure(self, test, err):
        self.stream.write(self.YELLOW)
        super().addFailure(test, err)
        self.stream.write(self.RESET)

    def printErrors(self):
        self.printErrorList(f"{self.RED}ERROR{self.RESET}", self.errors)
        self.printErrorList(f"{self.RED}FAIL{self.RESET}", self.failures)

class CompactTextTestRunner(unittest.TextTestRunner):
    resultclass = CompactTextTestResult

    def __init__(self):
        super().__init__(verbosity=2)

    def run(self, test):
        result = CompactTextTestResult(self.stream, self.descriptions, self.verbosity)
        result.failfast = self.failfast
        result.buffer = self.buffer
        result.tb_locals = self.tb_locals
        start_time = time.perf_counter()
        result.startTestRun()
        try:
            test(result)
        finally:
            result.stopTestRun()
            stop_time = time.perf_counter()
        time_taken = (stop_time - start_time) * 1000
        result.printErrors()

        total = result.testsRun
        errors = len(result.failures) + len(result.errors)
        if errors == 0:
            status = 'OK'
        else:
            status = 'Errors'

        self.stream.writeln("{} ({} failures, {} ran, {} ms)".format(
            status, errors, total, int(time_taken)))
        self.stream.writeln()
        self.stream.flush()

        return result

cwd = os.path.abspath(os.getcwd())

def load_tests(loader, tests, ignore):
    test_paths = []
    for arg in sys.argv[1:]:
        arg = os.path.join(cwd, arg)
        if os.path.isdir(arg):
            for root, dirs, files in os.walk(arg):
                for name in files:
                    if name.endswith('.py'):
                        test_paths.append(os.path.join(root, name))
        else:
            test_paths.append(arg)

    for path in test_paths:
        package = os.path.basename(os.path.dirname(path))
        name =  package + '.' + os.path.splitext(os.path.basename(path))[0]
        spec = importlib.util.spec_from_file_location(name, path)
        module = importlib.util.module_from_spec(spec)

        sys.modules[name] = module
        spec.loader.exec_module(module)
        tests.addTests(doctest.DocTestSuite(module))

    return tests

if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromModule(__import__(__name__))
    CompactTextTestRunner().run(suite)
