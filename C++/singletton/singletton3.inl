#ifdef DEBUG
#include <iostream>
#endif


inline ::mk::singletton3::singletton3()
{
#ifdef DEBUG
	::std::cout << "::mk::singletton3::singletton3()\n";
#endif
}

inline ::mk::singletton3::~singletton3()
{
#ifdef DEBUG
	::std::cout << "::mk::singletton3::~singletton3()\n";
#endif
}
