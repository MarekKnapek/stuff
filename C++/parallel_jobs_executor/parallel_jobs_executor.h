#pragma once


#include <memory>
#include <vector>


namespace mk
{
	namespace detail
	{
		class parallel_jobs_executor_impl_t;
	}
}


namespace mk
{

	class parallel_jobs_executor_t
	{
	public:
		typedef void(*single_job_executor_t)(void* const context, void* const job);
	public:
		parallel_jobs_executor_t();
		parallel_jobs_executor_t(parallel_jobs_executor_t const&) = delete;
		parallel_jobs_executor_t(parallel_jobs_executor_t&&) noexcept;
		parallel_jobs_executor_t& operator=(parallel_jobs_executor_t const&) = delete;
		parallel_jobs_executor_t& operator=(parallel_jobs_executor_t&&) noexcept;
		~parallel_jobs_executor_t() noexcept;
		void submit_jobs_and_wait_for_completion(void* const& context, void* const& jobs_begin, int const& jobs_count, int const& job_size, single_job_executor_t const& single_job_executor);
		template<typename t> void submit_jobs_and_wait_for_completion(void* const& context, std::vector<t>& jobs, single_job_executor_t const& single_job_executor);
		template<typename t> void submit_jobs_and_wait_for_completion(std::vector<t>& jobs, single_job_executor_t const& single_job_executor);
		template<typename t, typename fn_t> void submit_jobs_and_wait_for_completion(std::vector<t>& jobs, fn_t& fn);
		template<typename t, typename fn_t> void submit_jobs_and_wait_for_completion(std::vector<t>& jobs, fn_t const& fn);
	private:
		std::unique_ptr<detail::parallel_jobs_executor_impl_t> m_impl;
	};

}


template<typename t>
void mk::parallel_jobs_executor_t::submit_jobs_and_wait_for_completion(void* const& context, std::vector<t>& jobs, single_job_executor_t const& single_job_executor)
{
	return submit_jobs_and_wait_for_completion(context, jobs.data(), static_cast<int>(jobs.size()), static_cast<int>(sizeof(t)), single_job_executor);
}

template<typename t>
void mk::parallel_jobs_executor_t::submit_jobs_and_wait_for_completion(std::vector<t>& jobs, single_job_executor_t const& single_job_executor)
{
	return submit_jobs_and_wait_for_completion(nullptr, jobs, single_job_executor);
}

template<typename t, typename fn_t>
void mk::parallel_jobs_executor_t::submit_jobs_and_wait_for_completion(std::vector<t>& jobs, fn_t& fn)
{
	static constexpr auto const s_executor = [](void* const context, void* const job)
	{
		fn_t& fn = *static_cast<fn_t*>(context);
		t& job_item = *static_cast<t*>(job);
		fn(job_item);
	};
	return submit_jobs_and_wait_for_completion(&fn, jobs, s_executor);
}

template<typename t, typename fn_t>
void mk::parallel_jobs_executor_t::submit_jobs_and_wait_for_completion(std::vector<t>& jobs, fn_t const& fn)
{
	static constexpr auto const s_executor = [](void* const context, void* const job)
	{
		fn_t const& fn = *static_cast<fn_t const*>(context);
		t& job_item = *static_cast<t*>(job);
		fn(job_item);
	};
	return submit_jobs_and_wait_for_completion(const_cast<fn_t*>(&fn), jobs, s_executor);
}
