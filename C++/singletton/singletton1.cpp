#include "singletton1.h"

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <mutex>


namespace mk
{
	namespace detail
	{
		namespace
		{

			static ::std::mutex s_singletton1_mutex{};
			static ::std::atomic<::mk::singletton1*> s_singletton1_instance{nullptr};

		}
	}
}


::mk::singletton1& ::mk::singletton1::get_instance()
{
	::mk::singletton1* ret = nullptr;
	::mk::singletton1* const read_one = ::mk::detail::s_singletton1_instance.load(::std::memory_order_acquire);
	if(read_one){
		ret = read_one;
	}else{
		::mk::detail::s_singletton1_mutex.lock();
		::mk::singletton1* const read_two = ::mk::detail::s_singletton1_instance.load(::std::memory_order_relaxed);
		if(read_two){
			::mk::detail::s_singletton1_mutex.unlock();
			ret = read_two;
		}else{
			::mk::singletton1* const new_obj = new ::mk::singletton1;
			::mk::detail::s_singletton1_instance.store(new_obj, ::std::memory_order_release); // TOOD: relaxed?
			::mk::detail::s_singletton1_mutex.unlock();
			ret = new_obj;
			::std::atexit([](){
				::mk::singletton1* const obj = ::mk::detail::s_singletton1_instance.load(::std::memory_order_acquire);
				assert(obj);
				delete obj;
			});
		}
	}
	assert(ret);
	return *ret;
}
