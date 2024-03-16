#pragma once
#if (defined(_DEBUG) || defined(DEBUG))
    #define IG_VERIFY(EXPRESSION) \
        do                        \
            if (!(EXPRESSION))    \
            {                     \
                __debugbreak();   \
            }                     \
        while (false)

    #define IG_CHECK(CONDITION) IG_VERIFY(CONDITION)
#else
    #define IG_VERIFY(EXPRESSION) \
        do                        \
        {                         \
            (EXPRESSION);         \
        } while (false)

    #define IG_CHECK(CONDITION) ((void)0)
#endif

#define IG_CHECK_NO_ENTRY()      IG_CHECK(false)
#define IG_UNIMPLEMENTED()       IG_CHECK(false)

#define IG_VERIFY_SUCCEEDED(RESULT) IG_VERIFY(SUCCEEDED(RESULT))
#define IG_CHECK_SUCCEEDED(RESULT)  IG_CHECK(SUCCEEDED(RESULT))