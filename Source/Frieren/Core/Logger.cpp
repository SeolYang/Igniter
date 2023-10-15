#include <Core/Logger.h>

namespace fe
{
	Logger::Logger()
	{
		const std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [%n]  %^%v%$";
		consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>(spdlog::color_mode::always);
		consoleSink->set_pattern(pattern);
		fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("log.txt", true);
		fileSink->set_pattern(pattern);

		RegisterCategory<LogTemp>();
		RegisterCategory<LogInfo>();
		RegisterCategory<LogDebug>();
		RegisterCategory<LogWarn>();
		RegisterCategory<LogFatal>();
	}

	Logger::~Logger()
	{
		for (auto& logger : categoryMap)
		{
			delete logger.second;
		}
	}

}