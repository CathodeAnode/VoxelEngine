#ifndef __VE_THREAD_POOL_H
#define __VE_THREAD_POOL_H

#include <memory>
#include <queue>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <future>

// Zen Sepiol: https://www.youtube.com/watch?v=6re5U82KwbY

class ThreadPool
{
public:
	ThreadPool(unsigned int numThreads);
	~ThreadPool();

	void Shutdown();

	ThreadPool(ThreadPool& other) = delete;
	ThreadPool(ThreadPool&& other) = delete;
	ThreadPool& operator=(ThreadPool& other) = delete;
	ThreadPool& operator=(ThreadPool&& other) = delete;

	template<typename F, typename... Args>
	auto AddTask(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
	{
		std::packaged_task<decltype(f(args...))()> task(std::forward<F>(f), std::forward<Args>(args)...);
		auto future = task.get_future();
		auto wrapperFunc = [task = std::move(task)]() mutable { std::move(task)(); };

		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			m_Queue.emplace(std::move(wrapperFunc));
			m_Cv.notify_one();
		}

		return future;
	}

private:
	class ThreadWorker
	{
	public:
		ThreadWorker(ThreadPool* pool)
			: m_ThreadPool(pool)
		{
		}

		void operator()()
		{
			std::unique_lock<std::mutex> lock(m_ThreadPool->m_Mutex);
			while (!m_ThreadPool->m_Shutdown || (m_ThreadPool->m_Shutdown && !m_ThreadPool->m_Queue.empty()))
			{
				m_ThreadPool->m_BusyThreads--;
				m_ThreadPool->m_Cv.wait(lock, [this] {
					return this->m_ThreadPool->m_Shutdown || !m_ThreadPool->m_Queue.empty();
					});
				m_ThreadPool->m_BusyThreads++;

				if (!this->m_ThreadPool->m_Queue.empty())
				{
					auto func = std::move(m_ThreadPool->m_Queue.front());
					m_ThreadPool->m_Queue.pop();

					lock.unlock();
					func();
					lock.lock();
				}
			}
		}


	private:
		ThreadPool* m_ThreadPool;
	};

private:
	mutable std::mutex m_Mutex;
	std::condition_variable m_Cv;

	std::queue<std::move_only_function<void()>> m_Queue;
	std::vector<std::thread> m_Threads;

	int m_BusyThreads;
	bool m_Shutdown;

};



#endif

