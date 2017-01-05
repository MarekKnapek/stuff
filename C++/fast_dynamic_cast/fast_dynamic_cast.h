#pragma once
#ifndef MK_FAST_DYNAMIC_CAST_INCLUDED
#define MK_FAST_DYNAMIC_CAST_INCLUDED

// Inspired by https://github.com/tobspr/FastDynamicCast

#include <cstdint>
#include <limits>
#include <memory> // addressof
#include <type_traits>
#include <typeinfo> // bad_cast, type_info
#ifdef FAST_DYNAMIC_CAST_DEBUG
#include <iostream>
#endif


/*

B->B cast
B->D dynamic_cast
B->U nullptr
D->B cast
D->D cast
D->U nullptr
U->B nullptr
U->D nullptr
U->U cast

cast         -> iff is_base_of<*T, F>
dynamic_cast -> iff is_base_of<F, *T> && !is_same<F, *T>
nullptr      -> iif !is_base_of<*T, F> && !is_base_of<F, *T>

*/

namespace mk
{
	namespace detail
	{

		static const ::std::ptrdiff_t s_failed_cast = ::std::numeric_limits<::std::ptrdiff_t>::max() / 2;
		static const unsigned s_cache_size = 4;

		struct cache_elem
		{
			::std::uintptr_t m_vptr = 0;
			::std::ptrdiff_t m_offset;
		};
		struct cache
		{
			cache_elem m_elems[::mk::detail::s_cache_size];
			unsigned m_index = 0;
		};

	}

	// cast
	template<typename T, typename F>
	inline
	::std::enable_if_t<::std::is_base_of_v<::std::remove_pointer_t<T>, F>, T>
	fast_dynamic_cast(F* pf)
	{
		static_assert(::std::is_pointer_v<T>, "");

	#ifdef FAST_DYNAMIC_CAST_DEBUG
		::std::cout << "F = " << typeid(F).name() << " ";
		::std::cout << "T = " << typeid(::std::remove_pointer_t<T>).name() << " ";
		::std::cout << "cast";
		::std::cout << ::std::endl;
	#endif

		return pf;
	}

	// dynamic_cast
	template<typename T, typename F>
	inline
	::std::enable_if_t<::std::conjunction_v<::std::is_base_of<F, ::std::remove_pointer_t<T>>, ::std::negation<::std::is_same<F, ::std::remove_pointer_t<T>>>>, T>
	fast_dynamic_cast(F* pf)
	{
		static_assert(::std::is_pointer_v<T>, "");

	#ifdef FAST_DYNAMIC_CAST_DEBUG
		::std::cout << "F = " << typeid(F).name() << " ";
		::std::cout << "T = " << typeid(::std::remove_pointer_t<T>).name() << " ";
		::std::cout << "dynamic_cast";
		::std::cout << ::std::endl;
	#endif

		static thread_local ::mk::detail::cache s_cache;

		::std::uintptr_t const vptr = *reinterpret_cast<::std::uintptr_t const*>(pf);
		for(unsigned i = 0; i != ::mk::detail::s_cache_size; ++i){
			if(s_cache.m_elems[i].m_vptr == vptr){
				if(s_cache.m_elems[i].m_offset == ::mk::detail::s_failed_cast){
					return nullptr;
				}else{
					return reinterpret_cast<T>(const_cast<char*>(reinterpret_cast<char const*>(pf)) + s_cache.m_elems[i].m_offset);
				}
			}
		}
		T const pt = dynamic_cast<T>(pf);
		if(pt){
			s_cache.m_elems[s_cache.m_index].m_vptr = vptr;
			s_cache.m_elems[s_cache.m_index].m_offset = reinterpret_cast<char const*>(pt) - reinterpret_cast<char const*>(pf);
		}else{
			s_cache.m_elems[s_cache.m_index].m_vptr = vptr;
			s_cache.m_elems[s_cache.m_index].m_offset = ::mk::detail::s_failed_cast;
		}
		s_cache.m_index = (s_cache.m_index + 1) % ::mk::detail::s_cache_size;
		return pt;
	}

	// nullptr
	template<typename T, typename F>
	inline
	::std::enable_if_t<::std::conjunction_v<::std::negation<::std::is_base_of<::std::remove_pointer_t<T>, F>>, ::std::negation<::std::is_base_of<F, ::std::remove_pointer_t<T>>>>, T>
	fast_dynamic_cast(F* pf)
	{
		static_assert(::std::is_pointer_v<T>, "");

	#ifdef FAST_DYNAMIC_CAST_DEBUG
		::std::cout << "F = " << typeid(F).name() << " ";
		::std::cout << "T = " << typeid(::std::remove_pointer_t<T>).name() << " ";
		::std::cout << "nullptr";
		::std::cout << ::std::endl;
	#endif

		return nullptr;
	}

	// reference
	template<typename T, typename F>
	inline
	T
	fast_dynamic_cast(F& f)
	{
		static_assert(::std::is_reference_v<T>, "");

		auto const& pf = ::mk::fast_dynamic_cast<::std::add_pointer_t<::std::remove_reference_t<T>>>(::std::addressof(f));
		if(!pf){
			throw ::std::bad_cast{};
		}
		return *pf;
	}

}


#endif
