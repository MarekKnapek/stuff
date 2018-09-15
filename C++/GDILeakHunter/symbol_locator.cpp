#include "setup.h"
#if mk_should_enable == 1
#include "symbol_locator.h"

#include <stdexcept> // std::runtime_error

#include <Windows.h>
#pragma warning(push)
#pragma warning(disable:4091) // warning C4091: 'typedef ': ignored on left of '' when no variable is declared
#define DBGHELP_TRANSLATE_TCHAR
#include <DbgHelp.h>
#pragma warning(pop)


struct mk::symbol_locator::impl
{
	HANDLE m_handle;
};


mk::symbol_locator::symbol_locator() :
	m_impl(std::make_unique<impl>())
{
	HANDLE const hProcess = GetCurrentProcess();
	BOOL const initialized = SymInitializeW(hProcess, NULL, TRUE);
	if(initialized == FALSE)
	{
		DWORD const gle = GetLastError();
		throw std::runtime_error{""};
	}
	m_impl->m_handle = hProcess;
	DWORD const current_options = SymSetOptions(SYMOPT_EXACT_SYMBOLS);
}

mk::symbol_locator::~symbol_locator()
{
	if(m_impl->m_handle != HANDLE{})
	{
		BOOL const cleaned_up = SymCleanup(m_impl->m_handle);
		if(cleaned_up == FALSE)
		{
			DWORD const gle = GetLastError();
		}
	}
}

void* mk::symbol_locator::locate_symbol(wchar_t const* const& symbol_name)
{
	SYMBOL_INFOW symbol{};
	symbol.SizeOfStruct = sizeof(symbol);
	BOOL from_name = SymFromNameW(m_impl->m_handle, symbol_name, &symbol);
	if(from_name == FALSE)
	{
		DWORD const gle = GetLastError();
		return nullptr;
	}
	return reinterpret_cast<void*>(symbol.Address);
}


#endif
