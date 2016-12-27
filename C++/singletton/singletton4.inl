#ifdef DEBUG
#include <iostream>
#endif


inline ::mk::singletton4::singletton4()
{
#ifdef DEBUG
	::std::cout << "::mk::singletton4::singletton4()\n";
#endif
}

inline ::mk::singletton4::~singletton4()
{
#ifdef DEBUG
	::std::cout << "::mk::singletton4::~singletton4()\n";
#endif
}
