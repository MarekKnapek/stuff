#include "mk_type_eraser_tests.h"

#include "mk_type_eraser.h"

#include <cassert>
#include <memory>
#include <type_traits>


#define ASSERT(X) do{ if(!(X)){ assert((X)); throw 42; } }while(false)


int mk_some_func(int a, int b)
{
	return a + b;
}

struct mk_some_struct_1
{
	int m_state_1;
	int m_state_2;
	int operator()(int a, int b)
	{
		return m_state_1 + a + b;
	}
};

struct mk_some_struct_2
{
	int m_state_1;
	int m_state_2;
	int operator()(int a, int b)
	{
		return m_state_2 + a + b;
	}
};

struct mk_some_struct_large_1 : mk_some_struct_1
{
	int m_state_large[14];
};

struct mk_some_struct_large_2 : mk_some_struct_2
{
	int m_state_large[14];
};


int mk_type_eraser_test_func()
{
	{
		mk_some_struct_1 a;
		a.m_state_1 = 1;
		a.m_state_2 = 2;
		auto aa = mk::make_type_eraser<int(int, int)>(a);
		
		auto aaa_1 = aa(5, 6);
		auto bb{std::move(aa)};
		auto bbb_2 = bb(11, 12);

		ASSERT(aaa_1 == 12);
		ASSERT(bbb_2 == 24);
	}
	{
		mk_some_struct_large_1 a;
		a.m_state_1 = 1;
		a.m_state_2 = 2;
		auto aa = mk::make_type_eraser<int(int, int)>(a);

		auto aaa_1 = aa(5, 6);
		auto bb{std::move(aa)};
		auto bbb_2 = bb(11, 12);

		ASSERT(aaa_1 == 12);
		ASSERT(bbb_2 == 24);
	}
	{
		mk_some_struct_1 a;
		a.m_state_1 = 1;
		a.m_state_2 = 2;
		auto aa = mk::make_type_eraser<int(int, int)>(a);
		auto bb{std::move(aa)};
		aa = std::move(bb);

		auto aaa_1 = aa(5, 6);
		swap(aa, bb);
		auto bbb_2 = bb(11, 12);

		ASSERT(aaa_1 == 12);
		ASSERT(bbb_2 == 24);
	}
	{
		mk_some_struct_large_1 a;
		a.m_state_1 = 1;
		a.m_state_2 = 2;
		auto aa = mk::make_type_eraser<int(int, int)>(a);
		auto bb{std::move(aa)};
		aa = std::move(bb);

		auto aaa_1 = aa(5, 6);
		swap(aa, bb);
		auto bbb_2 = bb(11, 12);

		ASSERT(aaa_1 == 12);
		ASSERT(bbb_2 == 24);
	}
	{
		mk_some_struct_1 a;
		a.m_state_1 = 1;
		a.m_state_2 = 2;
		mk_some_struct_2 b;
		b.m_state_1 = 3;
		b.m_state_2 = 4;
		auto aa = mk::make_type_eraser<int(int, int)>(a);
		auto bb = mk::make_type_eraser<int(int, int)>(b);

		auto aaa_1 = aa(5, 6);
		auto bbb_1 = bb(7, 8);
		swap(aa, bb);
		auto aaa_2 = aa(9, 10);
		auto bbb_2 = bb(11, 12);

		ASSERT(aaa_1 == 12);
		ASSERT(bbb_1 == 19);
		ASSERT(aaa_2 == 23);
		ASSERT(bbb_2 == 24);
	}
	{
		mk_some_struct_large_1 a;
		a.m_state_1 = 1;
		a.m_state_2 = 2;
		mk_some_struct_2 b;
		b.m_state_1 = 3;
		b.m_state_2 = 4;
		auto aa = mk::make_type_eraser<int(int, int)>(a);
		auto bb = mk::make_type_eraser<int(int, int)>(b);

		auto aaa_1 = aa(5, 6);
		auto bbb_1 = bb(7, 8);
		swap(aa, bb);
		auto aaa_2 = aa(9, 10);
		auto bbb_2 = bb(11, 12);

		ASSERT(aaa_1 == 12);
		ASSERT(bbb_1 == 19);
		ASSERT(aaa_2 == 23);
		ASSERT(bbb_2 == 24);
	}
	{
		mk_some_struct_1 a;
		a.m_state_1 = 1;
		a.m_state_2 = 2;
		mk_some_struct_large_2 b;
		b.m_state_1 = 3;
		b.m_state_2 = 4;
		auto aa = mk::make_type_eraser<int(int, int)>(a);
		auto bb = mk::make_type_eraser<int(int, int)>(b);

		auto aaa_1 = aa(5, 6);
		auto bbb_1 = bb(7, 8);
		swap(aa, bb);
		auto aaa_2 = aa(9, 10);
		auto bbb_2 = bb(11, 12);

		ASSERT(aaa_1 == 12);
		ASSERT(bbb_1 == 19);
		ASSERT(aaa_2 == 23);
		ASSERT(bbb_2 == 24);
	}
	{
		mk_some_struct_large_1 a;
		a.m_state_1 = 1;
		a.m_state_2 = 2;
		mk_some_struct_large_2 b;
		b.m_state_1 = 3;
		b.m_state_2 = 4;
		auto aa = mk::make_type_eraser<int(int, int)>(a);
		auto bb = mk::make_type_eraser<int(int, int)>(b);

		auto aaa_1 = aa(5, 6);
		auto bbb_1 = bb(7, 8);
		swap(aa, bb);
		auto aaa_2 = aa(9, 10);
		auto bbb_2 = bb(11, 12);

		ASSERT(aaa_1 == 12);
		ASSERT(bbb_1 == 19);
		ASSERT(aaa_2 == 23);
		ASSERT(bbb_2 == 24);
	}
	mk_type_eraser_constructible_tests();
	mk_type_eraser_tests_return_reference();
	return 0;
}

