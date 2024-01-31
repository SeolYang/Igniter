#pragma once
#if (defined(_DEBUG) || defined(DEBUG))
	#define verify(EXPRESSION)  \
		do                      \
			if (!(EXPRESSION))  \
			{                   \
				__debugbreak(); \
			}                   \
		while (false)

	#define check(CONDITION) verify(CONDITION)
#else
	#define verify(EXPRESSION) \
		do                     \
		{                      \
			EXPRESSION;        \
		}                      \
		while (false)

	#define check(CONDITION) ((void)0)
#endif

#define checkNoEntry()			 check(false)
#define checkNoReentry()		 check(false)
#define checkNoRecursion()		 check(false)
#define unimplemented()			 check(false)

#define verify_succeeded(RESULT) verify(SUCCEEDED(RESULT))
#define check_succeeded(RESULT)	 check(SUCCEEDED(RESULT))