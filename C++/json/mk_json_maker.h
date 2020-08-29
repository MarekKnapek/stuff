#pragma once


#include "mk_json_utils.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>


namespace mk
{


	template<typename t>
	std::string json_make(t const& v);


	template<typename... members_t>
	struct json_maker_automagic_t
	{
		template<typename t>
		static nlohmann::json make(t const& v);
	};

	template<>
	struct json_maker_automagic_t<void>
	{
		template<typename t>
		static nlohmann::json make(t const& v);
	};


}


namespace mk
{
	namespace customize
	{
		template<typename t>
		struct json_maker_t
		{
			static nlohmann::json make(t const& v) = delete;
		};
	}
}

namespace mk
{
	namespace customize
	{
		template<typename t>
		struct json_maker_appender_t
		{
			static void append(nlohmann::json& j, char const* const& b, char const* const& e, t const& v);
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
			static nlohmann::json make(t const& v);
		};
	}
}

template<typename t>
nlohmann::json mk::detail::json_maker_verbatim_t<t>::make(t const& v)
{
	return v;
}


namespace mk
{
	namespace detail
	{
		template<typename from, typename to>
		struct json_maker_cast_t
		{
			static nlohmann::json make(from const& v);
		};
	}
}

template<typename from, typename to>
nlohmann::json mk::detail::json_maker_cast_t<from, to>::make(from const& v)
{
	return mk::customize::json_maker_t<to>::make(v);
}

namespace mk
{
	namespace detail
	{
		template<typename t>
		struct json_maker_array_t
		{
			static nlohmann::json make(t const* const& b, t const* const& e);
		};
	}
}

template<typename t>
nlohmann::json mk::detail::json_maker_array_t<t>::make(t const* const& b, t const* const& e)
{
	nlohmann::json j = nlohmann::json::array();
	for(auto i = b; i != e; ++i)
	{
		j.push_back(mk::customize::json_maker_t<t>::make(*i));
	}
	return j;
}


namespace mk { namespace customize { template<> struct json_maker_t<bool> : mk::detail::json_maker_verbatim_t<bool> {}; } }

namespace mk { namespace customize { template<> struct json_maker_t<signed char> : mk::detail::json_maker_cast_t<signed char, signed short int> {}; } }
namespace mk { namespace customize { template<> struct json_maker_t<signed short int> : mk::detail::json_maker_cast_t<signed short int, signed int> {}; } }
namespace mk { namespace customize { template<> struct json_maker_t<signed int> : mk::detail::json_maker_cast_t<signed int, signed long int> {}; } }
namespace mk { namespace customize { template<> struct json_maker_t<signed long int> : mk::detail::json_maker_cast_t<signed long int, signed long long int> {}; } }
namespace mk { namespace customize { template<> struct json_maker_t<signed long long int> : mk::detail::json_maker_verbatim_t<signed long long int> {}; } }

namespace mk { namespace customize { template<> struct json_maker_t<char> : mk::detail::json_maker_cast_t<char, int> {}; } }

namespace mk { namespace customize { template<> struct json_maker_t<unsigned char> : mk::detail::json_maker_cast_t<unsigned char, unsigned short int> {}; } }
namespace mk { namespace customize { template<> struct json_maker_t<unsigned short int> : mk::detail::json_maker_cast_t<unsigned short int, unsigned int> {}; } }
namespace mk { namespace customize { template<> struct json_maker_t<unsigned int> : mk::detail::json_maker_cast_t<unsigned int, unsigned long int> {}; } }
namespace mk { namespace customize { template<> struct json_maker_t<unsigned long int> : mk::detail::json_maker_cast_t<unsigned long int, unsigned long long int> {}; } }
namespace mk { namespace customize { template<> struct json_maker_t<unsigned long long int> : mk::detail::json_maker_verbatim_t<unsigned long long int> {}; } }