static_assert(mk::detail::is_invocable_t<void(), void(*)()>::value, "");
static_assert(mk::detail::is_invocable_t<int(int, int), int(*)(int, int)>::value, "");
static_assert(!mk::detail::is_invocable_t<int(int, int), int(*)(int)>::value, "");

class non_default_constructible_parameter_t
{
private:
	non_default_constructible_parameter_t() = delete;
};
static_assert(mk::detail::is_invocable_t<void(non_default_constructible_parameter_t), void(*)(non_default_constructible_parameter_t)>::value, "");

class non_default_constructible_invocable_t
{
private:
	non_default_constructible_invocable_t() = delete;
public:
	int operator()(int, int);
};
static_assert(mk::detail::is_invocable_t<int(int, int), non_default_constructible_invocable_t>::value, "");

void mk_type_eraser_constructible_tests()
{
	static constexpr auto const s_func_1 = [](int a, int b) -> int { return 42 + a + b; };
	static constexpr auto const s_func_2 = [](int a, int b) -> int { return 43 + a + b; };
	{
		int res_1 = s_func_1(3, 4);
		ASSERT(res_1 == 42 + 3 + 4);
		int res_2 = s_func_2(3, 4);
		ASSERT(res_2 == 43 + 3 + 4);
	}
	{
		static_assert(std::is_constructible_v<mk::type_eraser_t<int(int, int)>, int(*)(int, int)>, "");
		static_assert(std::is_constructible_v<mk::type_eraser_t<int(int, int)>, long long(*)(int, int)>, "");
		static_assert(std::is_constructible_v<mk::type_eraser_t<long long(int, int)>, int(*)(int, int)>, "");
		static_assert(!std::is_constructible_v<mk::type_eraser_t<int(int, int)>, int(*)(int)>, "");
		static_assert(!std::is_constructible_v<mk::type_eraser_t<int(int, int)>, void(*)(int, int)>, "");
		mk::type_eraser_t<int(int, int)> type_earsed_func_1{s_func_1};
		mk::type_eraser_t<int(int, int)> type_earsed_func_2{s_func_2};
		int res_1 = type_earsed_func_1(3, 4);
		ASSERT(res_1 == 42 + 3 + 4);
		int res_2 = type_earsed_func_2(3, 4);
		ASSERT(res_2 == 43 + 3 + 4);
	}
}


class mk_abstract_class
{
public:
	virtual int operator()(int, int) = 0;
};

mk_abstract_class& func_returning_abstract_ref()
{
	mk_abstract_class* ptr = nullptr;
	return *ptr;
}

void mk_type_eraser_tests_return_reference()
{
	mk_abstract_class& ref_1 = func_returning_abstract_ref();

	mk::type_eraser_t<mk_abstract_class&()> eraser_1{func_returning_abstract_ref};
	mk_abstract_class& ref_2 = eraser_1();
}
