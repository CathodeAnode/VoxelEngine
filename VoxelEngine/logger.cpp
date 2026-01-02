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

const char* LogManager::SysToString(EngineSystem sys)
{
	switch (sys)
	{
#define X(ENGINESYSTEM)			\
	case EngineSystem::ENGINESYSTEM :			\
		return #ENGINESYSTEM;	
		ENGINE_SYSTEMS
#undef X
	default:
		return "Unknown (X-macro failed)";
	}
	return nullptr;
}

void LogManager::Initialize()
{
	assert(!s_Instance);
	s_Instance = new LogManager();

	// TODO: log config parameters
	LOG_INFO(EngineSystem::CORE,
		"Initializing logging");
}

void LogManager::Shutdown()
{
	delete s_Instance;
	s_Instance = nullptr;

	LOG_INFO(EngineSystem::CORE,
		"Shutdown logging");
}

LogManager* LogManager::GetInstance()
{
	assert(s_Instance);

	return s_Instance;
}

std::shared_ptr<spdlog::logger> LogManager::GetLogger(EngineSystem sys)
{
	return m_LoggerMap[sys];
}


