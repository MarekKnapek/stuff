#include "fast_dynamic_cast.h"

#include <cassert> // assert
#include <cstdlib> // abort
#include <chrono>
#include <iostream>


#define FAST_DYNAMIC_CAST_ASSERT(X) do{ assert(X); ::mk::mkassert(X); } while(false)


namespace mk
{

	static void dynamic_cast_tests();
	static void mkassert(bool const exp);

	namespace detail
	{

		static int s_asserts_performed = 0;

		struct Base_____
		{
			virtual ~Base_____() = default;
		};

		struct Derived__ : public Base_____
		{
		};

		struct Unrelated
		{
			virtual ~Unrelated() = default;
		};

	}
}


void ::mk::dynamic_cast_tests()
{
	static const unsigned rep1 = 2;
	static const unsigned rep2 = 5000;

	::mk::detail::Base_____ base_____;
	::mk::detail::Derived__ derived__;
	::mk::detail::Unrelated unrelated;

	::mk::detail::Base_____* pbase_________ = &base_____;
	::mk::detail::Derived__* pderived______ = &derived__;
	::mk::detail::Base_____* pderivedasbase = &derived__;
	::mk::detail::Unrelated* punrelated____ = &unrelated;

	auto const& begin1 = ::std::chrono::high_resolution_clock::now();

	for(unsigned b = 0; b != rep2; ++b){
		for(unsigned a = 0; a != rep1; ++a){ auto const r = dynamic_cast<::mk::detail::Base_____*>(pbase_________); FAST_DYNAMIC_CAST_ASSERT(r == pbase_________); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = dynamic_cast<::mk::detail::Derived__*>(pbase_________); FAST_DYNAMIC_CAST_ASSERT(r == nullptr       ); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = dynamic_cast<::mk::detail::Unrelated*>(pbase_________); FAST_DYNAMIC_CAST_ASSERT(r == nullptr       ); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = dynamic_cast<::mk::detail::Base_____*>(pderived______); FAST_DYNAMIC_CAST_ASSERT(r == pderived______); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = dynamic_cast<::mk::detail::Derived__*>(pderived______); FAST_DYNAMIC_CAST_ASSERT(r == pderived______); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = dynamic_cast<::mk::detail::Unrelated*>(pderived______); FAST_DYNAMIC_CAST_ASSERT(r == nullptr       ); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = dynamic_cast<::mk::detail::Base_____*>(pderivedasbase); FAST_DYNAMIC_CAST_ASSERT(r == pderivedasbase); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = dynamic_cast<::mk::detail::Derived__*>(pderivedasbase); FAST_DYNAMIC_CAST_ASSERT(r == pderivedasbase); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = dynamic_cast<::mk::detail::Unrelated*>(pderivedasbase); FAST_DYNAMIC_CAST_ASSERT(r == nullptr       ); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = dynamic_cast<::mk::detail::Base_____*>(punrelated____); FAST_DYNAMIC_CAST_ASSERT(r == nullptr       ); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = dynamic_cast<::mk::detail::Derived__*>(punrelated____); FAST_DYNAMIC_CAST_ASSERT(r == nullptr       ); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = dynamic_cast<::mk::detail::Unrelated*>(punrelated____); FAST_DYNAMIC_CAST_ASSERT(r == punrelated____); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = dynamic_cast<::mk::detail::Base_____&>(*pbase_________); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == pbase_________); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == false); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = dynamic_cast<::mk::detail::Derived__&>(*pbase_________); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == nullptr       ); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == true ); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = dynamic_cast<::mk::detail::Unrelated&>(*pbase_________); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == nullptr       ); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == true ); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = dynamic_cast<::mk::detail::Base_____&>(*pderived______); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == pderived______); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == false); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = dynamic_cast<::mk::detail::Derived__&>(*pderived______); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == pderived______); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == false); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = dynamic_cast<::mk::detail::Unrelated&>(*pderived______); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == nullptr       ); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == true ); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = dynamic_cast<::mk::detail::Base_____&>(*pderivedasbase); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == pderivedasbase); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == false); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = dynamic_cast<::mk::detail::Derived__&>(*pderivedasbase); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == pderivedasbase); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == false); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = dynamic_cast<::mk::detail::Unrelated&>(*pderivedasbase); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == nullptr       ); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == true ); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = dynamic_cast<::mk::detail::Base_____&>(*punrelated____); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == nullptr       ); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == true ); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = dynamic_cast<::mk::detail::Derived__&>(*punrelated____); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == nullptr       ); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == true ); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = dynamic_cast<::mk::detail::Unrelated&>(*punrelated____); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == punrelated____); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == false); }
	}

	FAST_DYNAMIC_CAST_ASSERT(::mk::detail::s_asserts_performed == 30 * rep1 * rep2);

	auto const& end1 = ::std::chrono::high_resolution_clock::now();


	auto const& begin2 = ::std::chrono::high_resolution_clock::now();

	for(unsigned b = 0; b != rep2; ++b){
		for(unsigned a = 0; a != rep1; ++a){ auto const r = ::mk::fast_dynamic_cast<::mk::detail::Base_____*>(pbase_________); FAST_DYNAMIC_CAST_ASSERT(r == pbase_________); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = ::mk::fast_dynamic_cast<::mk::detail::Derived__*>(pbase_________); FAST_DYNAMIC_CAST_ASSERT(r == nullptr       ); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = ::mk::fast_dynamic_cast<::mk::detail::Unrelated*>(pbase_________); FAST_DYNAMIC_CAST_ASSERT(r == nullptr       ); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = ::mk::fast_dynamic_cast<::mk::detail::Base_____*>(pderived______); FAST_DYNAMIC_CAST_ASSERT(r == pderived______); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = ::mk::fast_dynamic_cast<::mk::detail::Derived__*>(pderived______); FAST_DYNAMIC_CAST_ASSERT(r == pderived______); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = ::mk::fast_dynamic_cast<::mk::detail::Unrelated*>(pderived______); FAST_DYNAMIC_CAST_ASSERT(r == nullptr       ); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = ::mk::fast_dynamic_cast<::mk::detail::Base_____*>(pderivedasbase); FAST_DYNAMIC_CAST_ASSERT(r == pderivedasbase); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = ::mk::fast_dynamic_cast<::mk::detail::Derived__*>(pderivedasbase); FAST_DYNAMIC_CAST_ASSERT(r == pderivedasbase); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = ::mk::fast_dynamic_cast<::mk::detail::Unrelated*>(pderivedasbase); FAST_DYNAMIC_CAST_ASSERT(r == nullptr       ); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = ::mk::fast_dynamic_cast<::mk::detail::Base_____*>(punrelated____); FAST_DYNAMIC_CAST_ASSERT(r == nullptr       ); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = ::mk::fast_dynamic_cast<::mk::detail::Derived__*>(punrelated____); FAST_DYNAMIC_CAST_ASSERT(r == nullptr       ); }
		for(unsigned a = 0; a != rep1; ++a){ auto const r = ::mk::fast_dynamic_cast<::mk::detail::Unrelated*>(punrelated____); FAST_DYNAMIC_CAST_ASSERT(r == punrelated____); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = ::mk::fast_dynamic_cast<::mk::detail::Base_____&>(*pbase_________); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == pbase_________); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == false); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = ::mk::fast_dynamic_cast<::mk::detail::Derived__&>(*pbase_________); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == nullptr       ); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == true ); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = ::mk::fast_dynamic_cast<::mk::detail::Unrelated&>(*pbase_________); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == nullptr       ); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == true ); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = ::mk::fast_dynamic_cast<::mk::detail::Base_____&>(*pderived______); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == pderived______); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == false); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = ::mk::fast_dynamic_cast<::mk::detail::Derived__&>(*pderived______); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == pderived______); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == false); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = ::mk::fast_dynamic_cast<::mk::detail::Unrelated&>(*pderived______); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == nullptr       ); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == true ); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = ::mk::fast_dynamic_cast<::mk::detail::Base_____&>(*pderivedasbase); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == pderivedasbase); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == false); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = ::mk::fast_dynamic_cast<::mk::detail::Derived__&>(*pderivedasbase); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == pderivedasbase); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == false); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = ::mk::fast_dynamic_cast<::mk::detail::Unrelated&>(*pderivedasbase); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == nullptr       ); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == true ); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = ::mk::fast_dynamic_cast<::mk::detail::Base_____&>(*punrelated____); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == nullptr       ); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == true ); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = ::mk::fast_dynamic_cast<::mk::detail::Derived__&>(*punrelated____); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == nullptr       ); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == true ); }
		for(unsigned a = 0; a != rep1; ++a){ bool exp; try { auto const& r = ::mk::fast_dynamic_cast<::mk::detail::Unrelated&>(*punrelated____); exp = false; FAST_DYNAMIC_CAST_ASSERT(&r == punrelated____); }catch(::std::bad_cast const&){ exp = true; } FAST_DYNAMIC_CAST_ASSERT(exp == false); }
	}

	FAST_DYNAMIC_CAST_ASSERT(::mk::detail::s_asserts_performed == 30 * rep1 * rep2 * 2 + 1);

	auto const& end2 = ::std::chrono::high_resolution_clock::now();

	::std::cout << "dynamic_cast:      " << ::std::chrono::duration_cast<::std::chrono::milliseconds>(end1 - begin1).count() << " ms." << ::std::endl;
	::std::cout << "fast_dynamic_cast: " << ::std::chrono::duration_cast<::std::chrono::milliseconds>(end2 - begin2).count() << " ms." << ::std::endl;
}

static void ::mk::mkassert(bool const exp)
{
	++::mk::detail::s_asserts_performed;
	if(!exp){
		::std::abort();
	}
}


int main()
{
	::mk::dynamic_cast_tests();
	return 0;
}
