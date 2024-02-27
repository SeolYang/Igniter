#include <Asset/AssetManager.h>
#include <Core/Assert.h>
#include <Core/Log.h>
#include <Engine.h>
#include <fstream>

namespace fe
{
	FE_DEFINE_LOG_CATEGORY(AssetManagerInfo, ELogVerbosiy::Info)
	FE_DEFINE_LOG_CATEGORY(AssetManagerWarn, ELogVerbosiy::Warning)
	FE_DEFINE_LOG_CATEGORY(AssetManagerErr, ELogVerbosiy::Error)
	FE_DEFINE_LOG_CATEGORY(AssetManagerDbg, ELogVerbosiy::Debug)
	FE_DEFINE_LOG_CATEGORY(AssetManagerFatal, ELogVerbosiy::Fatal)

} // namespace fe