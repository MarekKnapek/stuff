#pragma once


namespace mk
{

	namespace detail
	{
		template<unsigned n>
		struct string_t
		{
			constexpr string_t(char const(&str)[n])
			{
				for(unsigned i = 0; i != n; ++i)
				{
					m_value[i] = str[i];
				}
			}
			char m_value[n]{};
			static constexpr unsigned const s_size = n;
		};
		template<unsigned n>
		string_t(char const(&)[n]) -> string_t<n>;
	}

}


namespace mk
{

	namespace detail
	{
		template<typename t>
		struct value_t
		{
			constexpr value_t(t const& v) : m_value(v) {}
			typedef t type;
			type const m_value;
		};
		template<typename t, typename m>
		struct value_t<m t::*>
		{
			constexpr value_t(m t::* const& v) : m_value(v) {}
			typedef m t::* type;
			typedef t class_t;
			typedef m member_t;
			type const m_value;
		};
	}

}


namespace mk
{

	template<detail::string_t name_v, detail::value_t mem_ptr_v>
	struct member_t
	{
		typedef decltype(name_v) name_t;
		typedef decltype(mem_ptr_v) mem_ptr_t;
		static constexpr name_t const& s_name = name_v;
		static constexpr mem_ptr_t const& s_mem_ptr = mem_ptr_v;
	};

}
