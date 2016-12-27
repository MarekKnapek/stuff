#ifdef DEBUG
#include <iostream>
#endif


inline ::mk::singletton2::singletton2()
{
#ifdef DEBUG
	::std::cout << "::mk::singletton2::singletton2()\n";
#endif
}

inline ::mk::singletton2::~singletton2()
{
#ifdef DEBUG
	::std::cout << "::mk::singletton2::~singletton2()\n";
#endif
}
