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
struct my_get_index_helper;

template<int idx, typename head_t, typename... tail_ts>
struct my_get_index_helper<idx, head_t, tail_ts...>
{
	auto& operator()(my_tuple<head_t, tail_ts...>& val) const
	{
		return my_get_index_helper<idx - 1, tail_ts...>{}(val.m_tail);
	}
};

template<typename head_t, typename... tail_ts>
struct my_get_index_helper<0, head_t, tail_ts...>
{
	auto& operator()(my_tuple<head_t, tail_ts...>& val) const
	{
		return val.m_head;
	}
};


template<typename target_t, typename head_t, typename... tail_ts>
struct my_get_type_helper
{
	auto& operator()(my_tuple<head_t, tail_ts...>& val) const
	{
		return my_get_type_helper<target_t, tail_ts...>{}(val.m_tail);
	}
};

template<typename target_t_and_head_t, typename... tail_ts>
struct my_get_type_helper<target_t_and_head_t, target_t_and_head_t, tail_ts...>
{
	auto& operator()(my_tuple<target_t_and_head_t, tail_ts...>& val) const
	{
		return val.m_head;
	}
};


template<int idx, typename... ts>
auto& my_get(my_tuple<ts...>& val)
{
	return my_get_index_helper<idx, ts...>{}(val);
}

template<typename target, typename... tail_ts>
auto& my_get(my_tuple<tail_ts...>& val)
{
	return my_get_type_helper<target, tail_ts...>{}(val);
}


template<typename t>
struct my_add_pointer
{
	typedef t* type;
};


enum class my_layout
{
	aos,
	soa,
};


template<my_layout, typename>
struct my_container;

template<typename... ts>
struct my_container<my_layout::aos, my_tuple<ts...>>
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
	template<typename target_t>
	auto& get(int i)
	{
		return my_get<target_t>(m_data[i]);
	}
	typename my_add_pointer<my_tuple<ts...>>::type m_data;
};

template<typename... ts>
struct my_container<my_layout::soa, my_tuple<ts...>>
{
public:
	void resize(int cnt)
	{
		resize_impl<ts...>(cnt);
	}
	void destroy()
	{
		destroy_impl<true, ts...>();
	}
	template<int idx>
	auto& get(int i)
	{
		return my_get<idx>(m_data)[i];
	}
	template<typename target_t>
	auto& get(int i)
	{
		return my_get<typename my_add_pointer<target_t>::type>(m_data)[i];
	}
private:
	template<typename...>
	void resize_impl(...)
	{
	}
	template<typename head_t, typename... tail_ts>
	void resize_impl(int const cnt)
	{
		my_get<sizeof...(ts) - sizeof...(tail_ts) - 1>(m_data) = new head_t[cnt];
		resize_impl<tail_ts...>(cnt);
	}
	template<bool>
	void destroy_impl()
	{
	}
	template<bool, typename head_t, typename... tail_ts>
	void destroy_impl()
	{
		delete[] my_get<sizeof...(ts) - sizeof...(tail_ts) - 1>(m_data);
		destroy_impl<true, tail_ts...>();
	}
private:
	my_tuple<typename my_add_pointer<ts>::type...> m_data;
};





struct hours__{ int m_v; };
struct minutes{ int m_v; };
struct seconds{ int m_v; };
typedef my_tuple<hours__, minutes, seconds> time_t;

int main()
{
	time_t time;
	my_get<0>(time).m_v = 14;
	my_get<1>(time).m_v = 50;
	my_get<2>(time).m_v = 55;

	my_get<hours__>(time).m_v = 15;
	my_get<minutes>(time).m_v = 51;
	my_get<seconds>(time).m_v = 56;

	my_container<my_layout::aos, time_t> cnt1;
	cnt1.resize(1000);
	cnt1.get<0>(0).m_v = 1;
	cnt1.get<1>(0).m_v = 2;
	cnt1.get<2>(0).m_v = 3;
	cnt1.get<0>(1).m_v = 4;
	cnt1.get<1>(1).m_v = 5;
	cnt1.get<2>(1).m_v = 6;
	cnt1.get<0>(2).m_v = 7;
	cnt1.get<1>(2).m_v = 8;
	cnt1.get<2>(2).m_v = 9;

	cnt1.get<hours__>(0).m_v = 11;
	cnt1.get<minutes>(0).m_v = 12;
	cnt1.get<seconds>(0).m_v = 13;
	cnt1.get<hours__>(1).m_v = 14;
	cnt1.get<minutes>(1).m_v = 15;
	cnt1.get<seconds>(1).m_v = 16;
	cnt1.get<hours__>(2).m_v = 17;
	cnt1.get<minutes>(2).m_v = 18;
	cnt1.get<seconds>(2).m_v = 19;

	my_container<my_layout::soa, time_t> cnt2;
	cnt2.resize(1000);
	cnt2.get<0>(0).m_v = 1;
	cnt2.get<1>(0).m_v = 2;
	cnt2.get<2>(0).m_v = 3;
	cnt2.get<0>(1).m_v = 4;
	cnt2.get<1>(1).m_v = 5;
	cnt2.get<2>(1).m_v = 6;
	cnt2.get<0>(2).m_v = 7;
	cnt2.get<1>(2).m_v = 8;
	cnt2.get<2>(2).m_v = 9;

	cnt2.get<hours__>(0).m_v = 11;
	cnt2.get<minutes>(0).m_v = 12;
	cnt2.get<seconds>(0).m_v = 13;
	cnt2.get<hours__>(1).m_v = 14;
	cnt2.get<minutes>(1).m_v = 15;
	cnt2.get<seconds>(1).m_v = 16;
	cnt2.get<hours__>(2).m_v = 17;
	cnt2.get<minutes>(2).m_v = 18;
	cnt2.get<seconds>(2).m_v = 19;

	cnt1.destroy();
	cnt2.destroy();
}
