#pragma once


#include <nlohmann/json.hpp>


namespace mk
{

	namespace detail
	{
		template<unsigned n>
		struct fixed_string_impl_t
		{
			constexpr fixed_string_impl_t(char const(&str)[n])
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
		fixed_string_impl_t(char const(&)[n]) -> fixed_string_impl_t<n>;
	}

	template<detail::fixed_string_impl_t fs>
	struct string_t
	{
		typedef decltype(fs) type;
		static constexpr type const& s_value = fs;
	};

}


namespace mk
{

	namespace detail
	{
		template<typename t>
		struct value_impl_t
		{
			constexpr value_impl_t(t const& v) : m_value(v) {}
			typedef t type;
			type const m_value;
		};
		template<typename t, typename m>
		struct value_impl_t<m t::*>
		{
			constexpr value_impl_t(m t::* const& v) : m_value(v) {}
			typedef m t::* type;
			typedef t class_t;
			typedef m member_t;
			type const m_value;
		};
	}

	template<detail::value_impl_t v>
	struct value_t
	{
		typedef decltype(v) type;
		static constexpr type const& s_value = v;
	};

}


namespace mk
{

	template<detail::fixed_string_impl_t name_v, detail::value_impl_t mem_ptr_v>
	struct member_t
	{
		typedef decltype(name_v) name_t;
		typedef decltype(mem_ptr_v) mem_ptr_t;
		static constexpr name_t const& s_name = name_v;
		static constexpr mem_ptr_t const& s_mem_ptr = mem_ptr_v;
	};

}


namespace mk
{


	template<typename t>
	nlohmann::json json_make(t const& v);


	namespace customize
	{


		template<typename t>
		struct json_maker_t
		{
			static nlohmann::json make(t const& v) = delete;
		};

		template<typename t>
		struct json_appender_t
		{
			static void append(nlohmann::json& j, char const* const& b, char const* const& e, t const& v);
		};


	}


	template<typename... members_t>
	struct json_maker_automagic_t
	{
		template<typename t>
		static nlohmann::json make(t const& v);
	};


}





// ========== ==========





#include <array>
#include <iterator>
#include <optional>
#include <string>
#include <vector>


namespace mk
{
	namespace detail
	{
		template<typename t>
		struct json_appender_optional_t
		{
			static void append(nlohmann::json& j, char const* const& b, char const* const& e, std::optional<t> const& v)
			{
				if(v.has_value())
				{
					customize::json_appender_t<t>::append(j, b, e, v.value());
				}
			}
		};
	}
}

namespace mk
{
	namespace detail
	{
		template<typename t>
		struct json_maker_verbatim_t
		{
			static nlohmann::json make(t const& v)
			{
				return v;
			}
		};
	}
}

namespace mk
{
	namespace detail
	{
		template<typename to_t>
		struct json_maker_static_cast_t
		{
			template<typename from_t>
			static nlohmann::json make(from_t const& v)
			{
				return customize::json_maker_t<to_t>::make(static_cast<to_t>(v));
			}
		};
	}
}

namespace mk
{
	namespace detail
	{
		template<typename t>
		void append_array(nlohmann::json& j, t const* const& b, t const* const& e)
		{
			for(t const* it = b; it != e; ++it)
			{
				j.push_back(customize::json_maker_t<t>::make(*it));
			}
		}
	}
}


namespace mk { namespace customize { template<> struct json_maker_t<bool> : detail::json_maker_verbatim_t<bool> {}; } }

namespace mk { namespace customize { template<> struct json_maker_t<signed char> : detail::json_maker_static_cast_t<signed short> {}; } }
namespace mk { namespace customize { template<> struct json_maker_t<signed short> : detail::json_maker_static_cast_t<signed int> {}; } }
namespace mk { namespace customize { template<> struct json_maker_t<signed int> : detail::json_maker_static_cast_t<signed long int> {}; } }
namespace mk { namespace customize { template<> struct json_maker_t<signed long int> : detail::json_maker_static_cast_t<signed long long int> {}; } }
namespace mk { namespace customize { template<> struct json_maker_t<signed long long int> : detail::json_maker_verbatim_t<signed long long int> {}; } }

namespace mk { namespace customize { template<> struct json_maker_t<unsigned char> : detail::json_maker_static_cast_t<unsigned short> {}; } }
namespace mk { namespace customize { template<> struct json_maker_t<unsigned short> : detail::json_maker_static_cast_t<unsigned int> {}; } }
namespace mk { namespace customize { template<> struct json_maker_t<unsigned int> : detail::json_maker_static_cast_t<unsigned long int> {}; } }
namespace mk { namespace customize { template<> struct json_maker_t<unsigned long int> : detail::json_maker_static_cast_t<unsigned long long int> {}; } }
namespace mk { namespace customize { template<> struct json_maker_t<unsigned long long int> : detail::json_maker_verbatim_t<unsigned long long int> {}; } }

namespace mk { namespace customize { template<> struct json_maker_t<float> : detail::json_maker_static_cast_t<double> {}; } }
namespace mk { namespace customize { template<> struct json_maker_t<double> : detail::json_maker_static_cast_t<long double> {}; } }
namespace mk { namespace customize { template<> struct json_maker_t<long double> : detail::json_maker_verbatim_t<long double> {}; } }

namespace mk { namespace customize { template<> struct json_maker_t<std::string> : detail::json_maker_verbatim_t<std::string> {}; } }

namespace mk { namespace customize { template<typename t> struct json_appender_t<std::optional<t>> : detail::json_appender_optional_t<t> {}; } }


namespace mk
{
	namespace customize
	{
		template<typename t>
		struct json_maker_t<std::vector<t>>
		{
			static nlohmann::json make(std::vector<t> const& v);
		};
	}
}

template<typename t>
nlohmann::json mk::customize::json_maker_t<std::vector<t>>::make(std::vector<t> const& v)
{
	nlohmann::json j = nlohmann::json::array();
	detail::append_array(j, v.data(), v.data() + v.size());
	return j;
}


namespace mk
{
	namespace customize
	{
		template<typename t, std::size_t n>
		struct json_maker_t<std::array<t, n>>
		{
			static nlohmann::json make(std::array<t, n> const& v);
		};
	}
}

template<typename t, std::size_t n>
nlohmann::json mk::customize::json_maker_t<std::array<t, n>>::make(std::array<t, n> const& v)
{
	nlohmann::json j = nlohmann::json::array();
	detail::append_array(j, v.data(), v.data() + v.size());
	return j;
}


namespace mk
{
	namespace customize
	{
		template<>
		struct json_maker_t<std::nullptr_t>
		{
			static nlohmann::json make(std::nullptr_t const&);
		};
	}
}

nlohmann::json mk::customize::json_maker_t<std::nullptr_t>::make(std::nullptr_t const&)
{
	nlohmann::json j;
	return j;
}


namespace mk
{

