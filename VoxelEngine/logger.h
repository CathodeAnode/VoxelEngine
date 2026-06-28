#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/ansicolor_sink.h>
#include <unordered_map>
#include <memory>

enum class LogLevel : int
{
	Trace = spdlog::level::trace,
	Debug = spdlog::level::debug,
	Info =  spdlog::level::info,
	Warn = spdlog::level::warn,
	Error = spdlog::level::err,
	Critical = spdlog::level::critical,
	Off = spdlog::level::off
};

#define ENGINE_SYSTEMS	\
	X(CORE)	\
	X(RENDERER)	\
	X(INPUTS)	\
	X(CHUNK)	\
	X(VOXEL_MESHER)	\
	X(SCENE)	\
	X(GPU_BUFFER)	\
	X(VOXEL_ENGINE)

enum class EngineSystem
{
#define X(ENGINESYSTEM) ENGINESYSTEM,
	ENGINE_SYSTEMS
#undef X
};

// TODO: option to output to file
struct LogConfig
{
#define X(ENGINESYSTEM) LogLevel ENGINESYSTEM = LogLevel::Info;
	ENGINE_SYSTEMS
#undef X
};

class LogManager
{
public:
	static void Initialize(const LogConfig& cfg);
	static void Shutdown();

	static LogManager* GetInstance();

	// delete cloning for singleton pattern
	LogManager(LogManager&) = delete;
	void operator=(const LogManager&) = delete;

	std::shared_ptr<spdlog::logger> GetLogger(EngineSystem sys);
	void SetLevel(EngineSystem sys, LogLevel lvl);

private:
	LogManager();

private:
	std::unordered_map<EngineSystem, std::shared_ptr<spdlog::logger>> m_LoggerMap;
	inline static LogManager* s_Instance = nullptr;

	std::shared_ptr<spdlog::sinks::ansicolor_stdout_sink_mt> m_ConsoleSink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();

};

namespace LoggerUtils
{
	constexpr const char* ToString(LogLevel lvl) noexcept
	{
		switch (lvl)
		{
		case LogLevel::Trace:    return "Trace";
		case LogLevel::Debug:    return "Debug";
		case LogLevel::Info:     return "Info";
		case LogLevel::Warn:     return "Warn";
		case LogLevel::Error:    return "Error";
		case LogLevel::Critical: return "Critical";
		case LogLevel::Off:      return "Off";
		default:                 return "Unknown";
		}
	}

	constexpr const char* ToString(EngineSystem sys) noexcept
	{
		switch (sys)
		{
#define X(ENGINESYSTEM)			\
	case EngineSystem::ENGINESYSTEM :			\
		return #ENGINESYSTEM;	
			ENGINE_SYSTEMS
#undef X
		default:
			return "?????";
		}
	}
}


#ifdef VE_LOGGING_ENABLED
#define LOG_TRACE(engineSys, ...)		LogManager::GetInstance()->GetLogger(engineSys)->trace(__VA_ARGS__);
#define LOG_DEBUG(engineSys, ...)		LogManager::GetInstance()->GetLogger(engineSys)->debug(__VA_ARGS__);
#define LOG_INFO(engineSys, ...)		LogManager::GetInstance()->GetLogger(engineSys)->info(__VA_ARGS__);
#define LOG_WARN(engineSys, ...)		LogManager::GetInstance()->GetLogger(engineSys)->warn(__VA_ARGS__);
#define LOG_ERROR(engineSys, ...)		LogManager::GetInstance()->GetLogger(engineSys)->error(__VA_ARGS__);
#define LOG_CRITICAL(engineSys, ...)	LogManager::GetInstance()->GetLogger(engineSys)->critical(__VA_ARGS__);
#else
#define LOG_TRACE(engineSys, ...)		(void)0;
#define LOG_DEBUG(engineSys, ...)		(void)0;
#define LOG_INFO(engineSys, ...)		(void)0;
#define LOG_WARN(engineSys, ...)		(void)0;
#define LOG_ERROR(engineSys, ...)		(void)0;
#define LOG_CRITICAL(engineSys, ...)	(void)0;
#endif

#endif