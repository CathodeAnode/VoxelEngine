#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/ansicolor_sink.h>
#include <unordered_map>
#include <memory>

#define ENGINE_SYSTEMS	\
	X(GREEDY_MESHER)	\
	X(VOXEL_RENDERER)	\
	X(CHUNK_MANAGER)	\
	X(GPU_ALLOCATORS)

enum class EngineSystem
{
#define X(ENGINESYSTEM) ENGINESYSTEM,
	ENGINE_SYSTEMS
#undef X
};

class LogManager
{
public:
	static LogManager* GetInstance();

	// delete cloning for singleton pattern
	LogManager(LogManager&) = delete;
	void operator=(const LogManager&) = delete;

	std::shared_ptr<spdlog::logger> GetLogger(EngineSystem sys);

private:
	std::unordered_map<EngineSystem, std::shared_ptr<spdlog::logger>> m_LoggerMap;
	inline static LogManager* m_Instance = nullptr;

	std::shared_ptr<spdlog::sinks::ansicolor_stdout_sink_mt> m_ConsoleSink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();

private:
	LogManager();

	const char* SysToString(EngineSystem sys);

};

#if 1
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