#pragma once
#ifndef mk_singletton1_included
#define mk_singletton1_included


namespace mk
{

	// Uses mutex and new.
	class singletton1
	{
	private:
		singletton1();
		singletton1(singletton1 const&) = delete;
		singletton1(singletton1&&) = delete;
		singletton1& operator=(singletton1 const&) = delete;
		singletton1& operator=(singletton1&&) = delete;
		~singletton1();
	public:
		static singletton1& get_instance();
	};

}


#include "singletton1.inl"


#endif
