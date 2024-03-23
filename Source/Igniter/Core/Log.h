#pragma once
#include <PCH.h>
#include <Core/Container.h>
#include <Core/HashUtils.h>
#include <Core/String.h>

namespace ig
{
    enum class ELogVerbosity
    {
        Temp,
        Info,
        Debug,
        Warning,
        Error,
        Fatal
    };

    // @dependency	None
    class Logger final
    {
    public:
        Logger(const Logger&) = delete;
        Logger(Logger&&) noexcept = delete;
        ~Logger();

        Logger& operator=(const Logger&) = delete;
        Logger& operator=(Logger&&) noexcept = delete;

        template <typename C, typename... Args>
        void Log(const std::string_view logMessage, Args&&... args)
        {
            spdlog::logger* logger = QueryCategory<C>();
            if (logger == nullptr)
            {
                ReadWriteLock lock{ mutex };
                logger = new spdlog::logger(C::CategoryName.data(), { consoleSink, fileSink });
                logger->set_level(spdlog::level::trace);
                categoryMap[HashOfType<C>] = logger;
            }

            const std::string formattedMessage =
                std::vformat(logMessage, std::make_format_args(std::forward<Args>(args)...));

            switch (C::Verbosity)
            {
                case ELogVerbosity::Info:
                    logger->info(formattedMessage);
                    break;
                case ELogVerbosity::Temp:
                    logger->trace(formattedMessage);
                    break;

                case ELogVerbosity::Warning:
                    logger->warn(formattedMessage);
                    break;

                case ELogVerbosity::Error:
                    logger->error(formattedMessage);
                    break;

                case ELogVerbosity::Fatal:
                    logger->critical(formattedMessage);
                    break;

                case ELogVerbosity::Debug:
                    logger->debug(formattedMessage);
                    break;
            }
        }

        static Logger& GetInstance()
        {
            static Logger instance{};
            return instance;
        }

    private:
        Logger();

        template <typename C>
        spdlog::logger* QueryCategory()
        {
            ReadOnlyLock lock{ mutex };
            return categoryMap.contains(HashOfType<C>) ? categoryMap.find(HashOfType<C>)->second : nullptr;
        }

    private:
        mutable SharedMutex mutex;
        std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> consoleSink;
        std::shared_ptr<spdlog::sinks::basic_file_sink_mt> fileSink;
        robin_hood::unordered_map<uint64_t, spdlog::logger*> categoryMap;

    private:
        static constexpr std::string_view FileName = "Log";
    };
} // namespace ig

#define IG_DEFINE_LOG_CATEGORY(LOG_CATEGORY_NAME, VERBOSITY_LEVEL)           \
    struct LOG_CATEGORY_NAME                                                 \
    {                                                                        \
        static constexpr ig::ELogVerbosity Verbosity = VERBOSITY_LEVEL;      \
        static constexpr std::string_view CategoryName = #LOG_CATEGORY_NAME; \
    };

#if defined(DEBUG) || defined(_DEBUG)
    #define IG_LOG(LOG_CATEGORY, ...)                                                                                             \
        ig::Logger::GetInstance().Log<LOG_CATEGORY>(__VA_ARGS__);                                                                 \
        if constexpr (LOG_CATEGORY::Verbosity == ig::ELogVerbosity::Error || LOG_CATEGORY::Verbosity == ig::ELogVerbosity::Fatal) \
        {                                                                                                                         \
            IG_CHECK_NO_ENTRY();                                                                                                  \
        }
#else
    #define IG_LOG(LOG_CATEGORY, ...) ig::Logger::GetInstance().Log<LOG_CATEGORY>(__VA_ARGS__)
#endif

#define IG_CONDITIONAL_LOG(LOG_CATEGORY, CONDITION, ...) \
    do                                                   \
        if (!(CONDITION))                                \
        {                                                \
            IG_LOG(LOG_CATEGORY, __VA_ARGS__);           \
        }                                                \
    while (false)

namespace ig
{
    IG_DEFINE_LOG_CATEGORY(LogTemp, ELogVerbosity::Temp)
    IG_DEFINE_LOG_CATEGORY(LogInfo, ELogVerbosity::Info)
    IG_DEFINE_LOG_CATEGORY(LogDebug, ELogVerbosity::Debug)
    IG_DEFINE_LOG_CATEGORY(LogWarn, ELogVerbosity::Warning)
    IG_DEFINE_LOG_CATEGORY(LogError, ELogVerbosity::Error)
    IG_DEFINE_LOG_CATEGORY(LogFatal, ELogVerbosity::Fatal)
} // namespace ig