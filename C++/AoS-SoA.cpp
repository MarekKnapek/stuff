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


template<int idx, typename head_t, typename... tail_ts>
struct my_get_type
{
	typedef typename my_get_type<idx - 1, tail_ts...>::type type;
};

template<typename head_t, typename... tail_ts>
struct my_get_type<0, head_t, tail_ts...>
{
	typedef head_t type;
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
	typename my_add_pointer<my_tuple<ts...>>::type m_data;
};

template<typename... ts>
struct my_container<my_layout::soa, my_tuple<ts...>>
{
	my_tuple<typename my_add_pointer<ts>::type...> m_data;
};


namespace detail
{

	template<int idx, typename... ts>
	struct my_container_soa_init_impl
	{
		void operator()(my_container<my_layout::soa, my_tuple<ts...>>& container)
		{
			auto& elem = my_get<sizeof...(ts) - idx>(container.m_data);
			elem = nullptr;
			my_container_soa_init_impl<idx - 1, ts...>{}(container);
		}
	};
	template<typename... ts>
	struct my_container_soa_init_impl<0, ts...>
	{
		void operator()(...)
		{
		}
	};

	template<int idx, typename... ts>
	struct my_container_soa_destroy_impl
	{
		void operator()(my_container<my_layout::soa, my_tuple<ts...>>& container)
		{
			auto& elem = my_get<sizeof...(ts) - idx>(container.m_data);
			delete[] elem;
			my_container_soa_destroy_impl<idx - 1, ts...>{}(container);
		}
	};
	template<typename... ts>
	struct my_container_soa_destroy_impl<0, ts...>
	{
		void operator()(...)
		{
		}
	};

	template<int idx, typename... ts>
	struct my_container_soa_allocate_impl
	{
		void operator()(my_container<my_layout::soa, my_tuple<ts...>>& container, int const count)
		{
			static constexpr int const s_idx = sizeof...(ts) - idx;
			typedef typename my_get_type<s_idx, ts...>::type curr_type;
			auto& elem = my_get<s_idx>(container.m_data);
			elem = new curr_type[count];
			my_container_soa_allocate_impl<idx - 1, ts...>{}(container, count);
		}
	};
	template<typename... ts>
	struct my_container_soa_allocate_impl<0, ts...>
	{
		void operator()(...)
		{
		}
	};

}


template<typename my_tuple_t>
void my_container_init(my_container<my_layout::aos, my_tuple_t>& container)
{
	container.m_data = nullptr;
}

template<typename my_tuple_t>
void my_container_destroy(my_container<my_layout::aos, my_tuple_t>& container)
{
	delete[] container.m_data;
}

template<typename my_tuple_t>
void my_container_allocate(my_container<my_layout::aos, my_tuple_t>& container, int const count)
{
	container.m_data = new my_tuple_t[count];
}

template<int type_idx, typename my_tuple_t>
auto& my_container_get(my_container<my_layout::aos, my_tuple_t>& container, int const elem_idx)
{
	return my_get<type_idx>(container.m_data[elem_idx]);
}

template<typename target_t, typename my_tuple_t>
auto& my_container_get(my_container<my_layout::aos, my_tuple_t>& container, int const elem_idx)
{
	return my_get<target_t>(container.m_data[elem_idx]);
}

template<typename... ts>
void my_container_init(my_container<my_layout::soa, my_tuple<ts...>>& container)
{
	detail::my_container_soa_init_impl<sizeof...(ts), ts...>{}(container);
}

template<typename... ts>
void my_container_destroy(my_container<my_layout::soa, my_tuple<ts...>>& container)
{
	detail::my_container_soa_destroy_impl<sizeof...(ts), ts...>{}(container);
}

template<typename... ts>
void my_container_allocate(my_container<my_layout::soa, my_tuple<ts...>>& container, int const count)
{
	detail::my_container_soa_allocate_impl<sizeof...(ts), ts...>{}(container, count);
}

template<int type_idx, typename my_tuple_t>
auto& my_container_get(my_container<my_layout::soa, my_tuple_t>& container, int const elem_idx)
{
	return my_get<type_idx>(container.m_data)[elem_idx];
}

