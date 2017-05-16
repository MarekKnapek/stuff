#include <iostream>
#include <array>
#include <string>


namespace constexpr_xor_array
{


	namespace details
	{

		template<::std::size_t...>
		struct index_list
		{
		};

		template<typename index_list, ::std::size_t>
		struct appender
		{
		};

		template<::std::size_t... left, ::std::size_t right>
		struct appender<index_list<left...>, right>
		{
			typedef index_list<left..., right> type;
		};

		template<::std::size_t N>
		struct make_index_list
		{
			typedef typename appender<typename make_index_list<N - 1>::type, N - 1>::type type;
		};

		template<>
		struct make_index_list<0>
		{
			typedef index_list<> type;
		};

	}


	template<typename T, ::std::size_t N>
	class xor_array
	{
	private:
		template<typename>
		class xor_array_inner
		{
		};
		template<::std::size_t... Idxs>
		class xor_array_inner<constexpr_xor_array::details::index_list<Idxs...>>
		{
		public:
			template<::std::size_t M>
			constexpr xor_array_inner(const T(&arr)[M]) :
				m_value{encrypt_elem(arr, M, Idxs, 0)...}
			{
			}
		public:
			T operator[](::std::size_t const& idx) const
			{
				return decrypt_elem(idx, 0);
			}
		private:
			constexpr static T encrypt_elem(T const* const& arr, ::std::size_t const& M, ::std::size_t const& idx, ::std::size_t const& inner_idx)
			{
				return inner_idx == sizeof(T) ? T{0} : encrypt_elem(arr, M, idx, inner_idx + 1) | encrypt_byte(((idx < M ? arr[idx] : T{0}) >> (inner_idx * CHAR_BIT) & 0xFF), idx * sizeof(T) + inner_idx) << (inner_idx * CHAR_BIT);
			}
			constexpr static unsigned char encrypt_byte(T const& byte, ::std::size_t const& idx)
			{
				return static_cast<unsigned char>((static_cast<unsigned char>(byte & 0xFF) ^ static_cast<unsigned char>((s_secret_random_xor_key + idx) & 0xFF)) & 0xFF);
			}
			T decrypt_elem(::std::size_t const& idx, ::std::size_t const& inner_idx) const
			{
				return inner_idx == sizeof(T) ? T{0} : decrypt_elem(idx, inner_idx + 1) | decrypt_byte((m_value[idx] >> (inner_idx * CHAR_BIT) & 0xFF), idx * sizeof(T) + inner_idx) << (inner_idx * CHAR_BIT);
			}
			static unsigned char decrypt_byte(T const& byte, ::std::size_t const& idx)
			{
				return static_cast<unsigned char>((static_cast<unsigned char>(byte & 0xFF) ^ static_cast<unsigned char>((s_secret_random_xor_key + idx) & 0xFF)) & 0xFF);
			}
		private:
			constexpr static unsigned char const s_secret_random_xor_key = 42;
		private:
			::std::array<T const, N> const m_value;
		};
	public:
		template<::std::size_t M>
		constexpr xor_array(const T(&arr)[M]) :
			m_inner(arr)
		{
		}
	public:
		constexpr ::std::size_t size() const
		{
			return N;
		}
		T operator[](::std::size_t const& idx) const
		{
			return m_inner[idx];
		}
	private:
		xor_array_inner<typename constexpr_xor_array::details::make_index_list<N>::type> const m_inner;
	};


}


int main()
{
	static constexpr const constexpr_xor_array::xor_array<unsigned int, 100> hidden_arr_1{{0x12345678, 2, 3, 0x87654321}};
	for(::std::size_t i = 0; i != hidden_arr_1.size(); ++i){
		::std::cout << ::std::hex << hidden_arr_1[i] << " ";
	}
	::std::cout << ::std::endl;

	static constexpr const constexpr_xor_array::xor_array<char, 100> hidden_arr_2{"MarekKnapek"};
	for(::std::size_t i = 0; i != hidden_arr_2.size(); ++i){
		::std::cout << hidden_arr_2[i];
	}
	::std::cout << ::std::endl;

	static constexpr const constexpr_xor_array::xor_array<wchar_t, 100> hidden_arr_3{L"MarekKnapek"};
	for(::std::size_t i = 0; i != hidden_arr_3.size(); ++i){
		::std::wcout << hidden_arr_3[i];
	}
	::std::wcout << ::std::endl;
}
