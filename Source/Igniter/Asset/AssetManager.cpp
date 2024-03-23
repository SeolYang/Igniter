#include <Igniter.h>
#include <Asset/AssetManager.h>
#include <Core/Assert.h>
#include <Core/Log.h>
#include <Core/Engine.h>

namespace ig
{
    IG_DEFINE_LOG_CATEGORY(AssetManagerInfo, ELogVerbosity::Info)
    IG_DEFINE_LOG_CATEGORY(AssetManagerWarn, ELogVerbosity::Warning)
    IG_DEFINE_LOG_CATEGORY(AssetManagerErr, ELogVerbosity::Error)
    IG_DEFINE_LOG_CATEGORY(AssetManagerDbg, ELogVerbosity::Debug)
    IG_DEFINE_LOG_CATEGORY(AssetManagerFatal, ELogVerbosity::Fatal)
} // namespace ig