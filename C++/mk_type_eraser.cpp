#include "../utils/mk_type_eraser.h"
#include "../utils/verify.h"


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

		VERIFY(aaa_1 == 12);
		VERIFY(bbb_2 == 24);
	}
	{
		mk_some_struct_large_1 a;
		a.m_state_1 = 1;
		a.m_state_2 = 2;
		auto aa = mk::make_type_eraser<int(int, int)>(a);

		auto aaa_1 = aa(5, 6);
		auto bb{std::move(aa)};
		auto bbb_2 = bb(11, 12);

		VERIFY(aaa_1 == 12);
		VERIFY(bbb_2 == 24);
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

		VERIFY(aaa_1 == 12);
		VERIFY(bbb_2 == 24);
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

		VERIFY(aaa_1 == 12);
		VERIFY(bbb_2 == 24);
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

		VERIFY(aaa_1 == 12);
		VERIFY(bbb_1 == 19);
		VERIFY(aaa_2 == 23);
		VERIFY(bbb_2 == 24);
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

		VERIFY(aaa_1 == 12);
		VERIFY(bbb_1 == 19);
		VERIFY(aaa_2 == 23);
		VERIFY(bbb_2 == 24);
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

		VERIFY(aaa_1 == 12);
		VERIFY(bbb_1 == 19);
		VERIFY(aaa_2 == 23);
		VERIFY(bbb_2 == 24);
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

		VERIFY(aaa_1 == 12);
		VERIFY(bbb_1 == 19);
		VERIFY(aaa_2 == 23);
		VERIFY(bbb_2 == 24);
	}
	return 0;
}

static int mk_xxx = mk_type_eraser_test_func();
