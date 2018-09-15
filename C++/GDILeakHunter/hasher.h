#pragma once
#ifndef mk_hasher_h_included
#define mk_hasher_h_included
#include "setup.h"
#if mk_should_enable == 1


#include <utility> // std::move
#include <cstddef> // std::size_t


namespace mk
{

	template<typename alg_t>
	class hasher
	{
	public:
		hasher(alg_t const& alg) :
			m_alg(alg)
		{
		}
		hasher(alg_t&& alg) :
			m_alg(std::move(alg))
		{
		}
		void add_to_hash(void const* const& buff, unsigned const& size)
		{
			return m_alg.add_to_hash(buff, size);
		}
		std::size_t finish()
		{
			return m_alg.finish();
		}
	private:
		hasher(hasher const&) = delete;
		hasher(hasher&&) = delete;
		hasher& operator=(hasher const&) = delete;
		hasher& operator=(hasher&&) = delete;
	private:
		alg_t m_alg;
	};

}





#include <cstdint> // std::uint32_t CHAR_BIT
#include <cstddef> // std::size_t


namespace mk
{

	namespace detail
	{

		static std::uint32_t const s_fnv1_prime_32 = static_cast<std::uint32_t>(16777619ULL);
		static std::uint64_t const s_fnv1_prime_64 = static_cast<std::uint64_t>(1099511628211ULL);
		static std::uint32_t const s_fnv1_offset_basis_32 = static_cast<std::uint32_t>(2166136261ULL);
		static std::uint64_t const s_fnv1_offset_basis_64 = static_cast<std::uint64_t>(14695981039346656037ULL);

		template<unsigned N>
		class bitness
		{
		};

		template<>
		class bitness<32>
		{
		public:
			std::uint32_t get_prime() const
			{
				return s_fnv1_prime_32;
			}
			std::uint32_t get_offset_basis() const
			{
				return s_fnv1_offset_basis_32;
			}
		};

		template<>
		class bitness<64>
		{
		public:
			std::uint64_t get_prime() const
			{
				return s_fnv1_prime_64;
			}
			std::uint64_t get_offset_basis() const
			{
				return s_fnv1_offset_basis_64;
			}
		};

		std::size_t get_prime()
		{
			return bitness<sizeof(void*) * CHAR_BIT>{}.get_prime();
		}

		std::size_t get_offset_basis()
		{
			return bitness<sizeof(void*) * CHAR_BIT>{}.get_offset_basis();
		}

	}

	class fnv1
	{
	public:
		fnv1() :
			m_state(detail::get_offset_basis())
		{
		}
		void add_to_hash(void const* const& buff, unsigned const& size)
		{
			for(unsigned i = 0; i != size; ++i)
			{
				m_state = m_state * detail::get_prime();
				m_state = m_state ^ reinterpret_cast<unsigned char const*>(buff)[i];
			}
		}
		std::size_t finish()
		{
			return m_state;
		}
	private:
		std::size_t m_state;
	};

	class fnv1a
	{
	public:
		fnv1a() :
			m_state(detail::get_offset_basis())
		{
		}
		void add_to_hash(void const* const& buff, unsigned const& size)
		{
			for(unsigned i = 0; i != size; ++i)
			{
				m_state = m_state ^ reinterpret_cast<unsigned char const*>(buff)[i];
				m_state = m_state * detail::get_prime();
			}
		}
		std::size_t finish()
		{
			return m_state;
		}
	private:
		std::size_t m_state;
	};

}





#include <array>
#include <vector>


namespace mk
{

	template<typename T> void add_to_hash(T& hshr, char const& e){ hshr.add_to_hash(&e, sizeof(e)); }
	template<typename T> void add_to_hash(T& hshr, signed char const& e){ hshr.add_to_hash(&e, sizeof(e)); }
	template<typename T> void add_to_hash(T& hshr, unsigned char const& e){ hshr.add_to_hash(&e, sizeof(e)); }
	template<typename T> void add_to_hash(T& hshr, signed short int const& e){ hshr.add_to_hash(&e, sizeof(e)); }
	template<typename T> void add_to_hash(T& hshr, unsigned short int const& e){ hshr.add_to_hash(&e, sizeof(e)); }
	template<typename T> void add_to_hash(T& hshr, signed int const& e){ hshr.add_to_hash(&e, sizeof(e)); }
	template<typename T> void add_to_hash(T& hshr, unsigned int const& e){ hshr.add_to_hash(&e, sizeof(e)); }
	template<typename T> void add_to_hash(T& hshr, signed long int const& e){ hshr.add_to_hash(&e, sizeof(e)); }
	template<typename T> void add_to_hash(T& hshr, unsigned long int const& e){ hshr.add_to_hash(&e, sizeof(e)); }
	template<typename T> void add_to_hash(T& hshr, signed long long int const& e){ hshr.add_to_hash(&e, sizeof(e)); }
	template<typename T> void add_to_hash(T& hshr, unsigned long long int const& e){ hshr.add_to_hash(&e, sizeof(e)); }
	template<typename T> void add_to_hash(T& hshr, float const& e){ hshr.add_to_hash(&e, sizeof(e)); }
	template<typename T> void add_to_hash(T& hshr, double const& e){ hshr.add_to_hash(&e, sizeof(e)); }
	template<typename T> void add_to_hash(T& hshr, long double const& e){ hshr.add_to_hash(&e, sizeof(e)); }
	template<typename T> void add_to_hash(T& hshr, void const* const& e){ hshr.add_to_hash(&e, sizeof(e)); }

	template<typename T, typename E, unsigned N> void add_to_hash(T& hshr, std::array<E, N> const& arr){ for(auto const& e : arr){ add_to_hash(hshr, e); } }
	template<typename T, typename E> void add_to_hash(T& hshr, std::vector<T> const& vec){ for(auto const&& e : vec){ add_to_hash(hshr, e); } }

	template<typename T> std::size_t finish(T& hshr){ return hshr.finish(); }

}





namespace mk
{

	template<typename T>
	class std_hasher
	{
	public:
		template<typename TT>
		std::size_t operator()(TT const& tt) const
		{
			hasher<T> hshr{T{}};
			add_to_hash(hshr, tt);
			return finish(hshr);
		}
	};

}


#endif
#endif
