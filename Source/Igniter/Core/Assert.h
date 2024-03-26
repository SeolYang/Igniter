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

#define IG_CHECK_NO_ENTRY()         IG_CHECK(false)
#define IG_UNIMPLEMENTED()          IG_CHECK(false)

#define IG_VERIFY_SUCCEEDED(RESULT) IG_VERIFY(SUCCEEDED(RESULT))
#define IG_CHECK_SUCCEEDED(RESULT)  IG_CHECK(SUCCEEDED(RESULT))

#define IG_ENSURE(EXPR) \
    [](const bool bExpression) { if (!bExpression) { IG_LOG(LogEnsure, Error, "\"{}\" => \"{}\":{}", #EXPR, __FILE__, __LINE__); IG_CHECK_NO_ENTRY(); } return bExpression; }(EXPR);


#define IG_ENSURE_MSG(EXPR, MESSAGE) \
    [](const bool bExpression) { if (!bExpression) { IG_LOG(LogEnsure, Error, "{}: \"{}\" => \"{}\":{}", MESSAGE, #EXPR, __FILE__, __LINE__); IG_CHECK_NO_ENTRY(); } return bExpression; }(EXPR);