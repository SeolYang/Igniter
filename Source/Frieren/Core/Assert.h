#pragma once
#if (defined(_DEBUG) || defined(DEBUG) || defined(FORCE_ENABLE_ASSERT)) && !defined(FORCE_DISABLE_ASSERT)
	#define FE_ASSERT(CONDITION, FORMAT_STR, ...) \
		do                                        \
			if (!(CONDITION))                     \
			{                                     \
				__debugbreak();                   \
			}                                     \
		while (false)

	#define FE_FORCE_ASSERT(FORMAT_STR, ...) \
		__debugbreak();

#else
	#define FE_ASSERT(CONDITION, FORMAT_STR, ...) \
		do                                        \
		{                                         \
			CONDITION;                            \
		}                                         \
		while (false)

	#define FE_FORCE_ASSERT(FORMAT_STR, ...)

#endif