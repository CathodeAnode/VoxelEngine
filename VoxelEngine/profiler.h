#ifndef	PERFORMANCE_PROFILER_H
#define PERFORMANCE_PROFILER_H

// Reference: https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Hazel/Debug/Instrumentor.h (The Cherno hazel engine)

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <string>
#include <thread>
#include <mutex>
#include <sstream>

#include "logger.h"

struct ProfileResult
{
	std::string Name;
	std::chrono::duration<double, std::micro> Start;
	std::chrono::duration<double, std::micro> ElapsedTime;
	std::thread::id ThreadID;
};

struct ProfilerSession
{
	std::string Name;
};

class Profiler
{
public:
	Profiler(const Profiler&) = delete;
	Profiler(Profiler&&) = delete;

	static Profiler& GetInstance();

	void BeginSession(const std::string& name, const std::string& filepath = "profiling_result.json");
	void EndSession();
	void WriteProfile(const ProfileResult& result);

	void SetEnabled(bool isEnabled);
	bool IsEnabled() { return m_Enabled; }
	
private:
	Profiler();
	~Profiler();

	void _WriteHeader();
	void _WriteFooter();
	void _InternalEndSession();

private:
	std::mutex m_Mutex;
	ProfilerSession* m_CurrentSession;
	std::ofstream m_OutputStream;

	bool m_Enabled;
};

class ProfilerTimer
{
public:
	ProfilerTimer(const char* name)
		: m_Name(name)
	{
		if (Profiler::GetInstance().IsEnabled())
		{
			m_StartTimepoint = std::chrono::steady_clock::now();
			m_Stopped = false;
		}
		else
		{
			m_Stopped = true;
		}
	}

	~ProfilerTimer()
	{
		if (!m_Stopped)
			Stop();
	}

	void Stop()
	{
		auto endTimepoint = std::chrono::steady_clock::now();
		auto highResStart = std::chrono::duration<double, std::micro>{ m_StartTimepoint.time_since_epoch() };
		auto elapsedTime = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch() - std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch();

		Profiler::GetInstance().WriteProfile({m_Name, highResStart, elapsedTime, std::this_thread::get_id()});

		m_Stopped = true;
	}
private:
	const char* m_Name;
	std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
	bool m_Stopped;
};

namespace ProfilerUtils {

	template <size_t N>
	struct ChangeResult
	{
		char Data[N];
	};

	template <size_t N, size_t K>
	constexpr auto CleanupOutputString(const char(&expr)[N], const char(&remove)[K])
	{
		ChangeResult<N> result = {};

		size_t srcIndex = 0;
		size_t dstIndex = 0;
		while (srcIndex < N)
		{
			size_t matchIndex = 0;
			while (matchIndex < K - 1 && srcIndex + matchIndex < N - 1 && expr[srcIndex + matchIndex] == remove[matchIndex])
				matchIndex++;
			if (matchIndex == K - 1)
				srcIndex += matchIndex;
			result.Data[dstIndex++] = expr[srcIndex] == '"' ? '\'' : expr[srcIndex];
			srcIndex++;
		}
		return result;
	}
}


#if 1
// TODO: move to macros header file
#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
#define FUNC_SIG __PRETTY_FUNCTION__
#elif defined(__DMC__) && (__DMC__ >= 0x810)
#define FUNC_SIG __PRETTY_FUNCTION__
#elif (defined(__FUNCSIG__) || (_MSC_VER))
#define FUNC_SIG __FUNCSIG__
#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
#define FUNC_SIG __FUNCTION__
#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
#define FUNC_SIG __FUNC__
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
#define FUNC_SIG __func__
#elif defined(__cplusplus) && (__cplusplus >= 201103)
#define FUNC_SIG __func__
#else
#define FUNC_SIG "FUNC_SIG unknown!"
#endif

#define PROFILE_SCOPE_LINE2(name, line)			constexpr auto fixedName##line = ProfilerUtils::CleanupOutputString(name, "__cdecl ");\
													ProfilerTimer timer##line(fixedName##line.Data)

#define PROFILE_BEGIN_SESSION(name, filepath)	Profiler::GetInstance().BeginSession(name, filepath)
#define PROFILE_END_SESSION()					Profiler::GetInstance().EndSession()
#define PROFILE_SCOPE_LINE(name, line)			PROFILE_SCOPE_LINE2(name, line)
#define PROFILE_SCOPE(name)						PROFILE_SCOPE_LINE(name, __LINE__)
#define PROFILE_FUNCTION()						PROFILE_SCOPE(FUNC_SIG)
#else
#define PROFILE_BEGIN_SESSION(name, filepath)	(void)0;
#define PROFILE_END_SESSION()					(void)0;
#define PROFILE_SCOPE_LINE(name, line)			(void)0;
#define PROFILE_SCOPE(name)						(void)0;
#define PROFILE_FUNCTION()						(void)0;
#endif

#endif
