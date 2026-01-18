#pragma once

#include "Utility/Log.hpp"

#include <vector>
#include <functional>
#include <thread>
#include <future>
#include <queue>

class ThreadPool;

class Thread {
public:
	explicit Thread(std::mutex* mutex) : m_thread(&Thread::ProcessQueue, this), m_mutex(mutex), m_terminated(false) {
        if (mutex == nullptr)
            LogError("mutex must be vaild!!!");
    }

	virtual ~Thread() { Terminate();  m_thread.join(); }

	using Task = std::function<void()>;

    [[nodiscard]] bool IsIdle() const noexcept { return !m_queue.empty(); }
    [[nodiscard]] bool IsTerminated() const noexcept { return m_terminated; }
    [[nodiscard]] size_t TaskCount() const noexcept { return m_queue.size(); }
    
	void AddTask(const Task& task) { m_queue.push(task); }
	void Notify() { m_conditionVariable.notify_one(); }
	void WaitForIdle();
	void Terminate();

	Thread(const Thread&) = delete;
	Thread& operator=(const Thread&) = delete;
	Thread(Thread&&) = delete;
	Thread& operator=(Thread&&) = delete;
private:
	void ProcessQueue();

	std::thread m_thread;
	std::queue<Task> m_queue;
	std::condition_variable m_conditionVariable;
    std::mutex* m_mutex;
	bool m_terminated;
};

class ThreadPool {
public:
	explicit ThreadPool(uint32_t threadCount = std::thread::hardware_concurrency());
	~ThreadPool();

	[[nodiscard]] Thread* GetThread(const uint32_t i) const { return m_threads[i].get(); }
	[[nodiscard]] const std::vector<std::unique_ptr<Thread>>& GetThreads() const { return m_threads; }
	[[nodiscard]] const std::mutex& GetMutex() const { return m_mutex; }

	Thread* AddTask(const Thread::Task& task);
private:
	std::vector<std::unique_ptr<Thread>> m_threads;
	std::mutex m_mutex;

	friend Thread;
};

inline void Thread::ProcessQueue() {
	std::unique_lock<std::mutex> lock(*m_mutex);
	while (!m_terminated) {
		m_conditionVariable.wait(lock, [this] { return ( IsIdle() || IsTerminated() ); });

		lock.unlock();
		while (!m_queue.empty()) {
			auto& task = m_queue.front();
			task();
			m_queue.pop();
		}

		lock.lock();
		m_conditionVariable.notify_one();
	}
}

inline void Thread::WaitForIdle() {
	std::unique_lock<std::mutex> lock(*m_mutex);
	m_conditionVariable.wait(lock, [this] { return m_queue.empty(); });
}

inline void Thread::Terminate() {
	WaitForIdle();

	m_terminated = true;
	m_conditionVariable.notify_one();
}

inline ThreadPool::ThreadPool(const uint32_t threadCount) {
	m_threads.resize(threadCount);
	for (auto& thread : m_threads)
		thread = std::make_unique<Thread>(&m_mutex);
}

inline Thread* ThreadPool::AddTask(const Thread::Task& task) {
    Thread* minTaskThread = GetThread(0);
    size_t minTaskCount = minTaskThread->TaskCount();

    for (auto& thread : m_threads) {
        if (thread->TaskCount() == 0) {
            minTaskThread = thread.get();
            break;
        }

        if (thread->TaskCount() <= minTaskCount) {
            minTaskThread = thread.get();
            minTaskCount = thread->TaskCount();
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        minTaskThread->AddTask(task);
    }
    minTaskThread->Notify();

    return minTaskThread;
}

inline ThreadPool::~ThreadPool() {
    m_threads.clear();
}