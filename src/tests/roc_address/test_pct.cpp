/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/pct.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace address {

TEST_GROUP(pct) {
    ssize_t encode(char* dst, size_t dst_sz, const char* src, size_t src_sz, PctMode mode) {
        core::StringBuilder b(dst, dst_sz);

        if (!pct_encode(b, src, src_sz, mode)) {
            return -1;
        }

        if (!b.ok()){
            return -1;
        }

        return (ssize_t)b.actual_size() - 1;
    }

    ssize_t decode(char* dst, size_t dst_sz, const char* src, size_t src_sz) {
        core::StringBuilder b(dst, dst_sz);

        if (!pct_decode(b, src, src_sz)) {
            return -1;
        }

        if (!b.ok()){
            return -1;
        }

        return (ssize_t)b.actual_size() - 1;
    }
};

TEST(pct, unreserved_symbols) {
    const char* decoded =
        // allowed (unreserved)
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~"
        // disallowed
        "!#$%&'()*+,:;=/?@[]"
        // disallowed
        "`^{}<>|\\\" ";

    const char* encoded =
        // allowed (unreserved)
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~"
        // !  #  $  %  &  '  (  )  *  +  ,  :  ;  =  /  ?  @  [  ]
        "%21%23%24%25%26%27%28%29%2A%2B%2C%3A%3B%3D%2F%3F%40%5B%5D"
        // `  ^  {  }  <  >  |  \  " spc
        "%60%5E%7B%7D%3C%3E%7C%5C%22%20";

    {
        char buf[512];
        ssize_t ret =
            encode(buf, sizeof(buf), decoded, strlen(decoded), PctNonUnreserved);
        CHECK(ret > 0);
        STRCMP_EQUAL(encoded, buf);
        LONGS_EQUAL(strlen(encoded), ret);
    }

    {
        char buf[512];
        ssize_t ret = decode(buf, sizeof(buf), encoded, strlen(encoded));
        CHECK(ret > 0);
        STRCMP_EQUAL(decoded, buf);
        LONGS_EQUAL(strlen(decoded), ret);
    }
}

TEST(pct, host_symbols) {
    const char* decoded =
        // allowed (unreserved)
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~"
        // allowed (subdelims)
        "!$&'()*+,;="
        // allowed (ipv6)
        ":[]"
        // disallowed
        "#?/@"
        // disallowed
        "`^{}<>|\\\" ";

    const char* encoded =
        // allowed (unreserved)
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~"
        // allowed (subdelims)
        "!$&'()*+,;="
        // allowed (ipv6)
        ":[]"
        // #  ?  /  @
        "%23%3F%2F%40"
        // `  ^  {  }  <  >  |  \  " spc
        "%60%5E%7B%7D%3C%3E%7C%5C%22%20";

    {
        char buf[512];
        ssize_t ret = encode(buf, sizeof(buf), decoded, strlen(decoded), PctNonHost);
        CHECK(ret > 0);
        STRCMP_EQUAL(encoded, buf);
        LONGS_EQUAL(strlen(encoded), ret);
    }

    {
        char buf[512];
        ssize_t ret = decode(buf, sizeof(buf), encoded, strlen(encoded));
        CHECK(ret > 0);
        STRCMP_EQUAL(decoded, buf);
        LONGS_EQUAL(strlen(decoded), ret);
    }
}

TEST(pct, path_symbols) {
    const char* decoded =
        // allowed (unreserved)
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~"
        // allowed (subdelims)
        "!$&'()*+,;="
        // allowed (pchar, path)
        ":@/"
        // disallowed
        "#?[]"
        // disallowed
        "`^{}<>|\\\" ";

    const char* encoded =
        // allowed (unreserved)
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~"
        // allowed (subdelims)
        "!$&'()*+,;="
        // allowed (pchar, path)
        ":@/"
        // #  ?  [  ]
        "%23%3F%5B%5D"
        // `  ^  {  }  <  >  |  \  " spc
        "%60%5E%7B%7D%3C%3E%7C%5C%22%20";

    {
        char buf[512];
        ssize_t ret = encode(buf, sizeof(buf), decoded, strlen(decoded), PctNonPath);
        CHECK(ret > 0);
        STRCMP_EQUAL(encoded, buf);
        LONGS_EQUAL(strlen(encoded), ret);
    }

    {
        char buf[512];
        ssize_t ret = decode(buf, sizeof(buf), encoded, strlen(encoded));
        CHECK(ret > 0);
        STRCMP_EQUAL(decoded, buf);
        LONGS_EQUAL(strlen(decoded), ret);
    }
}

TEST(pct, unicode_symbols) {
    const char* decoded = "♥";
    const char* encoded = "%E2%99%A5";

    {
        char buf[512];
        ssize_t ret =
            encode(buf, sizeof(buf), decoded, strlen(decoded), PctNonUnreserved);
        CHECK(ret > 0);
        STRCMP_EQUAL(encoded, buf);
        LONGS_EQUAL(strlen(encoded), ret);
    }

    {
        char buf[512];
        ssize_t ret = decode(buf, sizeof(buf), encoded, strlen(encoded));
        CHECK(ret > 0);
        STRCMP_EQUAL(decoded, buf);
        LONGS_EQUAL(strlen(decoded), ret);
    }
}

TEST(pct, case_sensitivity) {
    const char* encoded_lower = "%3f";
    const char* encoded_upper = "%3F";

    const char* decoded = "?";

    {
        char buf[512];
        ssize_t ret = decode(buf, sizeof(buf), encoded_lower, strlen(encoded_lower));
        CHECK(ret > 0);
        STRCMP_EQUAL(decoded, buf);
        LONGS_EQUAL(strlen(decoded), ret);
    }

    {
        char buf[512];
        ssize_t ret = decode(buf, sizeof(buf), encoded_upper, strlen(encoded_upper));
        CHECK(ret > 0);
        STRCMP_EQUAL(decoded, buf);
        LONGS_EQUAL(strlen(decoded), ret);
    }

    {
        char buf[512];
        ssize_t ret =
            encode(buf, sizeof(buf), decoded, strlen(decoded), PctNonUnreserved);
        CHECK(ret > 0);
        STRCMP_EQUAL(encoded_upper, buf);
        LONGS_EQUAL(strlen(encoded_upper), ret);
    }
}

TEST(pct, small_buffer) {
    const char* str = "12345";

    char buf[5];

    LONGS_EQUAL(-1, encode(buf, sizeof(buf), str, strlen(str), PctNonUnreserved));
    LONGS_EQUAL(-1, decode(buf, sizeof(buf), str, strlen(str)));
}

TEST(pct, invalid_input) {
    char buf[512];
    const char* str;

    str = "%2A";
    LONGS_EQUAL(1, decode(buf, sizeof(buf), str, strlen(str)));

    str = "%";
    LONGS_EQUAL(-1, decode(buf, sizeof(buf), str, strlen(str)));

    str = "%??";
    LONGS_EQUAL(-1, decode(buf, sizeof(buf), str, strlen(str)));

    str = "%00";
    LONGS_EQUAL(-1, decode(buf, sizeof(buf), str, strlen(str)));

    str = "a\0b";
    LONGS_EQUAL(-1, decode(buf, sizeof(buf), str, 3));
    LONGS_EQUAL(-1, encode(buf, sizeof(buf), str, 3, PctNonUnreserved));
}

} // namespace address
} // namespace roc
