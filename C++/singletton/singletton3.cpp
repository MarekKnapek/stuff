#include "singletton3.h"

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <type_traits>
#include <mutex>


namespace mk
{
	namespace detail
	{
		namespace
		{

			static ::std::mutex s_singletton3_mutex{};
			static ::std::atomic<bool> s_singletton3_initialized{false};
			static ::std::aligned_storage_t<sizeof(::mk::singletton3), alignof(::mk::singletton3)> s_singletton3_instance{};

		}
	}
}


::mk::singletton3& ::mk::singletton3::get_instance()
{
	::mk::singletton3* ret = nullptr;
	bool const read_one = ::mk::detail::s_singletton3_initialized.load(::std::memory_order_acquire);
	if(read_one){
	}else{
		::mk::detail::s_singletton3_mutex.lock();
		bool const read_two = ::mk::detail::s_singletton3_initialized.load(::std::memory_order_relaxed);
		if(read_two){
			::mk::detail::s_singletton3_mutex.unlock();
		}else{
			new(&::mk::detail::s_singletton3_instance)::mk::singletton3;
			::mk::detail::s_singletton3_initialized.store(true, ::std::memory_order_release); // TODO: relaxed?
			::mk::detail::s_singletton3_mutex.unlock();
			::std::atexit([](){
				assert(::mk::detail::s_singletton3_initialized.load(::std::memory_order_acquire));
				::mk::singletton3* const obj = reinterpret_cast<::mk::singletton3*>(&::mk::detail::s_singletton3_instance);
				using ::mk::singletton3;
				obj->~singletton3();
			});
		}
	}
	ret = reinterpret_cast<::mk::singletton3*>(&::mk::detail::s_singletton3_instance);
	assert(ret);
	return *ret;
}
