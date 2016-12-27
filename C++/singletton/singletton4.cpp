#include "singletton4.h"

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <type_traits>


namespace mk
{
	namespace detail
	{
		namespace
		{

			static ::std::atomic_flag s_singletton4_spin_mutex = ATOMIC_FLAG_INIT;
			static ::std::atomic<bool> s_singletton4_initialized{false};
			static ::std::aligned_storage_t<sizeof(::mk::singletton4), alignof(::mk::singletton4)> s_singletton4_instance{};

		}
	}
}


::mk::singletton4& ::mk::singletton4::get_instance()
{
	::mk::singletton4* ret = nullptr;
	bool const read_one = ::mk::detail::s_singletton4_initialized.load(::std::memory_order_acquire);
	if(read_one){
	}else{
		while(::mk::detail::s_singletton4_spin_mutex.test_and_set(::std::memory_order_acquire)){}
		bool const read_two = ::mk::detail::s_singletton4_initialized.load(::std::memory_order_relaxed);
		if(read_two){
			::mk::detail::s_singletton4_spin_mutex.clear(::std::memory_order_release);
		}else{
			new(&::mk::detail::s_singletton4_instance)::mk::singletton4;
			::mk::detail::s_singletton4_initialized.store(true, ::std::memory_order_release); // TODO: relaxed?
			::mk::detail::s_singletton4_spin_mutex.clear(::std::memory_order_release);
			::std::atexit([](){
				assert(::mk::detail::s_singletton4_initialized.load(::std::memory_order_acquire));
				::mk::singletton4* const obj = reinterpret_cast<::mk::singletton4*>(&::mk::detail::s_singletton4_instance);
				using ::mk::singletton4;
				obj->~singletton4();
			});
		}
	}
	ret = reinterpret_cast<::mk::singletton4*>(&::mk::detail::s_singletton4_instance);
	assert(ret);
	return *ret;
}
