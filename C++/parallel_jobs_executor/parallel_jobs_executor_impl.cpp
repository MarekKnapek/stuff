#include "parallel_jobs_executor_impl.h"

#include <cassert>


namespace
{
	static constexpr int const s_stop_thread_job_size = -1;
}


mk::detail::parallel_jobs_executor_impl_t::parallel_jobs_executor_impl_t() :
	m_context(),
	m_single_job_executor(),
	m_jobs_begin(),
	m_job_size(),
	m_job_index(),
	m_jobs_mtx(),
	m_threads(),
	m_consumers_cv(),
	m_producer_cv(),
	m_consumers_pending_mtx(),
	m_consumers_pending()
{
	int const processors_count =  static_cast<int>(std::thread::hardware_concurrency());
	m_threads.resize(processors_count);
	for(int i = 0; i != processors_count; ++i)
	{
		m_threads[i] = std::thread{&parallel_jobs_executor_impl_t::thread_proc, this};
	}
}

mk::detail::parallel_jobs_executor_impl_t::~parallel_jobs_executor_impl_t()
{
	{
		std::lock_guard<std::mutex> const lck_jobs{m_jobs_mtx};
		m_job_size = s_stop_thread_job_size;
	}
	m_consumers_cv.notify_all();
	for(auto& thread : m_threads)
	{
		thread.join();
	}
}

void mk::detail::parallel_jobs_executor_impl_t::submit_jobs_and_wait_for_completion(void* const& context, void* const& jobs_begin, int const& jobs_count, int const& job_size, single_job_executor_t const& single_job_executor)
{
	assert((jobs_count == 0) || ((jobs_count != 0) && (single_job_executor && jobs_begin)));
	assert((single_job_executor) || ((!single_job_executor) && (jobs_count == 0)));
	assert((jobs_begin) || ((!jobs_begin) && (jobs_count == 0)));
	assert(jobs_count >= 0);
	assert(job_size > 0);
	assert(job_size != s_stop_thread_job_size);
	{
		std::lock_guard<std::mutex> const lck_jobs{m_jobs_mtx};
		std::lock_guard<std::mutex> const lck_consumers{m_consumers_pending_mtx};
		m_context = context;
		m_single_job_executor = single_job_executor;
		m_jobs_begin = jobs_begin;
		m_job_size = job_size;
		m_job_index = jobs_count;
		m_consumers_pending = static_cast<int>(m_threads.size());
	}
	m_consumers_cv.notify_all();
	{
		std::unique_lock<std::mutex> lck_consumers{m_consumers_pending_mtx};
		for(;;)
		{
			if(m_consumers_pending == 0)
			{
				break;
			}
			else
			{
				m_producer_cv.wait(lck_consumers);
			}
		}
	}
}

void mk::detail::parallel_jobs_executor_impl_t::thread_proc()
{
	for(;;)
	{
		void* job;
		{
			std::unique_lock<std::mutex> lck_jobs{m_jobs_mtx};
			for(;;)
			{
				if(m_job_index != 0)
				{
					job = static_cast<char*>(m_jobs_begin) + (m_job_index - 1) * m_job_size;
					--m_job_index;
					break;
				}
				else
				{
					if(m_job_size == s_stop_thread_job_size)
					{
						return;
					}
					else
					{
						bool wake_up_producer;
						{
							std::lock_guard<std::mutex> const lck_consumers{m_consumers_pending_mtx};
							--m_consumers_pending;
							wake_up_producer = m_consumers_pending == 0;
						}
						if(wake_up_producer)
						{
							m_producer_cv.notify_one();
						}
						m_consumers_cv.wait(lck_jobs);
					}
				}
			}
		}
		m_single_job_executor(m_context, job);
	}
}
