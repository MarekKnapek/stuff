#pragma once
#ifndef mk_singletton3_included
#define mk_singletton3_included


namespace mk
{

	// Uses mutex and placement new in static storage.
	class singletton3
	{
	private:
		singletton3();
		singletton3(singletton3 const&) = delete;
		singletton3(singletton3&&) = delete;
		singletton3& operator=(singletton3 const&) = delete;
		singletton3& operator=(singletton3&&) = delete;
		~singletton3();
	public:
		static singletton3& get_instance();
	};

}


#include "singletton3.inl"


#endif
