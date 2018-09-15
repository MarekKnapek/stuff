#include "setup.h"
#if mk_should_enable == 1
#include "stack_tracer.h"

#include <algorithm>
#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <unordered_set>

#include <Windows.h>
#include <Psapi.h>
#pragma warning(push)
#pragma warning(disable:4091) // warning C4091: 'typedef ': ignored on left of '' when no variable is declared
#ifndef UNICODE
#define UNICODE
#endif
#define _IMAGEHLP64
#define DBGHELP_TRANSLATE_TCHAR
#include <Dbghelp.h>
#pragma warning(pop)

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "Psapi.lib")

#ifdef assert
#undef assert
#endif

#define assert(X) mk::detail::fn_assert(#X, !!(X))

#define STRING_JOIN2(A, B) DO_STRING_JOIN2(A, B)
#define DO_STRING_JOIN2(A, B) A ## B
#define SCOPE_EXIT(code) auto STRING_JOIN2(scope_exit_, __LINE__) = make_scope_exit([&](){ code; })

namespace mk
{
	namespace detail
	{

		typedef USHORT(WINAPI * capture_stack_back_trace_fn_t)(ULONG, ULONG, PVOID*, PULONG);

		struct tracer_task
		{
			void** m_buff;
			unsigned m_frames;
			::std::atomic<::std::wstring*>* m_out;
		};
		bool operator==(tracer_task const& a, tracer_task const& b){ return ::std::tie(a.m_buff, a.m_frames, a.m_out) == ::std::tie(b.m_buff, b.m_frames, b.m_out); }
		bool operator!=(tracer_task const& a, tracer_task const& b){ return !(a == b); }

		void fn_assert(char const* const& str, bool const& b);

		static bool s_enable_asserts = true;
		static tracer_task s_end_task = { (void**)0x1, 0x2, (::std::atomic<::std::wstring*>*)0x3 };

		static mk::stack_tracer* s_tracer_ptr = nullptr;
		static ::std::mutex s_tracer_mtx{};

	}
}

void mk::detail::fn_assert(char const* const& str, bool const& b)
{
	if(s_enable_asserts){
		if(!b){
			//int const msgbox_ret = ::MessageBoxA(0, str, "StackTrace assert", 0);
		}
	}
}

template<typename F>
class scope_exit
{
public:
	scope_exit(F const& f) :
		m_f(f)
	{
	}
	~scope_exit()
	{
		m_f();
	}
private:
	F const& m_f;
};

template<typename F>
scope_exit<F> make_scope_exit(F const& f)
{
	return scope_exit<F>(f);
}

class mk::stack_tracer::stack_tracer_impl
{
public:
	stack_tracer_impl(stack_tracer* p_parent);
	~stack_tracer_impl();
public:
	void translate_trace_async(void** const& buff, unsigned const& frames, ::std::atomic<::std::wstring*>& out);
private:
	void thread_func();
	void name_thread();
	bool thread_loop();
	void process_task(mk::detail::tracer_task& task);
	void init_dbghlp();
	void close_dbghlp();
	void load_all_modules();
	::std::wstring get_error_msg(DWORD const& gle);
	::std::wstring get_last_error_msg();
	::std::wstring strip_newlines(::std::wstring const& str);
	::std::wstring strip_newlines(::std::wstring&& str);
public:
	mk::detail::capture_stack_back_trace_fn_t m_capture_fn;
private:
	stack_tracer& m_parent;
	HANDLE m_h_process;
	::std::unordered_set<HMODULE> m_modules;
	::std::vector<mk::detail::tracer_task> m_queue;
	::std::vector<mk::detail::tracer_task> m_second_queue;
	::std::mutex m_mutex;
	::std::condition_variable m_cv;
	::std::thread m_thread;
};

