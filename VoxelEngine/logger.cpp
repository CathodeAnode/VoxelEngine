#include "logger.h"


LogManager::LogManager()
{
	m_ConsoleSink->set_pattern("[%T.%e] %^[%=8l]%$ [%n]: %v");

	// Set colors for each log level
	m_ConsoleSink->set_color(spdlog::level::trace, "\033[90m");     // Trace - Bright Black (Light Gray)
	m_ConsoleSink->set_color(spdlog::level::debug, "\033[94m");     // Debug - Bright Blue
	m_ConsoleSink->set_color(spdlog::level::info, "\033[92m");     // Info - Bright Green
	m_ConsoleSink->set_color(spdlog::level::warn, "\033[93m");     // Warn - Bright Yellow
	m_ConsoleSink->set_color(spdlog::level::err, "\033[91m");     // Error - Bright Red
	m_ConsoleSink->set_color(spdlog::level::critical, "\033[95;1m"); // Critical - Bright Magenta & Bold


#define X(ENGINESYSTEM)	m_LoggerMap[EngineSystem::ENGINESYSTEM] = std::make_shared<spdlog::logger>(#ENGINESYSTEM, m_ConsoleSink);
	ENGINE_SYSTEMS
#undef X

#define X(ENGINESYSTEM)	m_LoggerMap[EngineSystem::ENGINESYSTEM]->set_level(spdlog::level::info);
		ENGINE_SYSTEMS
#undef X

}

void LogManager::Initialize(const LogConfig& cfg)
{
	assert(s_Instance == nullptr);
	s_Instance = new LogManager();

	LOG_INFO(EngineSystem::CORE,
		"Initializing logging");

#define X(ENGINESYSTEM) GetInstance()->SetLevel(EngineSystem::ENGINESYSTEM, cfg.ENGINESYSTEM);
	ENGINE_SYSTEMS
#undef X

}

void LogManager::Shutdown()
{
	LOG_INFO(EngineSystem::CORE,
		"Shutdown logging");

	delete s_Instance;
	s_Instance = nullptr;
}

LogManager* LogManager::GetInstance()
{
	assert(s_Instance != nullptr);

	return s_Instance;
}

std::shared_ptr<spdlog::logger> LogManager::GetLogger(EngineSystem sys)
{
	return m_LoggerMap[sys];
}

void LogManager::SetLevel(EngineSystem sys, LogLevel lvl)
{
	assert(s_Instance != nullptr);

	LOG_INFO(EngineSystem::CORE, "{:<12} : LogLevel set to {:<8}", 
		LoggerUtils::ToString(sys),
		LoggerUtils::ToString(lvl));

	m_LoggerMap[sys]->set_level(static_cast<spdlog::level::level_enum>(lvl));
}


