/*
    This file is part of Corrade.

    Copyright © 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016,
                2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "Corrade/Interconnect/Test/EmitterLibrary.h"
#include "Corrade/TestSuite/Tester.h"

namespace Corrade { namespace Interconnect { namespace Test { namespace {

struct LibraryTest: TestSuite::Tester {
    explicit LibraryTest();

    void test();
};

LibraryTest::LibraryTest() {
    addTests({&LibraryTest::test});
}

void LibraryTest::test() {
    EmitterLibrary e;

    int fired = 1;
    connect(e, &EmitterLibrary::fireInline, [&fired]() { fired *= 2; });
    connect(e, &EmitterLibrary::fireNonInline, [&fired]() { fired *= 3; });

    e.fireNonInline();
    CORRADE_COMPARE(fired, 3);

    e.fireInline();
    CORRADE_COMPARE(fired, 6);

    e.fireNonInlineThroughAFunction();
    CORRADE_COMPARE(fired, 18);

    {
        #ifdef __MINGW32__
        CORRADE_EXPECT_FAIL("Inline member functions are duplicated inside and outside of the DLL when under MinGW.");
        #endif
        e.fireInlineThroughAFunction();
        CORRADE_COMPARE(fired, 36);
    }
}

}}}}

CORRADE_TEST_MAIN(Corrade::Interconnect::Test::LibraryTest)
