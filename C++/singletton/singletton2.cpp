#include "singletton2.h"

#include <atomic>
#include <cassert>
#include <cstdlib>


namespace mk
{
	namespace detail
	{
		namespace
		{

			static ::std::atomic<::mk::singletton2*> s_singletton2_instance{nullptr};

		}
	}
}


::mk::singletton2& ::mk::singletton2::get_instance()
{
	::mk::singletton2* ret = nullptr;
	::mk::singletton2* const read_one = ::mk::detail::s_singletton2_instance.load(::std::memory_order_acquire);
	if(read_one){
		ret = read_one;
	}else{
		::mk::singletton2* const new_obj = new ::mk::singletton2;
		::mk::singletton2* read_two = nullptr;
		if(::mk::detail::s_singletton2_instance.compare_exchange_strong(read_two, new_obj, ::std::memory_order_release, ::std::memory_order_acquire)){
			ret = new_obj;
			::std::atexit([](){
				::mk::singletton2* const obj = ::mk::detail::s_singletton2_instance.load(::std::memory_order_acquire);
				assert(obj);
				delete obj;
			});
		}else{
			delete new_obj;
			ret = read_two;
		}
	}
	assert(ret);
	return *ret;
}
