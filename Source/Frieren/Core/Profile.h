#pragma once
#if defined(DEBUG) || defined(_DEBUG)
#define TRACY_ENABLE
#else
#endif

#include <tracy/Tracy.hpp>
