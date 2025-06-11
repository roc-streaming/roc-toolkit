/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>
#include "roc_rtp/parser.h"
#include "roc_rtp/composer.h"

namespace roc {
namespace rtp {

// The following tests check that Parser and Composer return the correct error codes

// This is a dummy and is set to always fail to allocate memory. For simulating an out-of-memory error
// Always returns nullptr
struct FailingAllocator {
    void* allocate(size_t) { return nullptr; }
    void  deallocate(void*)  {}
};


TEST_GROUP(status_propagation) {};

// Test case: ParserNoMem
//When the parser can't get memory, it should return StatusNoMem.
TEST(status_propagation, parser_no_mem)
{
    //creates a Parser that uses the failing allocator
    Parser parser(new FailingAllocator());

    //try to provide/parse a minimal RTP header (4 bytes)
    uint8_t header[4] = { 0x80, 0x00, 0x00, 0x00 };
    StatusCode code = parser.parse(header, 4);

    //the parser should return no-memory error
    CHECK_EQUAL(StatusNoMem, code);
}

// Test case: ParserBadHeader
//When invalid data is fed to parser, it should return StatusBadHeader.
TEST(status_propagation, parser_bad_header)
{
    Parser parser;

    //invalide header is provided (4 zero bytes)
    uint8_t badData[4] = { 0, 0, 0, 0 };
    StatusCode code = parser.parse(badData, 4);

    //should return a bad-header error
    CHECK_EQUAL(StatusBadHeader, code);
}

// Test case: ParserError
// When no data is fed to parser, it should return StatusError.
TEST(status_propagation, parser_error)
{
    Parser parser;

    //Try to parse with no buffer (nullptr, size 0)
    StatusCode code = parser.parse(nullptr, 0);

    //Shoudl return StatusError 
    CHECK_EQUAL(StatusError, code);
}

// Test case: ComposerError
// When the composer is fed bad pointers, it should StatusError.
TEST(status_propagation, composer_error)
{
    Composer composer;

    //call compose with null output pointers
    uint8_t* outPtr = nullptr;
    size_t   outSz  = 0;
    StatusCode code = composer.compose(nullptr, 0, &outPtr, &outSz);

    //composer should report StatusError
    CHECK_EQUAL(StatusError, code);
}

// Test case: ComposerNoMem
// If composer cannot allocate memory, it should return StatusNoMem.
TEST(status_propagation, composer_no_mem)
{
    //create a composer that uses the dummy failing allocator
    Composer composer(new FailingAllocator());

    // attempts to compose (allocator always fails)
    uint8_t* outPtr = nullptr;
    size_t   outSz  = 0;
    StatusCode code = composer.compose(nullptr, 0, &outPtr, &outSz);

    // composer should return StatusNoMem
    CHECK_EQUAL(StatusNoMem, code);
}

}  // namespace rtp
}  // namespace roc
