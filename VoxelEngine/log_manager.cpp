#include "log_manager.h"


LogManager::LogManager()
{
#define X(ENGINESYSTEM)	m_LoggerMap[EngineSystem::ENGINESYSTEM] = std::make_shared<spdlog::logger>(#ENGINESYSTEM);
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

LogManager* LogManager::GetInstance()
{
	if (m_Instance == nullptr)
	{
		m_Instance = new LogManager();
	}

	return m_Instance;
}

std::shared_ptr<spdlog::logger> LogManager::GetLogger(EngineSystem sys)
{
	return m_LoggerMap[sys];
}


