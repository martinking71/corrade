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

#include "Corrade/Containers/String.h"
#include "Corrade/TestSuite/Tester.h"
#include "Corrade/TestSuite/Compare/File.h"
#include "Corrade/Utility/Format.h"
#include "Corrade/Utility/Path.h"

/* The __EMSCRIPTEN_major__ etc macros used to be passed implicitly, version
   3.1.4 moved them to a version header and version 3.1.23 dropped the
   backwards compatibility. To work consistently on all versions, including the
   header only if the version macros aren't present.
   https://github.com/emscripten-core/emscripten/commit/f99af02045357d3d8b12e63793cef36dfde4530a
   https://github.com/emscripten-core/emscripten/commit/f76ddc702e4956aeedb658c49790cc352f892e4c */
#if defined(CORRADE_TARGET_EMSCRIPTEN) && !defined(__EMSCRIPTEN_major__)
#include <emscripten/version.h>
#endif

#include "configure.h"

namespace Corrade { namespace TestSuite { namespace Compare { namespace Test { namespace {

struct FileTest: Tester {
    explicit FileTest();

    void same();
    void empty();
    void utf8Filename();

    void actualNotFound();
    void expectedNotFound();

    void differentContents();
    void actualSmaller();
    void expectedSmaller();
};

FileTest::FileTest() {
    addTests({&FileTest::same,
              &FileTest::empty,
              &FileTest::utf8Filename,

              &FileTest::actualNotFound,
              &FileTest::expectedNotFound,

              &FileTest::differentContents,
              &FileTest::actualSmaller,
              &FileTest::expectedSmaller});
}

void FileTest::same() {
    CORRADE_COMPARE_WITH("base.txt", "base.txt", Compare::File{FILETEST_DIR});

    /* Should not return Diagnostic as everything is okay */
    CORRADE_COMPARE(Comparator<Compare::File>{FILETEST_DIR}("base.txt", "base.txt"), ComparisonStatusFlags{});
}

void FileTest::empty() {
    CORRADE_COMPARE_WITH("empty.txt", "empty.txt", Compare::File{FILETEST_DIR});
}

void FileTest::utf8Filename() {
    #if defined(CORRADE_TARGET_EMSCRIPTEN) && __EMSCRIPTEN_major__*10000 + __EMSCRIPTEN_minor__*100 + __EMSCRIPTEN_tiny__ >= 30103
    /* Emscripten 3.1.3 changed the way files are bundled, putting them
       directly to WASM instead of Base64'd to the JS file. However, it broke
       UTF-8 handling, causing both a compile error (due to a syntax error in
       the assembly file) and if that's patched, also runtime errors later.
        https://github.com/emscripten-core/emscripten/pull/16050 */
    /** @todo re-enable once a fix is made */
    CORRADE_SKIP("Emscripten 3.1.3+ has broken UTF-8 handling in bundled files.");
    #endif

    CORRADE_COMPARE_WITH("hýždě.txt", "base.txt", Compare::File{FILETEST_DIR});
    CORRADE_COMPARE_WITH("base.txt", "hýždě.txt", Compare::File{FILETEST_DIR});
}

void FileTest::actualNotFound() {
    Containers::String out;

    {
        Debug redirectOutput{&out};
        Comparator<Compare::File> compare;
        ComparisonStatusFlags flags = compare("nonexistent.txt", Utility::Path::join(FILETEST_DIR, "base.txt"));
        /* Should not return Diagnostic as there's no file to read from */
        CORRADE_COMPARE(flags, ComparisonStatusFlag::Failed);
        compare.printMessage(flags, redirectOutput, "a", "b");
    }

    CORRADE_COMPARE(out, "Actual file a (nonexistent.txt) cannot be read.\n");
}

void FileTest::expectedNotFound() {
    Containers::String out;

    Comparator<Compare::File> compare;
    ComparisonStatusFlags flags = compare(Utility::Path::join(FILETEST_DIR, "base.txt"), "nonexistent.txt");
    /* Should return Diagnostic even though we can't find the expected file
       as it doesn't matter */
    CORRADE_COMPARE(flags, ComparisonStatusFlag::Failed|ComparisonStatusFlag::Diagnostic);

    {
        Debug redirectOutput{&out};
        compare.printMessage(flags, redirectOutput, "a", "b");
    }

    CORRADE_COMPARE(out, "Expected file b (nonexistent.txt) cannot be read.\n");

    /* Create the output dir if it doesn't exist, but avoid stale files making
       false positives */
    CORRADE_VERIFY(Utility::Path::make(FILETEST_SAVE_DIR));
    Containers::String filename = Utility::Path::join(FILETEST_SAVE_DIR, "nonexistent.txt");
    if(Utility::Path::exists(filename))
        CORRADE_VERIFY(Utility::Path::remove(filename));

    {
        out = {};
        Debug redirectOutput{&out};
        compare.saveDiagnostic(flags, redirectOutput, FILETEST_SAVE_DIR);
    }

    /* Extreme dogfooding, eheh. We expect the *actual* contents, but under the
       *expected* filename */
    CORRADE_COMPARE(out, Utility::format("-> {}\n", filename));
    CORRADE_COMPARE_AS(filename,
        Utility::Path::join(FILETEST_DIR, "base.txt"), File);
}

void FileTest::differentContents() {
    Containers::String out;

    Comparator<Compare::File> compare{FILETEST_DIR};
    ComparisonStatusFlags flags = compare("different.txt", "base.txt");
    CORRADE_COMPARE(flags, ComparisonStatusFlag::Failed|ComparisonStatusFlag::Diagnostic);

    {
        Debug redirectOutput{&out};
        compare.printMessage(flags, redirectOutput, "a", "b");
    }

    CORRADE_COMPARE(out, "Files a and b have different contents. Actual character w but W expected on position 6.\n");

    /* Create the output dir if it doesn't exist, but avoid stale files making
       false positives */
    CORRADE_VERIFY(Utility::Path::make(FILETEST_SAVE_DIR));
    Containers::String filename = Utility::Path::join(FILETEST_SAVE_DIR, "base.txt");
    if(Utility::Path::exists(filename))
        CORRADE_VERIFY(Utility::Path::remove(filename));

    {
        out = {};
        Debug redirectOutput{&out};
        compare.saveDiagnostic(flags, redirectOutput, FILETEST_SAVE_DIR);
    }

    /* Extreme dogfooding, eheh. We expect the *actual* contents, but under the
       *expected* filename */
    CORRADE_COMPARE(out, Utility::format("-> {}\n", filename));
    CORRADE_COMPARE_AS(filename,
        Utility::Path::join(FILETEST_DIR, "different.txt"), File);
}

void FileTest::actualSmaller() {
    Containers::String out;

    {
        Debug redirectOutput{&out};
        Comparator<Compare::File> compare(FILETEST_DIR);
        ComparisonStatusFlags flags = compare("smaller.txt", "base.txt");
        CORRADE_COMPARE(flags, ComparisonStatusFlag::Failed|ComparisonStatusFlag::Diagnostic);
        compare.printMessage(flags, redirectOutput, "a", "b");
        /* not testing diagnostic as differentContents() tested this code path
           already */
    }

    CORRADE_COMPARE(out, "Files a and b have different size, actual 7 but 12 expected. Expected has character o on position 7.\n");
}

void FileTest::expectedSmaller() {
    Containers::String out;

    {
        Debug redirectOutput{&out};
        Comparator<Compare::File> compare(FILETEST_DIR);
        ComparisonStatusFlags flags = compare("base.txt", "smaller.txt");
        CORRADE_COMPARE(flags, ComparisonStatusFlag::Failed|ComparisonStatusFlag::Diagnostic);
        compare.printMessage(flags, redirectOutput, "a", "b");
        /* not testing diagnostic as differentContents() tested this code path
           already */
    }

    CORRADE_COMPARE(out, "Files a and b have different size, actual 12 but 7 expected. Actual has character o on position 7.\n");
}

}}}}}

CORRADE_TEST_MAIN(Corrade::TestSuite::Compare::Test::FileTest)
