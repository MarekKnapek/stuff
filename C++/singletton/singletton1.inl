#ifdef DEBUG
#include <iostream>
#endif


inline ::mk::singletton1::singletton1()
{
#ifdef DEBUG
	::std::cout << "::mk::singletton1::singletton1()\n";
#endif
}

inline ::mk::singletton1::~singletton1()
{
#ifdef DEBUG
	::std::cout << "::mk::singletton1::~singletton1()\n";
#endif
}
