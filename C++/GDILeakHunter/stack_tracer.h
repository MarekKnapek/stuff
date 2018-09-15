#pragma once
#ifndef STACK_TRACER_H_DEFINED
#define STACK_TRACER_H_DEFINED
#include "setup.h"
#if mk_should_enable == 1


#include <string>
#include <atomic>


namespace mk
{

	class stack_tracer
	{
	public:
		stack_tracer();
		~stack_tracer();
	private:
		stack_tracer(stack_tracer const&) = delete;
		stack_tracer(stack_tracer&&) = delete;
		stack_tracer& operator=(stack_tracer const&) = delete;
		stack_tracer& operator=(stack_tracer&&) = delete;
	public:
		static stack_tracer& get_instance();
	public:
		void get_trace(void** const& buff, unsigned const& frames) const;
		void translate_trace_async(void** const& buff, unsigned const& frames, ::std::atomic<::std::wstring*>& out);
	private:
		class stack_tracer_impl;
		stack_tracer_impl* m_p_impl;
	};

}


#endif
#endif
