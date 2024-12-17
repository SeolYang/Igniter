#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/Hash.h"

namespace ig
{
    enum class ELogVerbosity
    {
        Trace,
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

        template <typename Category, ELogVerbosity Verbosity, typename... Args>
        void Log(const std::string_view logMessage, Args&&... args)
        {
            spdlog::logger& logger = QueryCategory<Category>();
            const std::string formattedMessage = std::vformat(logMessage, std::make_format_args(args...));
            switch (Verbosity)
            {
            case ELogVerbosity::Info:
                logger.info(formattedMessage);
                break;
            case ELogVerbosity::Trace:
                logger.trace(formattedMessage);
                break;

            case ELogVerbosity::Warning:
                logger.warn(formattedMessage);
                break;

            case ELogVerbosity::Error:
                logger.error(formattedMessage);
                break;

            case ELogVerbosity::Fatal:
                logger.critical(formattedMessage);
                break;

            case ELogVerbosity::Debug:
                logger.debug(formattedMessage);
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
        spdlog::logger& QueryCategory()
        {
            UniqueLock lock{ categoryMapMutex };
            auto itr = categoryMap.find(TypeHash64<C>);
            if (itr == categoryMap.end())
            {
                spdlog::logger* newLogger = new spdlog::logger(C::CategoryName.data(), { consoleSink, fileSink });
                newLogger->set_level(spdlog::level::trace);
                itr = categoryMap.insert(itr, std::make_pair(TypeHash64<C>, newLogger));
            }

            return *itr->second;
        }

    private:
        std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> consoleSink;
        std::shared_ptr<spdlog::sinks::basic_file_sink_mt> fileSink;
        Mutex categoryMapMutex;
        UnorderedMap<uint64_t, spdlog::logger*> categoryMap;

    private:
        static constexpr std::string_view FileName = "Log";
    };
}    // namespace ig

/* #sy_note 카테고리의 고유성을 보장하기 위해, 네임스페이스의 밖에 정의되어야 함. */
#define IG_DEFINE_LOG_CATEGORY(LOG_CATEGORY_NAME)                                \
    namespace ig::categories                                                     \
    {                                                                            \
        struct LOG_CATEGORY_NAME                                                 \
        {                                                                        \
            static constexpr std::string_view CategoryName = #LOG_CATEGORY_NAME; \
        };                                                                       \
    }

#if defined(DEBUG) || defined(_DEBUG)
#define IG_LOG(LOG_CATEGORY, LOG_VERBOSITY, MESSAGE, ...)                                                                \
    ig::Logger::GetInstance().Log<ig::categories::LOG_CATEGORY, ig::ELogVerbosity::LOG_VERBOSITY>(MESSAGE, __VA_ARGS__); \
    if constexpr (ig::ELogVerbosity::LOG_VERBOSITY == ig::ELogVerbosity::Fatal)                                          \
    {                                                                                                                    \
        IG_CHECK_NO_ENTRY();                                                                                             \
    }
#else
#define IG_LOG(LOG_CATEGORY, LOG_VERBOSITY, MESSAGE, ...) \
    ig::Logger::GetInstance().Log<ig::categories::LOG_CATEGORY, ig::ELogVerbosity::LOG_VERBOSITY>(MESSAGE, __VA_ARGS__)
#endif

IG_DEFINE_LOG_CATEGORY(LogTemp);

IG_DEFINE_LOG_CATEGORY(LogEnsure);