mk::stack_tracer::stack_tracer_impl::stack_tracer_impl(mk::stack_tracer* p_parent) :
	m_capture_fn(nullptr),
	m_parent(*p_parent),
	m_h_process(),
	m_queue(),
	m_second_queue(),
	m_mutex(),
	m_cv(),
	m_thread()
{
	assert(p_parent != nullptr);

	HMODULE const& kernel32_hmodule = ::GetModuleHandleW(L"kernel32.dll");
	if(kernel32_hmodule == NULL){
		auto const& err = get_last_error_msg();
		assert(kernel32_hmodule != NULL);
	}
	FARPROC const& capture_fn = ::GetProcAddress(kernel32_hmodule, "RtlCaptureStackBackTrace");
	if(capture_fn == NULL){
		auto const& err = get_last_error_msg();
		assert(capture_fn != NULL);
	}
	m_capture_fn = reinterpret_cast<mk::detail::capture_stack_back_trace_fn_t>(capture_fn);
	assert(m_capture_fn);

	m_thread = ::std::thread(&stack_tracer_impl::thread_func, this);
}

mk::stack_tracer::stack_tracer_impl::~stack_tracer_impl()
{
	translate_trace_async(mk::detail::s_end_task.m_buff, mk::detail::s_end_task.m_frames, *mk::detail::s_end_task.m_out);
	m_thread.join();
}

void mk::stack_tracer::stack_tracer_impl::translate_trace_async(void** const& buff, unsigned const& frames, ::std::atomic<::std::wstring*>& out)
{
	assert(buff);
	assert(frames);

	mk::detail::tracer_task task;
	task.m_buff = buff;
	task.m_frames = frames;
	task.m_out = &out;

	{
		::std::lock_guard<::std::mutex> lg(m_mutex);
		m_queue.push_back(task);
	}
	m_cv.notify_one();
}

void mk::stack_tracer::stack_tracer_impl::thread_func()
{
	name_thread();
	init_dbghlp();
	while(thread_loop());
	close_dbghlp();
}

void mk::stack_tracer::stack_tracer_impl::name_thread()
{
	static DWORD const name_thread_exception_code = 0x406D1388;

	#pragma pack(push,8)
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType;
		LPCSTR szName;
		DWORD dwThreadID;
		DWORD dwFlags;
	} THREADNAME_INFO;
	#pragma pack(pop)

	THREADNAME_INFO info;
	::std::memset(&info, 0, sizeof info);
	info.dwType = 0x1000;
	info.szName = "stack_tracer";
	info.dwThreadID = -1;
	info.dwFlags = 0;

	__try{
		RaiseException(name_thread_exception_code, 0, sizeof(info) / sizeof(ULONG_PTR), reinterpret_cast<ULONG_PTR CONST*>(&info));
	}__except(EXCEPTION_EXECUTE_HANDLER){
	}
}

bool mk::stack_tracer::stack_tracer_impl::thread_loop()
{
	{
		::std::unique_lock<::std::mutex> ul{m_mutex};
		auto& queue = m_queue;
		m_cv.wait(ul, [&queue](){ return !queue.empty(); });
		m_queue.swap(m_second_queue);
	}
	load_all_modules();
	for(auto&& e : m_second_queue){
		if(e == mk::detail::s_end_task){
			return false;
		}else{
			process_task(e);
		}
	}
	m_second_queue.clear();
	return true;
}

