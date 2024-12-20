/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_harness.h"

ExtTestOutput::ExtTestOutput()
    : ConsoleTestOutput() {
}

void ExtTestOutput::printCurrentTestStarted(const UtestShell& test) {
    ExtTestRunner::getCurrent()->markTestStarted();

    ConsoleTestOutput::printCurrentTestStarted(test);
}

void ExtTestOutput::printCurrentTestEnded(const TestResult& res) {
    if (ExtTestRunner::getCurrent()->isTestSkipped()) {
        setProgressIndicator("s");
    }

    ConsoleTestOutput::printCurrentTestEnded(res);

    ExtTestRunner::getCurrent()->markTestEnded();
}

ExtTestRunner* ExtTestRunner::current_runner_ = NULL;

ExtTestRunner* ExtTestRunner::getCurrent() {
    return current_runner_;
}

ExtTestRunner::ExtTestRunner(int argc, const char* const* argv)
    : CommandLineTestRunner(argc, argv, TestRegistry::getCurrentRegistry())
    , test_skipped_(false) {
    current_runner_ = this;
}

void ExtTestRunner::markTestStarted() {
    test_skipped_ = false;
}

void ExtTestRunner::markTestSkipped() {
    test_skipped_ = true;
}

void ExtTestRunner::markTestEnded() {
    test_skipped_ = false;
}

bool ExtTestRunner::isTestSkipped() const {
    return test_skipped_;
}

TestOutput* ExtTestRunner::createConsoleOutput() {
    return new ExtTestOutput;
}
