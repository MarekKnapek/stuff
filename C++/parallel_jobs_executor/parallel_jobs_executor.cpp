#include "parallel_jobs_executor.h"

#include "parallel_jobs_executor_impl.h"

#include <cassert>


mk::parallel_jobs_executor_t::parallel_jobs_executor_t() :
	m_impl(std::make_unique<detail::parallel_jobs_executor_impl_t>())
{
}

mk::parallel_jobs_executor_t::parallel_jobs_executor_t(parallel_jobs_executor_t&&) noexcept = default;

mk::parallel_jobs_executor_t& mk::parallel_jobs_executor_t::operator=(parallel_jobs_executor_t&&) noexcept = default;

mk::parallel_jobs_executor_t::~parallel_jobs_executor_t() noexcept = default;

void mk::parallel_jobs_executor_t::submit_jobs_and_wait_for_completion(void* const& context, void* const& jobs_begin, int const& jobs_count, int const& job_size, single_job_executor_t const& single_job_executor)
{
	assert(m_impl);
	return m_impl->submit_jobs_and_wait_for_completion(context, jobs_begin, jobs_count, job_size, single_job_executor);
}
