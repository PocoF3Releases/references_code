#ifndef ANDROID_TEST_COMMON_H
#define ANDROID_TEST_COMMON_H

#define STDOUT stdout

#define COLOR_RED   "\x1B[31m"
#define COLOR_GRN   "\x1B[32m"
#define COLOR_YEL   "\x1B[33m"
#define COLOR_BLU   "\x1B[34m"
#define COLOR_MAG   "\x1B[35m"
#define COLOR_CYN   "\x1B[36m"
#define COLOR_WHT   "\x1B[37m"
#define COLOR_RESET "\x1B[0m"

#ifdef TEST_LOG_OPEN
#define TLOGE(...) { fprintf(STDOUT, __VA_ARGS__); printf("\n"); }
#define TLOGW(...) { fprintf(STDOUT, __VA_ARGS__); printf("\n"); }
#define TLOGI(...) { fprintf(STDOUT, __VA_ARGS__); printf("\n"); }
#define TLOGV(...) { fprintf(STDOUT, __VA_ARGS__); printf("\n"); }
#else
#define TLOGE(...) { }
#define TLOGW(...) { }
#define TLOGI(...) { }
#define TLOGV(...) { }
#endif

template <class A, class B>
inline
bool PLAIN_EQUAL(const A & a, const B & b) { return a == b; }

#define TEST_PRINT(...)   fprintf(STDOUT, __VA_ARGS__)
#define TEST_PRINT_OK     TEST_PRINT(COLOR_GRN "OK \n" COLOR_RESET)
#define TEST_PRINT_FAIL   TEST_PRINT(COLOR_RED "FAIL \n" COLOR_RESET)

#define TEST_ACTION(action)                                                             \
    {                                                                                   \
        TEST_PRINT(COLOR_YEL "ACTION: " #action " \n" COLOR_RESET);                     \
        action                                                                          \
    }

#define _TEST_ASSERT(msg, expr)                                                                 \
    {                                                                                   \
        TEST_PRINT(COLOR_YEL msg ": " COLOR_RESET);                                     \
        if (expr) { TEST_PRINT_OK; }                                                    \
        else { TEST_PRINT_FAIL; }                                                       \
    }

// Avoid parameter expending.
#define TEST_ASSERT_EQUAL(expect, actual, comp)                                         \
    {                                                                                   \
        bool b = (comp)((actual), (expect));                                            \
        _TEST_ASSERT(#actual " should be " #expect, b)                                  \
    }

// Avoid parameter expending.
#define TEST_ASSERT_NOT_EQUAL(not_expect, actual, comp)                                 \
    {                                                                                   \
        bool b = (comp)((actual), (not_expect)); b = !b;                                \
        _TEST_ASSERT(#actual " should not be " #not_expect, b)                          \
    }

// Avoid parameter expending.
#define TEST_ASSERT_EQUAL_PLAIN(expect, actual)                                         \
    {                                                                                   \
        bool b = (PLAIN_EQUAL)((actual), (expect));                                     \
        _TEST_ASSERT(#actual " should be " #expect, b)                                  \
    }

// Avoid parameter expending.
#define TEST_ASSERT_NOT_EQUAL_PLAIN(not_expect, actual)                                 \
    {                                                                                   \
        bool b = (PLAIN_EQUAL)((actual), (not_expect)); b = !b;                         \
        _TEST_ASSERT(#actual " should not be " #not_expect, b)                          \
    }

#define BEGIN_TEST   fprintf(STDOUT, COLOR_BLU "TEST BIGIN \n" COLOR_RESET)
#define END_TEST     fprintf(STDOUT, COLOR_BLU "TEST END   \n" COLOR_RESET)


#endif //ANDROID_TEST_COMMON_H