	namespace detail
	{

		template<typename member_t>
		struct json_automagic_impl_base_t
		{
			template<typename t>
			static void append_base(nlohmann::json& j, t const& v);
		};

		template<typename member_t, typename... members_t>
		struct json_maker_automagic_impl_t : json_automagic_impl_base_t<member_t>
		{
			template<typename t>
			static void append_impl(nlohmann::json& j, t const& v);
		};

		template<typename member_t>
		struct json_maker_automagic_impl_t<member_t> : json_automagic_impl_base_t<member_t>
		{
			template<typename t>
			static void append_impl(nlohmann::json& j, t const& v);
		};

	}

}

template<typename member_t>
template<typename t>
void mk::detail::json_automagic_impl_base_t<member_t>::append_base(nlohmann::json& j, t const& v)
{
	char const* const b = member_t::s_name.m_value + 0;
	char const* const e = member_t::s_name.m_value + member_t::s_name.s_size - 1;
	auto const p = member_t::s_mem_ptr.m_value;
	customize::json_appender_t<typename member_t::mem_ptr_t::member_t>::append(j, b, e, v.*p);
}

template<typename member_t, typename... members_t>
template<typename t>
void mk::detail::json_maker_automagic_impl_t<member_t, members_t...>::append_impl(nlohmann::json& j, t const& v)
{
	json_maker_automagic_impl_t::append_base(j, v);
	json_maker_automagic_impl_t<members_t...>::append_impl(j, v);
}

template<typename member_t>
template<typename t>
void mk::detail::json_maker_automagic_impl_t<member_t>::append_impl(nlohmann::json& j, t const& v)
{
	json_maker_automagic_impl_t::append_base(j, v);
}





// ========== ==========





template<typename t>
nlohmann::json mk::json_make(t const& v)
{
	return customize::json_maker_t<t>::make(v);
}


template<typename t>
void mk::customize::json_appender_t<t>::append(nlohmann::json& j, char const* const& b, char const* const& e, t const& v)
{
	j[std::string{b, e}] = customize::json_maker_t<t>::make(v);
}


template<typename... members_t>
template<typename t>
nlohmann::json mk::json_maker_automagic_t<members_t...>::make(t const& v)
{
	nlohmann::json j;
	detail::json_maker_automagic_impl_t<members_t...>::append_impl(j, v);
	return j;
}
