#include "Igniter/Igniter.h"
#include "Igniter/Core/Log.h"

IG_DEFINE_LOG_CATEGORY(LogTemp);

IG_DEFINE_LOG_CATEGORY(LogEnsure);

IG_DEFINE_LOG_CATEGORY(LogError);

namespace ig
{
    Logger::Logger()
    {
        const std::string pattern = "[%Y-%m-%d %H:%M:%S] [%n]  %^%v%$";
        consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>(spdlog::color_mode::always);
        consoleSink->set_pattern(pattern);
        fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("log.txt", true);
        fileSink->set_pattern(pattern);
    }

    Logger::~Logger()
    {
        for (const auto& [typeHash, loggerPtr] : categories)
        {
            delete loggerPtr;
        }
    }

    thread_local bool gThreadLogSuppressed = false;

    void Logger::SuppressLogInCurrentThread()
    {
        gThreadLogSuppressed = true;
    }

    void Logger::UnsuppressLogInCurrentThread()
    {
        gThreadLogSuppressed = false;
    }

    bool Logger::IsLogSuppressedInCurrentThread()
    {
        return gThreadLogSuppressed;
    }
} // namespace ig