void mk::stack_tracer::stack_tracer_impl::process_task(mk::detail::tracer_task& task)
{
	::std::wstring* const out_p_str = task.m_out->exchange(nullptr);
	::std::wstring& out_str = *out_p_str;
	out_str.clear();

	unsigned const max_symbol_name_len_guess = 1000;
	for(unsigned i = 0; i != task.m_frames && task.m_buff[i] != nullptr; ++i){
		DWORD64 displacement = 0;
		unsigned const symbol_size = sizeof(SYMBOL_INFOW) + (max_symbol_name_len_guess - 1) * sizeof(TCHAR);
		alignas(alignof(SYMBOL_INFOW)) unsigned char symbol_buff[symbol_size];
		::std::memset(symbol_buff, 0, symbol_size);
		SYMBOL_INFOW& symbol = *(SYMBOL_INFOW*)symbol_buff;
		symbol.SizeOfStruct = sizeof symbol;
		symbol.MaxNameLen = max_symbol_name_len_guess;
		BOOL const sym_from_addr_ret = ::SymFromAddrW(m_h_process, (DWORD64)task.m_buff[i], &displacement, &symbol);
		if(sym_from_addr_ret == FALSE){
			auto const& err = strip_newlines(get_last_error_msg());
			//out_str += err;
			//assert(("SymFromAddrW failed", false));
		}else{
			out_str += symbol.Name;
		}
		out_str += L" ";

		DWORD displacement2 = 0;
		IMAGEHLP_LINEW64 line;
		::std::memset(&line, 0, sizeof line);
		line.SizeOfStruct = sizeof line;
		BOOL const sym_get_line_from_addr_ret = ::SymGetLineFromAddrW64(m_h_process, (DWORD64)task.m_buff[i], &displacement2, &line);
		if(sym_get_line_from_addr_ret == FALSE){
			auto const& err = strip_newlines(get_last_error_msg());
			//out_str += err;
			//assert(("SymGetLineFromAddrW64 failed", false));
		}else{
			out_str += line.FileName;
			out_str += L" ";
			out_str += ::std::to_wstring(line.LineNumber);
		}

		HMODULE ret_hmodule = NULL;
		BOOL const gmhex_ret = ::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)symbol.Address, &ret_hmodule);
		if(gmhex_ret == 0){
			auto const& err = strip_newlines(get_last_error_msg());
			//assert(("GetModuleHandleEx", false));
		}
		unsigned const max_module_name_len_guess = 512;
		wchar_t buff[max_module_name_len_guess];
		::std::memset(buff, 0, max_module_name_len_guess * sizeof(wchar_t));
		DWORD const& gmfn_ret = ::GetModuleFileNameW(ret_hmodule, buff, max_module_name_len_guess);
		out_str.append(L" ");
		out_str.append(buff, buff + gmfn_ret);

		out_str += L"\n";
	}

	::std::wstring* const out_p_str2 = task.m_out->exchange(out_p_str);
	assert(out_p_str2 == nullptr);
}

void mk::stack_tracer::stack_tracer_impl::init_dbghlp()
{
	assert(m_h_process == HANDLE());
	m_h_process = ::GetCurrentProcess();

	unsigned const file_name_len_guess = 1024;
	TCHAR file_name[file_name_len_guess];
	::std::memset(file_name, 0, sizeof(TCHAR) * file_name_len_guess);
	DWORD const get_module_file_name_ret = ::GetModuleFileNameW(NULL, file_name, (DWORD)file_name_len_guess);
	if(get_module_file_name_ret == (DWORD)file_name_len_guess){
		auto const& err = strip_newlines(get_last_error_msg());
		assert(("buffer too small", false));
	}else if(get_module_file_name_ret == 0){
		auto const& err = strip_newlines(get_last_error_msg());
		assert(("GetModuleFileNameW failed", false));
	}
	for(unsigned i = static_cast<unsigned>(get_module_file_name_ret); i != 0; --i){
		if(file_name[i] == L'/' || file_name[i] == L'\\'){
			file_name[i] = L'\0';
			break;
		}
	}

	DWORD const current_opts = ::SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
	BOOL const initialize = ::SymInitialize(m_h_process, NULL, TRUE);
	if(initialize == FALSE){
		auto const& err = strip_newlines(get_last_error_msg());
		assert(("SymInitializeW failed", false));
	}

	unsigned const max_env_var_len = 32767;
	wchar_t buff[max_env_var_len];
	DWORD const env_var_ret = ::GetEnvironmentVariableW(L"_NT_SYMBOL_PATH", buff, max_env_var_len);
	if(env_var_ret == 0){
		DWORD const gle = ::GetLastError();
		if(gle != ERROR_ENVVAR_NOT_FOUND){
			auto const& err = strip_newlines(get_error_msg(gle));
			assert(false);
		}
	}
	::std::wstring env_var(file_name);
	env_var.append(L";");
	env_var.append(buff, buff + env_var_ret);

	BOOL const sym_set_search_path_ret = ::SymSetSearchPathW(m_h_process, env_var.data());
	if(sym_set_search_path_ret == FALSE){
		auto const& err = strip_newlines(get_last_error_msg());
		assert(("SymSetSearchPathW failed", false));
	}
}

void mk::stack_tracer::stack_tracer_impl::close_dbghlp()
{
	assert(m_h_process != HANDLE());
	BOOL const cleanup = ::SymCleanup(m_h_process);
	if(cleanup == FALSE){
		auto const& err = strip_newlines(get_last_error_msg());
		assert(("SymCleanup failed", false));
	}
	m_h_process = HANDLE();
}

