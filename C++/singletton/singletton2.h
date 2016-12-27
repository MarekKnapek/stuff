#pragma once
#ifndef mk_singletton2_included
#define mk_singletton2_included


namespace mk
{

	// Uses new, multiple instances can exist simultaneously for short period of time.
	class singletton2
	{
	private:
		singletton2();
		singletton2(singletton2 const&) = delete;
		singletton2(singletton2&&) = delete;
		singletton2& operator=(singletton2 const&) = delete;
		singletton2& operator=(singletton2&&) = delete;
		~singletton2();
	public:
		static singletton2& get_instance();
	};

}


#include "singletton2.inl"


#endif