template<typename target_t, typename my_tuple_t>
auto& my_container_get(my_container<my_layout::soa, my_tuple_t>& container, int const elem_idx)
{
	return my_get<typename my_add_pointer<target_t>::type>(container.m_data)[elem_idx];
}





struct hours__{ int m_v; };
struct minutes{ int m_v; };
struct seconds{ int m_v; };
typedef my_tuple<hours__, minutes, seconds> my_time_t;
typedef my_container<my_layout::aos, my_time_t> my_times_aos;
typedef my_container<my_layout::soa, my_time_t> my_times_soa;


template<typename my_container_t>
void process_container(my_container_t& container)
{
	my_container_init(container);
	my_container_allocate(container, 3);

	// This order of access works for both, but is better for aos.
	my_container_get<0>(container, 0).m_v = 1;
	my_container_get<1>(container, 0).m_v = 2;
	my_container_get<2>(container, 0).m_v = 3;
	my_container_get<0>(container, 1).m_v = 4;
	my_container_get<1>(container, 1).m_v = 5;
	my_container_get<2>(container, 1).m_v = 6;
	my_container_get<0>(container, 2).m_v = 7;
	my_container_get<1>(container, 2).m_v = 8;
	my_container_get<2>(container, 2).m_v = 9;
	my_container_get<hours__>(container, 0).m_v = 11;
	my_container_get<minutes>(container, 0).m_v = 12;
	my_container_get<seconds>(container, 0).m_v = 13;
	my_container_get<hours__>(container, 1).m_v = 14;
	my_container_get<minutes>(container, 1).m_v = 15;
	my_container_get<seconds>(container, 1).m_v = 16;
	my_container_get<hours__>(container, 2).m_v = 17;
	my_container_get<minutes>(container, 2).m_v = 18;
	my_container_get<seconds>(container, 2).m_v = 19;

	// This order of access works for both, but is better for soa.
	my_container_get<0>(container, 0).m_v = 1;
	my_container_get<0>(container, 1).m_v = 2;
	my_container_get<0>(container, 2).m_v = 3;
	my_container_get<1>(container, 0).m_v = 4;
	my_container_get<1>(container, 1).m_v = 5;
	my_container_get<1>(container, 2).m_v = 6;
	my_container_get<2>(container, 0).m_v = 7;
	my_container_get<2>(container, 1).m_v = 8;
	my_container_get<2>(container, 2).m_v = 9;
	my_container_get<hours__>(container, 0).m_v = 11;
	my_container_get<hours__>(container, 1).m_v = 12;
	my_container_get<hours__>(container, 2).m_v = 13;
	my_container_get<minutes>(container, 0).m_v = 14;
	my_container_get<minutes>(container, 1).m_v = 15;
	my_container_get<minutes>(container, 2).m_v = 16;
	my_container_get<seconds>(container, 0).m_v = 17;
	my_container_get<seconds>(container, 1).m_v = 18;
	my_container_get<seconds>(container, 2).m_v = 19;

	my_container_destroy(container);
}


int main()
{
	my_time_t time;
	my_get<0>(time).m_v = 14;
	my_get<1>(time).m_v = 50;
	my_get<2>(time).m_v = 55;

	my_get<hours__>(time).m_v = 15;
	my_get<minutes>(time).m_v = 51;
	my_get<seconds>(time).m_v = 56;

	/*
	*
	*    |----|                 |----||----||----||----||----||----||----||----||----|
	*    |ptr |---------------->| hr || mn || sc || hr || mn || sc || hr || mn || sc |
	*    |----|                 |----||----||----||----||----||----||----||----||----|
	*
	*/
	my_times_aos cnt1;
	process_container(cnt1);
	/*
	*
	*    |----||----||----|
	*    |ptr ||ptr ||ptr |--------------------------------\
	*    |----||----||----|                                |
	*      |     |                                         |
	*      |     \-----------------\                       |
	*      |                       |                       |
	*      |   |----||----||----|  |   |----||----||----|  |   |----||----||----|
	*      \-->| hr || hr || hr |  \-->| mn || mn || mn |  \-->| sc || sc || sc |
	*          |----||----||----|      |----||----||----|      |----||----||----|
	*
	*/
	my_times_soa cnt2;
	process_container(cnt2);
}
