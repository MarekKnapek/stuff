#pragma once
#ifndef mk_symbol_locator_h_included
#define mk_symbol_locator_h_included
#include "setup.h"
#if mk_should_enable == 1


#include <memory> // std::unique_ptr


namespace mk
{


	class symbol_locator
	{
	public:
		symbol_locator();
		~symbol_locator();
	private:
		symbol_locator(symbol_locator const&) = delete;
		symbol_locator(symbol_locator&&) = delete;
		symbol_locator& operator=(symbol_locator const&) = delete;
		symbol_locator& operator=(symbol_locator&&) = delete;
	public:
		void* locate_symbol(wchar_t const* const& symbol_name);
	private:
		struct impl;
		std::unique_ptr<impl> m_impl;
	};


}


#endif
#endif