namespace mk { namespace customize { template<> struct json_maker_t<float> : mk::detail::json_maker_cast_t<float, double> {}; } }
namespace mk { namespace customize { template<> struct json_maker_t<double> : mk::detail::json_maker_cast_t<double, long double> {}; } }
namespace mk { namespace customize { template<> struct json_maker_t<long double> : mk::detail::json_maker_verbatim_t<long double> {}; } }

namespace mk { namespace customize { template<> struct json_maker_t<std::string> : mk::detail::json_maker_verbatim_t<std::string> {}; } }

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
	return nlohmann::json{};
}

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
	return mk::detail::json_maker_array_t<t>::make(v.data(), v.data() + v.size());
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
	return mk::detail::json_maker_array_t<t>::make(v.data(), v.data() + v.size());
}

namespace mk
{
	namespace customize
	{
		template<typename t, std::size_t n>
		struct json_maker_t<t[n]>
		{
			static nlohmann::json make(t const(&v)[n]);
		};
	}
}

template<typename t, std::size_t n>
nlohmann::json mk::customize::json_maker_t<t[n]>::make(t const(&v)[n])
{
	return mk::detail::json_maker_array_t<t>::make(v + 0, v + n);
}


template<typename t>
void mk::customize::json_maker_appender_t<t>::append(nlohmann::json& j, char const* const& b, char const* const& e, t const& v)
{
	j[std::string{b, e}] = mk::customize::json_maker_t<t>::make(v);
}


namespace mk
{
	namespace customize
	{
		template<typename t>
		struct json_maker_appender_t<std::optional<t>>
		{
			static void append(nlohmann::json& j, char const* const& b, char const* const& e, std::optional<t> const& v);
		};
	}
}

template<typename t>
void mk::customize::json_maker_appender_t<std::optional<t>>::append(nlohmann::json& j, char const* const& b, char const* const& e, std::optional<t> const& v)
{
	if(v)
	{
		mk::customize::json_maker_appender_t<t>::append(j, b, e, *v);
	}
}



namespace mk
{
	namespace detail
	{
		template<typename t, typename member_t>
		struct json_maker_appender_base_t
		{
			static void append(nlohmann::json& j, t const& v);
		};
		template<typename t, typename member_t, typename... members_t>
		struct json_maker_appender_t
		{
			static void append(nlohmann::json& j, t const& v);
		};
		template<typename t, typename member_t>
		struct json_maker_appender_t<t, member_t>
		{
			static void append(nlohmann::json& j, t const& v);
		};
	}
}

template<typename t, typename member_t>
void mk::detail::json_maker_appender_base_t<t, member_t>::append(nlohmann::json& j, t const& v)
{
	char const* const b = member_t::s_name.m_value + 0;
	char const* const e = b + member_t::s_name.s_size - 1;
	auto const& m = v.*(member_t::s_mem_ptr.m_value);
	mk::customize::json_maker_appender_t<typename member_t::mem_ptr_t::member_t>::append(j, b, e, m);
}

template<typename t, typename member_t, typename... members_t>
void mk::detail::json_maker_appender_t<t, member_t, members_t...>::append(nlohmann::json& j, t const& v)
{
	mk::detail::json_maker_appender_base_t<t, member_t>::append(j, v);
	mk::detail::json_maker_appender_t<t, members_t...>::append(j, v);
}

template<typename t, typename member_t>
void mk::detail::json_maker_appender_t<t, member_t>::append(nlohmann::json& j, t const& v)
{
	mk::detail::json_maker_appender_base_t<t, member_t>::append(j, v);
}





template<typename t>
std::string mk::json_make(t const& v)
{
	return mk::customize::json_maker_t<t>::make(v).dump(-1, ' ', true, nlohmann::json::error_handler_t::strict);
}

template<typename... members_t>
template<typename t>
nlohmann::json mk::json_maker_automagic_t<members_t...>::make(t const& v)
{
	nlohmann::json j;
	mk::detail::json_maker_appender_t<t, members_t...>::append(j, v);
	return j;
}

template<typename t>
nlohmann::json mk::json_maker_automagic_t<void>::make(t const& v)
{
	return nlohmann::json::object();
}
