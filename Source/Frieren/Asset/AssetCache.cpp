#include <Asset/AssetCache.h>
#include <Core/Assert.h>
#include <Core/Log.h>
#include <Engine.h>
#include <fstream>

namespace fe
{
	FE_DEFINE_LOG_CATEGORY(AssetCacheInfo, ELogVerbosity::Info)
	FE_DEFINE_LOG_CATEGORY(AssetCacheWarn, ELogVerbosity::Warning)
	FE_DEFINE_LOG_CATEGORY(AssetCacheErr, ELogVerbosity::Error)
	FE_DEFINE_LOG_CATEGORY(AssetCacheDbg, ELogVerbosity::Debug)
	FE_DEFINE_LOG_CATEGORY(AssetCacheFatal, ELogVerbosity::Fatal)

} // namespace fe