#include <Asset/AssetManager.h>
#include <Core/Assert.h>
#include <Core/Log.h>
#include <Engine.h>
#include <fstream>

namespace fe
{
	FE_DEFINE_LOG_CATEGORY(AssetManagerInfo, ELogVerbosity::Info)
	FE_DEFINE_LOG_CATEGORY(AssetManagerWarn, ELogVerbosity::Warning)
	FE_DEFINE_LOG_CATEGORY(AssetManagerErr, ELogVerbosity::Error)
	FE_DEFINE_LOG_CATEGORY(AssetManagerDbg, ELogVerbosity::Debug)
	FE_DEFINE_LOG_CATEGORY(AssetManagerFatal, ELogVerbosity::Fatal)

} // namespace fe