#pragma once
#ifndef MAREK_LIB_OVERRIDER_INCLUDED
#define MAREK_LIB_OVERRIDER_INCLUDED


#include <cstdint> // uint32_t
#include <cstring> // memcpy, memset
#include <cstdlib> // exit


#include <Windows.h>


namespace mk
{

#pragma push_macro("assert")
#ifdef assert
# undef assert
#endif

	void assert(bool expr);

	void override_func(char* orig_func, unsigned n_bytes, char* backup_buff, char* new_func);
	template<typename R, typename T, typename U, typename... Args>
	void override_func(R(T::*orig_func)(Args...), unsigned n_bytes, char* backup_buff, R(U::*new_func)(Args...));
	template<typename R, typename... Args>
	void override_func(R(*orig_func)(Args...), unsigned n_bytes, char* backup_buff, R(*new_func)(Args...));
	template<typename R, typename... Args>
	void override_func(R(__stdcall*orig_func)(Args...), unsigned n_bytes, char* backup_buff, R(__stdcall*new_func)(Args...));
	char* follow_jumps(char* func_ptr);
	template<typename R, typename T, typename... Args>
	char* fn_ptr_to_ptr(R(T::*fn_ptr)(Args...));
	template<typename R, typename... Args>
	char* fn_ptr_to_ptr(R(*fn_ptr)(Args...));
	template<typename R, typename... Args>
	char* fn_ptr_to_ptr(R(__stdcall*fn_ptr)(Args...));
	template<typename R, typename T, typename... Args>
	R call_backup_as(char* backup_buff, R(T::*)(Args...), T& self, Args... args);
	template<typename R, typename... Args>
	R call_backup_as(char* backup_buff, R(*)(Args...), Args... args);
	template<typename R, typename... Args>
	R call_backup_as(char* backup_buff, R(__stdcall*)(Args...), Args... args);

}


void (::mk::assert)(bool expr)
{
	if(!expr){
		::std::exit(EXIT_FAILURE);
	}
}

void (::mk::override_func)(char* orig_func, unsigned n_bytes, char* backup_buff, char* new_func)
{
	::std::memcpy(backup_buff, orig_func, n_bytes);
	backup_buff[n_bytes] = '\xE9';
	::std::uint32_t offset = (orig_func + n_bytes) - (backup_buff + n_bytes + 5);
	::std::memcpy(backup_buff + n_bytes + 1, &offset, 4);

	DWORD orig_perm = 0;
	BOOL const vp_ret1 = ::VirtualProtect(orig_func, n_bytes, PAGE_EXECUTE_READWRITE, &orig_perm);
	orig_func[0] = '\xE9';
	offset = (new_func) - (orig_func + 5);
	::std::memcpy(orig_func + 1, &offset, 4);
	::std::memset(orig_func + 5, '\xCC', n_bytes - 5);
	DWORD dummy = 0;
	BOOL const vp_ret2 = ::VirtualProtect(orig_func, n_bytes, orig_perm, &dummy);
}

template<typename R, typename T, typename U, typename... Args>
void (::mk::override_func)(R(T::*orig_func)(Args...), unsigned n_bytes, char* backup_buff, R(U::*new_func)(Args...))
{
	return ::mk::override_func(::mk::follow_jumps(::mk::fn_ptr_to_ptr(orig_func)), n_bytes, backup_buff, ::mk::follow_jumps(::mk::fn_ptr_to_ptr(new_func)));
}

template<typename R, typename... Args>
void (::mk::override_func)(R(*orig_func)(Args...), unsigned n_bytes, char* backup_buff, R(*new_func)(Args...))
{
	return ::mk::override_func(::mk::follow_jumps(::mk::fn_ptr_to_ptr(orig_func)), n_bytes, backup_buff, ::mk::follow_jumps(::mk::fn_ptr_to_ptr(new_func)));
}

template<typename R, typename... Args>
void (::mk::override_func)(R(__stdcall*orig_func)(Args...), unsigned n_bytes, char* backup_buff, R(__stdcall*new_func)(Args...))
{
	return ::mk::override_func(::mk::follow_jumps(::mk::fn_ptr_to_ptr(orig_func)), n_bytes, backup_buff, ::mk::follow_jumps(::mk::fn_ptr_to_ptr(new_func)));
}

char* (::mk::follow_jumps)(char* func_ptr)
{
	char* ret = func_ptr;
	bool jumped;
	do{
		jumped = false;
		if(ret[0] == '\xE9'){
			jumped = true;
			static_assert(sizeof(char*) == sizeof(::std::uint32_t), "");
			ret += 5 + *reinterpret_cast<::std::uint32_t*>(ret + 1);
		}
		if(ret[0] == '\xFF' && ret[1] == '\x25'){
			jumped = true;
			ret = **reinterpret_cast<char***>(ret + 2);
		}
	}while(jumped);
	return ret;
}

template<typename R, typename T, typename... Args>
char* (::mk::fn_ptr_to_ptr)(R(T::*fn_ptr)(Args...))
{
	static_assert(sizeof(fn_ptr) == sizeof(char*), "");
	return *reinterpret_cast<char**>(&fn_ptr);
}

template<typename R, typename... Args>
char* (::mk::fn_ptr_to_ptr)(R(*fn_ptr)(Args...))
{
	static_assert(sizeof(fn_ptr) == sizeof(char*), "");
	return *reinterpret_cast<char**>(&fn_ptr);
}

template<typename R, typename... Args>
char* (::mk::fn_ptr_to_ptr)(R(__stdcall*fn_ptr)(Args...))
{
	static_assert(sizeof(fn_ptr) == sizeof(char*), "");
	return *reinterpret_cast<char**>(&fn_ptr);
}

template<typename R, typename T, typename... Args>
R (::mk::call_backup_as)(char* backup_buff, R(T::*)(Args...), T& self, Args... args)
{
	static_assert(sizeof(R(T::*)(Args...)) == sizeof(char*), "");
	return (self.*(*reinterpret_cast<R(T::**)(Args...)>(&backup_buff)))(args...);
}

template<typename R, typename... Args>
R (::mk::call_backup_as)(char* backup_buff, R(*)(Args...), Args... args)
{
	static_assert(sizeof(R(*)(Args...)) == sizeof(char*), "");
	return (*reinterpret_cast<R(**)(Args...)>(&backup_buff))(args...);
}

template<typename R, typename... Args>
R (::mk::call_backup_as)(char* backup_buff, R(__stdcall*)(Args...), Args... args)
{
	static_assert(sizeof(R(__stdcall*)(Args...)) == sizeof(char*), "");
	return (*reinterpret_cast<R(__stdcall**)(Args...)>(&backup_buff))(args...);
}

#pragma pop_macro("assert")
#endif
