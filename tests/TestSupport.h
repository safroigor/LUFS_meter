#pragma once

#include <cmath>
#include <cstdio>
#include <string>

/**
    Minimal dependency-free test support.

    We deliberately avoid pulling in a full unit-test framework: the checks here
    are simple numeric assertions, and a framework would add a dependency
    without adding value for this project.
*/
namespace test
{
    inline int& failureCount() { static int n = 0; return n; }
    inline int& checkCount()   { static int n = 0; return n; }

    inline std::string num (double v)
    {
        char buffer[64];
        std::snprintf (buffer, sizeof (buffer), "%.6f", v);
        return buffer;
    }

    inline bool check (bool ok,
                       const char* expression,
                       const char* file,
                       int line,
                       const std::string& detail = {})
    {
        ++checkCount();
        if (! ok)
        {
            ++failureCount();
            std::printf ("  FAIL %s:%d: %s",
                         file, line, expression);
            if (! detail.empty())
                std::printf ("  [%s]", detail.c_str());
            std::printf ("\n");
        }
        return ok;
    }
}

#define TEST_CHECK(cond) \
    test::check ((cond), #cond, __FILE__, __LINE__)

#define TEST_CHECK_NEAR(actual, expected, tolerance) \
    test::check (std::abs ((actual) - (expected)) <= (tolerance), \
                 #actual " near " #expected, __FILE__, __LINE__, \
                 std::string ("got ") + test::num ((actual)) \
                   + ", expected " + test::num ((expected)))

#define TEST_CHECK_NOT_A_NUMBER(value) \
    test::check (std::isnan (value), #value " is Not-a-Number", __FILE__, __LINE__, \
                 std::string ("got ") + test::num ((value)))
