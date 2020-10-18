#pragma once


#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>


namespace mk
{
	namespace detail
	{

		class parallel_jobs_executor_impl_t
		{
		public:
			typedef void(*single_job_executor_t)(void* const context, void* const job);
		public:
			parallel_jobs_executor_impl_t();
			parallel_jobs_executor_impl_t(parallel_jobs_executor_impl_t const&) = delete;
			parallel_jobs_executor_impl_t(parallel_jobs_executor_impl_t&&) noexcept = delete;
			parallel_jobs_executor_impl_t& operator=(parallel_jobs_executor_impl_t const&) = delete;
			parallel_jobs_executor_impl_t& operator=(parallel_jobs_executor_impl_t&&) noexcept = delete;
			~parallel_jobs_executor_impl_t();
			void submit_jobs_and_wait_for_completion(void* const& context, void* const& jobs_begin, int const& jobs_count, int const& job_size, single_job_executor_t const& single_job_executor);
		private:
			void thread_proc();
		private:
			void* m_context;
			single_job_executor_t m_single_job_executor;
			void* m_jobs_begin;
			int m_job_size;
			int m_job_index;
			std::mutex m_jobs_mtx;
			std::vector<std::thread> m_threads;
			std::condition_variable m_consumers_cv;
			std::condition_variable m_producer_cv;
			std::mutex m_consumers_pending_mtx;
			int m_consumers_pending;
		};

	}
}
