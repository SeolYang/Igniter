#include <Asset/AssetCache.h>
#include <Core/Assert.h>
#include <Core/Log.h>
#include <Core/Igniter.h>
#include <fstream>

namespace ig
{
    IG_DEFINE_LOG_CATEGORY(AssetCacheInfo, ELogVerbosity::Info)
    IG_DEFINE_LOG_CATEGORY(AssetCacheWarn, ELogVerbosity::Warning)
    IG_DEFINE_LOG_CATEGORY(AssetCacheErr, ELogVerbosity::Error)
    IG_DEFINE_LOG_CATEGORY(AssetCacheDbg, ELogVerbosity::Debug)
    IG_DEFINE_LOG_CATEGORY(AssetCacheFatal, ELogVerbosity::Fatal)
} // namespace ig