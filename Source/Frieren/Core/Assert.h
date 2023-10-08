#pragma once
#if defined(_DEBUG) || defined(DEBUG)
	#define FE_ASSERT(CONDITION, FORMAT_STR, ...) \
		do                                        \
			if (!(CONDITION))                     \
			{                                     \
				__debugbreak();                   \
			}                                     \
		while (0)
#else
	#define FE_ASSERT(CONDITION, FORMAT_STR, ...) \
		do                                        \
		{                                         \
			CONDITION;                            \
		}                                         \
		while (0)
#endif