#pragma once
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <robin_hood.h>
#include <format>
#include <Core/Name.h>
#include <Core/Hash.h>
#include <Core/Mutex.h>

namespace fe
{
	enum class ELogVerbosiy
	{
		Temp,
		Info,
		Debug,
		Warning,
		Fatal
	};

	class Logger final
	{
	public:
		Logger();
		~Logger();

		template <typename C, typename... Args>
		void Log(const std::string_view logMessage, Args&&... args)
		{
			spdlog::logger* logger = QueryCategory<C>();
			if (logger == nullptr)
			{
				WriteLock lock{ mutex };
				logger = new spdlog::logger(C::CategoryName.data(), { consoleSink, fileSink });
				logger->set_level(spdlog::level::trace);
			}

			const std::string formattedMessage = std::vformat(logMessage, std::make_format_args(std::forward<Args>(args)...));
			switch (C::Verbosity)
			{
				case ELogVerbosiy::Info:
					logger->info(formattedMessage);
					break;
				case ELogVerbosiy::Temp:
					logger->trace(formattedMessage);
					break;

				case ELogVerbosiy::Warning:
					logger->warn(formattedMessage);
					break;

				case ELogVerbosiy::Fatal:
					logger->critical(formattedMessage);
					break;

				case ELogVerbosiy::Debug:
					logger->debug(formattedMessage);
					break;
			}
		}

	private:
		template <typename C>
		spdlog::logger* QueryCategory() const
		{
			ReadOnlyLock lock{ mutex };
			return categoryMap.contains(HashOfType<C>) ? categoryMap.find(HashOfType<C>)->second : nullptr;
		}

	private:
		mutable SharedMutex mutex;

		std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> consoleSink;
		std::shared_ptr<spdlog::sinks::basic_file_sink_mt>	 fileSink;
		robin_hood::unordered_map<uint64_t, spdlog::logger*> categoryMap;

	private:
		static constexpr std::string_view FileName = "Log";
	};
} // namespace fe

#define FE_DECLARE_LOG_CATEGORY(LOG_CATEGORY_NAME, VERBOSITY_LEVEL)          \
	struct LOG_CATEGORY_NAME                                                 \
	{                                                                        \
		static constexpr fe::ELogVerbosiy Verbosity = VERBOSITY_LEVEL;       \
		static constexpr std::string_view CategoryName = #LOG_CATEGORY_NAME; \
	}

#define FE_LOG(LOG_CATEGORY, ...) fe::Engine::GetLogger().Log<LOG_CATEGORY>(__VA_ARGS__)

namespace fe
{
	FE_DECLARE_LOG_CATEGORY(LogTemp, ELogVerbosiy::Temp);
	FE_DECLARE_LOG_CATEGORY(LogInfo, ELogVerbosiy::Info);
	FE_DECLARE_LOG_CATEGORY(LogDebug, ELogVerbosiy::Debug);
	FE_DECLARE_LOG_CATEGORY(LogWarn, ELogVerbosiy::Warning);
	FE_DECLARE_LOG_CATEGORY(LogFatal, ELogVerbosiy::Fatal);
} // namespace fe