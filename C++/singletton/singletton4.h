#pragma once
#ifndef mk_singletton4_included
#define mk_singletton4_included


namespace mk
{

	// Uses spinlock and placement new in static storage.
	class singletton4
	{
	private:
		singletton4();
		singletton4(singletton4 const&) = delete;
		singletton4(singletton4&&) = delete;
		singletton4& operator=(singletton4 const&) = delete;
		singletton4& operator=(singletton4&&) = delete;
		~singletton4();
	public:
		static singletton4& get_instance();
	};

}


#include "singletton4.inl"


#endif
