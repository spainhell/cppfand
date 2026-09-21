#include "Logging.h"

#include <spdlog/cfg/env.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>

namespace
{
	constexpr const char* kLoggerName = "fand";
	constexpr const char* kFileName = "fand.log";
	constexpr std::size_t kMaxFileSize = 5 * 1024 * 1024;
	constexpr std::size_t kMaxFiles = 3;

	bool initialized = false;

	std::string GetEnv(const char* name)
	{
		size_t requiredSize = 0;
		getenv_s(&requiredSize, nullptr, 0, name);
		if (requiredSize == 0) return {};

		std::unique_ptr<char[]> buffer = std::make_unique<char[]>(requiredSize);
		getenv_s(&requiredSize, buffer.get(), requiredSize, name);
		return std::string(buffer.get());
	}

	// FANDWORK\fand.log; kdyz promenna neni nastavena, tak fand.log v aktualnim adresari.
	std::string LogFilePath()
	{
		const std::string work = GetEnv("FANDWORK");
		if (work.empty()) return kFileName;
		return (std::filesystem::path(work) / kFileName).string();
	}
}

void Log::Init()
{
	if (initialized) return;
	initialized = true;

	try {
		auto logger = spdlog::rotating_logger_mt(kLoggerName, LogFilePath(), kMaxFileSize, kMaxFiles);
		// 2020-10-07 15:00:39.775 [8064] [info] [base.cpp:413] zprava
		logger->set_pattern("%Y-%m-%d %H:%M:%S.%e [%t] [%l] [%s:%#] %v");
		logger->set_level(spdlog::level::debug);
		logger->flush_on(spdlog::level::warn);
		spdlog::set_default_logger(logger);
	}
	catch (const spdlog::spdlog_ex&) {
		// Log se nepodarilo otevrit (chybejici adresar, prava, zamceny soubor).
		// Aplikace musi bezet dal, takze zahazujeme vsechny zpravy.
		spdlog::set_default_logger(spdlog::null_logger_mt(kLoggerName));
	}

	// Dovoli prepsat uroven za behu, napr. SPDLOG_LEVEL=warn nebo SPDLOG_LEVEL=fand=trace.
	spdlog::cfg::load_env_levels();
}

void Log::Shutdown()
{
	spdlog::shutdown();
	// spdlog::shutdown() vynuluje vychozi logger, takze by SPDLOG_* makra sahala
	// na nullptr. Podstrcime zahazovaci logger, aby pozdni zapis byl no-op, ne pad.
	spdlog::set_default_logger(
		std::make_shared<spdlog::logger>("discard", std::make_shared<spdlog::sinks::null_sink_mt>()));
	initialized = false;
}
