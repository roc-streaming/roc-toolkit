/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_TEST_HARNESS_H_
#define ROC_TEST_HARNESS_H_

#include <CppUTest/CommandLineArguments.h>
#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness.h>
#include <CppUTest/TestRegistry.h>

#define TEST_SKIP()                                                                      \
    do {                                                                                 \
        ExtTestRunner::getCurrent()->markTestSkipped();                                  \
        TEST_EXIT;                                                                       \
    } while (0)

class ExtTestOutput : public ConsoleTestOutput {
public:
    ExtTestOutput();

    virtual void printCurrentTestStarted(const UtestShell& test);
    virtual void printCurrentTestEnded(const TestResult& res);
};

class ExtTestRunner : public CommandLineTestRunner {
public:
    static ExtTestRunner* getCurrent();

    ExtTestRunner(int argc, const char* const* argv);

    void markTestStarted();
    void markTestSkipped();
    void markTestEnded();

    bool isTestSkipped() const;

protected:
    virtual TestOutput* createConsoleOutput();

private:
    static ExtTestRunner* current_runner_;
    bool test_skipped_;
};

#endif // ROC_TEST_HARNESS_H_
