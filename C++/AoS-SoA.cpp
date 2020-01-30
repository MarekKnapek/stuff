template<typename...>
struct my_tuple;

template<typename head_t, typename... tail_ts>
struct my_tuple<head_t, tail_ts...>
{
	head_t m_head;
	my_tuple<tail_ts...> m_tail;
};

template<typename head_t>
struct my_tuple<head_t>
{
	head_t m_head;
};


template<int, typename...>
struct my_get_helper;

template<int idx, typename head_t, typename... tail_ts>
struct my_get_helper<idx, head_t, tail_ts...>
{
	auto& operator()(my_tuple<head_t, tail_ts...>& val) const
	{
		return my_get_helper<idx - 1, tail_ts...>{}(val.m_tail);
	}
};

template<typename head_t, typename... tail_ts>
struct my_get_helper<0, head_t, tail_ts...>
{
	auto& operator()(my_tuple<head_t, tail_ts...>& val) const
	{
		return val.m_head;
	}
};


template<int idx, typename... ts>
auto& my_get(my_tuple<ts...>& val)
{
	return my_get_helper<idx, ts...>{}(val);
}


template<typename t>
struct my_add_pointer
{
	typedef t* type;
};


template<int...>
struct my_sequence
{
};

template<int head_int, int... tail_ints>
struct my_sequence<head_int, tail_ints...>
{
	static int const s_int = head_int;
	typedef my_sequence<tail_ints...> rest_seq;
};


template<int...>
struct my_sequence_helper;

template<int head_int, int... tail_ints>
struct my_sequence_helper<head_int, tail_ints...>
{
	typedef typename my_sequence_helper<head_int - 1, head_int, tail_ints...>::type type;
};

template<int... ints>
struct my_sequence_helper<0, ints...>
{
	typedef my_sequence<0, ints...> type;
};


template<int size>
auto my_make_my_sequence()
{
	return typename my_sequence_helper<size>::type{};
}


enum my_layout
{
	aos,
	soa,
};


template<my_layout, typename>
struct my_container;

template<typename... ts>
struct my_container<aos, my_tuple<ts...>>
{
	void resize(int cnt)
	{
		m_data = new my_tuple<ts...>[cnt];
	}
	void destroy()
	{
		delete[] m_data;
	}
	template<int idx>
	auto& get(int i)
	{
		return my_get<idx>(m_data[i]);
	}
	typename my_add_pointer<my_tuple<ts...>>::type m_data;
};

template<typename... ts>
struct my_container<soa, my_tuple<ts...>>
{
public:
	void resize(int cnt)
	{
		auto const s = my_make_my_sequence<sizeof...(ts)>();
		resize_impl<ts...>(cnt, s);
	}
	void destroy()
	{
		auto const s = my_make_my_sequence<sizeof...(ts)>();
		destroy_impl<ts...>(s);
	}
	template<int idx>
	auto& get(int i)
	{
		return my_get<idx>(m_data)[i];
	}
private:
	template<int head_int>
	void resize_impl(int const, my_sequence<head_int> const&)
	{
	}
	template<typename t, typename... ts2, int head_int, int... tail_ints>
	void resize_impl(int const cnt, my_sequence<head_int, tail_ints...> const& s)
	{
		my_get<s.s_int>(m_data) = new t[cnt];
		resize_impl<ts2...>(cnt, my_sequence<tail_ints...>{});
	}
	template<int head_int>
	void destroy_impl(my_sequence<head_int> const&)
	{
	}
	template<typename t, typename... ts2, int head_int, int... tail_ints>
	void destroy_impl(my_sequence<head_int, tail_ints...> const& s)
	{
		delete[] my_get<s.s_int>(m_data);
	}
private:
	my_tuple<typename my_add_pointer<ts>::type...> m_data;
};





int main()
{
	struct hours{ int m_v; };
	struct minutes{ int m_v; };
	struct seconds{ int m_v; };
	typedef my_tuple<hours, minutes, seconds> time_t;

	time_t time;
	my_get<0>(time).m_v = 14;
	my_get<1>(time).m_v = 50;
	my_get<2>(time).m_v = 59;

	my_container<aos, time_t> cnt1;
	cnt1.resize(3);
	cnt1.get<0>(0).m_v = 1;
	cnt1.get<1>(0).m_v = 2;
	cnt1.get<2>(0).m_v = 3;
	cnt1.get<0>(1).m_v = 4;
	cnt1.get<1>(1).m_v = 5;
	cnt1.get<2>(1).m_v = 6;
	cnt1.get<0>(2).m_v = 7;
	cnt1.get<1>(2).m_v = 8;
	cnt1.get<2>(2).m_v = 9;

	my_container<soa, time_t> cnt2;
	cnt2.resize(3);
	cnt2.get<0>(0).m_v = 1;
	cnt2.get<1>(0).m_v = 2;
	cnt2.get<2>(0).m_v = 3;
	cnt2.get<0>(1).m_v = 4;
	cnt2.get<1>(1).m_v = 5;
	cnt2.get<2>(1).m_v = 6;
	cnt2.get<0>(2).m_v = 7;
	cnt2.get<1>(2).m_v = 8;
	cnt2.get<2>(2).m_v = 9;

	cnt1.destroy();
	cnt2.destroy();
}
