/*
 * Copyright (c) 2021 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_NETIO_TEST_HELPERS_CONN_EXPECTATION_H_
#define ROC_NETIO_TEST_HELPERS_CONN_EXPECTATION_H_

namespace roc {
namespace netio {
namespace test {

enum ConnExpectation { ExpectNotFailed, ExpectFailed };

} // namespace test
} // namespace netio
} // namespace roc

#endif // ROC_NETIO_TEST_HELPERS_CONN_EXPECTATION_H_
