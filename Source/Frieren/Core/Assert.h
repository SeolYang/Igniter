#pragma once
#if (defined(_DEBUG) || defined(DEBUG))
	#define FE_ASSERT(CONDITION) \
		do                                        \
			if (!(CONDITION))                     \
			{                                     \
				__debugbreak();                   \
			}                                     \
		while (false)

#else
	#define FE_ASSERT(CONDITION) \
		do                                        \
		{                                         \
			CONDITION;                            \
		}                                         \
		while (false)

	#define FE_FORCE_ASSERT(FORMAT_STR, ...)

#endif