void mk::stack_tracer::stack_tracer_impl::load_all_modules()
{
	unsigned const max_modules_guess = 1024;
	HMODULE modules[max_modules_guess];
	::std::memset(modules, 0, sizeof(HMODULE) * max_modules_guess);
	DWORD out = 0;
	BOOL const enum_ret = ::EnumProcessModules(m_h_process, modules, sizeof(HMODULE) * max_modules_guess, &out);
	if(enum_ret == 0){
		auto const& err = strip_newlines(get_last_error_msg());
		assert(false);
	}
	for(unsigned i = 0; i != out / sizeof(HMODULE); ++i){
		if(m_modules.find(modules[i]) == m_modules.end()){
			unsigned const max_path_len_guess = 1024;
			wchar_t buff[max_path_len_guess];
			::std::memset(buff, 0, sizeof(wchar_t) * max_path_len_guess);
			DWORD const gmfn_ret = GetModuleFileNameW(modules[i], buff, max_path_len_guess - 1);
			if(gmfn_ret == 0){
				auto const& err = strip_newlines(get_last_error_msg());
				assert(false);
			}
			buff[gmfn_ret] = L'\0';
			DWORD64 const slm_ret = SymLoadModuleExW(m_h_process, NULL, buff, NULL, 0, 0, NULL, 0);
			if(slm_ret == 0){
				auto const& err = strip_newlines(get_last_error_msg());
				assert(false);
			}
		}
	}
}

::std::wstring mk::stack_tracer::stack_tracer_impl::get_error_msg(DWORD const& gle)
{
	::std::wstring ret;
	if(gle == 0){
		return ret;
	}
	LPWSTR buff = nullptr;
	DWORD const len = ::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, gle, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&buff, 0, NULL);
	if(len == 0){
		DWORD const gle = ::GetLastError();
		assert(false);
	}
	ret.assign(buff, buff + len);
	HLOCAL const free_ret = ::LocalFree(buff);
	if(free_ret != NULL){
		DWORD const gle = ::GetLastError();
		assert(false);
	}
	return ret;
}

::std::wstring mk::stack_tracer::stack_tracer_impl::get_last_error_msg()
{
	return get_error_msg(::GetLastError());
}

::std::wstring mk::stack_tracer::stack_tracer_impl::strip_newlines(::std::wstring const& str)
{
	::std::wstring ret = str;
	ret.erase(::std::remove_if(ret.begin(), ret.end(), [](auto const& e){ return e == L'\r' || e == L'\n'; }), ret.end());
	return ret;
}

::std::wstring mk::stack_tracer::stack_tracer_impl::strip_newlines(::std::wstring&& str)
{
	str.erase(::std::remove_if(str.begin(), str.end(), [](auto const& e){ return e == L'\r' || e == L'\n'; }), str.end());
	return str;
}

mk::stack_tracer::stack_tracer() :
	m_p_impl(new stack_tracer::stack_tracer_impl{this})
{
}

mk::stack_tracer::~stack_tracer()
{
	delete m_p_impl;
}

mk::stack_tracer& mk::stack_tracer::get_instance()
{
	{
		::std::lock_guard<::std::mutex> lg(mk::detail::s_tracer_mtx);
		if(!mk::detail::s_tracer_ptr){
			mk::detail::s_tracer_ptr = new stack_tracer();
			::std::atexit([](){ delete mk::detail::s_tracer_ptr; });
		}
	}
	return *mk::detail::s_tracer_ptr;
}

void mk::stack_tracer::get_trace(void** const& buff, unsigned const& frames) const
{
	USHORT const capture_stack_back_trace_ret = (*m_p_impl->m_capture_fn)(0, (ULONG)frames, buff, NULL);
	::std::memset(buff + capture_stack_back_trace_ret, 0, sizeof(void*) * (frames - capture_stack_back_trace_ret));
}

void mk::stack_tracer::translate_trace_async(void** const& buff, unsigned const& frames, ::std::atomic<::std::wstring*>& out)
{
	m_p_impl->translate_trace_async(buff, frames, out);
}


#endif
