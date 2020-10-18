#include "parallel_jobs_executor.h"

#include <cstdio>
#include <random>
#include <vector>


int main()
{
	std::random_device rd;
	std::default_random_engine eng{rd()};
	std::uniform_int_distribution<int> dist{0, 64 * 1024};

	std::vector<int> vec;
	vec.resize(1'000'000);
	for(int& i : vec)
	{
		i = dist(eng);
	}

	std::vector<int> copy = vec;
	mk::parallel_jobs_executor_t executor;
	mk::parallel_jobs_executor_t e = std::move(executor);
	executor = std::move(e);
	std::swap(executor, e);
	std::swap(executor, e);
	executor.submit_jobs_and_wait_for_completion(vec, [](int& job_item){ job_item = job_item * job_item; });

	for(int i = 0; i != static_cast<int>(vec.size()); ++i)
	{
		if(copy[i] * copy[i] != vec[i])
		{
			std::puts("bad");
			return 1;
		}
	}
	std::puts("good");
	return 0;
}
