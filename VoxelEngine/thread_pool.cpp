#include "thread_pool.h"

#include "logger.h"


ThreadPool::ThreadPool(unsigned int numThreads)
	: m_Threads(numThreads)
	, m_BusyThreads(numThreads)
	, m_Shutdown(false)
{
	LOG_INFO(
		EngineSystem::CORE,
		"Creating ThreadPool with {} worker threads",
		numThreads);

	for (size_t i = 0; i < numThreads; ++i)
	{
		m_Threads[i] = std::thread(ThreadWorker(this));
	}
}

ThreadPool::~ThreadPool()
{
	Shutdown();
}

void ThreadPool::Shutdown()
{
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		if (m_Shutdown)
		{
			LOG_WARN(
				EngineSystem::CORE,
				"ThreadPool shutdown requested but pool is already shutting down");

			return;
		}

		LOG_INFO(
			EngineSystem::CORE,
			"ThreadPool shutdown");

		m_Shutdown = true;
		m_Cv.notify_all();
	}

	for (size_t i = 0; i < m_Threads.size(); ++i)
	{
		if (m_Threads[i].joinable())
		{
			m_Threads[i].join();
		}
	}
}
