#pragma once
#ifndef Hasher_h_included
#define Hasher_h_included


#include <cstddef> // size_t
#include <memory> // addressof
#include <vector>
#include <string>
#include <utility> // pair



template<typename T>struct idnt{ typedef T type; };

template<typename T>struct strct{};
template<>struct strct<char                  	> : idnt<char                  	> { template<typename HasherT>void operator()(HasherT& hasher, typename type const* const& pt, ::std::size_t const& n){ hasher.add_data(reinterpret_cast<unsigned char const*>(pt), n * sizeof(typename type)); } };
template<>struct strct<signed char           	> : idnt<signed char           	> { template<typename HasherT>void operator()(HasherT& hasher, typename type const* const& pt, ::std::size_t const& n){ hasher.add_data(reinterpret_cast<unsigned char const*>(pt), n * sizeof(typename type)); } };
template<>struct strct<unsigned char         	> : idnt<unsigned char         	> { template<typename HasherT>void operator()(HasherT& hasher, typename type const* const& pt, ::std::size_t const& n){ hasher.add_data(reinterpret_cast<unsigned char const*>(pt), n * sizeof(typename type)); } };
template<>struct strct<wchar_t               	> : idnt<wchar_t               	> { template<typename HasherT>void operator()(HasherT& hasher, typename type const* const& pt, ::std::size_t const& n){ hasher.add_data(reinterpret_cast<unsigned char const*>(pt), n * sizeof(typename type)); } };
template<>struct strct<char16_t              	> : idnt<char16_t              	> { template<typename HasherT>void operator()(HasherT& hasher, typename type const* const& pt, ::std::size_t const& n){ hasher.add_data(reinterpret_cast<unsigned char const*>(pt), n * sizeof(typename type)); } };
template<>struct strct<char32_t              	> : idnt<char32_t              	> { template<typename HasherT>void operator()(HasherT& hasher, typename type const* const& pt, ::std::size_t const& n){ hasher.add_data(reinterpret_cast<unsigned char const*>(pt), n * sizeof(typename type)); } };
template<>struct strct<signed short int      	> : idnt<signed short int      	> { template<typename HasherT>void operator()(HasherT& hasher, typename type const* const& pt, ::std::size_t const& n){ hasher.add_data(reinterpret_cast<unsigned char const*>(pt), n * sizeof(typename type)); } };
template<>struct strct<unsigned short int    	> : idnt<unsigned short int    	> { template<typename HasherT>void operator()(HasherT& hasher, typename type const* const& pt, ::std::size_t const& n){ hasher.add_data(reinterpret_cast<unsigned char const*>(pt), n * sizeof(typename type)); } };
template<>struct strct<signed int            	> : idnt<signed int            	> { template<typename HasherT>void operator()(HasherT& hasher, typename type const* const& pt, ::std::size_t const& n){ hasher.add_data(reinterpret_cast<unsigned char const*>(pt), n * sizeof(typename type)); } };
template<>struct strct<unsigned int          	> : idnt<unsigned int          	> { template<typename HasherT>void operator()(HasherT& hasher, typename type const* const& pt, ::std::size_t const& n){ hasher.add_data(reinterpret_cast<unsigned char const*>(pt), n * sizeof(typename type)); } };
template<>struct strct<signed long int       	> : idnt<signed long int       	> { template<typename HasherT>void operator()(HasherT& hasher, typename type const* const& pt, ::std::size_t const& n){ hasher.add_data(reinterpret_cast<unsigned char const*>(pt), n * sizeof(typename type)); } };
template<>struct strct<unsigned long int     	> : idnt<unsigned long int     	> { template<typename HasherT>void operator()(HasherT& hasher, typename type const* const& pt, ::std::size_t const& n){ hasher.add_data(reinterpret_cast<unsigned char const*>(pt), n * sizeof(typename type)); } };
template<>struct strct<signed long long int  	> : idnt<signed long long int  	> { template<typename HasherT>void operator()(HasherT& hasher, typename type const* const& pt, ::std::size_t const& n){ hasher.add_data(reinterpret_cast<unsigned char const*>(pt), n * sizeof(typename type)); } };
template<>struct strct<unsigned long long int	> : idnt<unsigned long long int	> { template<typename HasherT>void operator()(HasherT& hasher, typename type const* const& pt, ::std::size_t const& n){ hasher.add_data(reinterpret_cast<unsigned char const*>(pt), n * sizeof(typename type)); } };
template<>struct strct<float                 	> : idnt<float                 	> { template<typename HasherT>void operator()(HasherT& hasher, typename type const* const& pt, ::std::size_t const& n){ hasher.add_data(reinterpret_cast<unsigned char const*>(pt), n * sizeof(typename type)); } };
template<>struct strct<double                	> : idnt<double                	> { template<typename HasherT>void operator()(HasherT& hasher, typename type const* const& pt, ::std::size_t const& n){ hasher.add_data(reinterpret_cast<unsigned char const*>(pt), n * sizeof(typename type)); } };
template<>struct strct<long double           	> : idnt<long double           	> { template<typename HasherT>void operator()(HasherT& hasher, typename type const* const& pt, ::std::size_t const& n){ hasher.add_data(reinterpret_cast<unsigned char const*>(pt), n * sizeof(typename type)); } };

template<typename HasherT, typename T>                 	void add_to_hash(HasherT& hasher, T const& t                                	){ strct<T>{}(hahser, ::std::addressof(t), 1); } // single element
template<typename HasherT, typename T, ::std::size_t N>	void add_to_hash(HasherT& hasher, const T(&arr)[N]                          	){ strct<T>{}(hasher, arr, N); } // array
template<typename HasherT, typename T, ::std::size_t N>	void add_to_hash(HasherT& hasher, const T(&arr)[N], ::std::size_t const& n  	){ strct<T>{}(hasher, arr, n); } // first few element from an array
template<typename HasherT, typename T>                 	void add_to_hash(HasherT& hasher, T const* const& pt, ::std::size_t const& n	){ strct<T>{}(hasher, pt, n); } // pointer and number of elements
template<typename HasherT, typename T>                 	void add_to_hash(HasherT& hasher, T* const& pt, ::std::size_t const& n      	){ strct<T>{}(hasher, pt, n); } // non-const pointer and number of elements

template<typename HasherT, typename T>            	void add_to_hash(HasherT& hahser, ::std::vector<T> const& vec      	){ add_to_hash(hahser, vec.data(), vec.size()); }
template<typename HasherT, typename T>            	void add_to_hash(HasherT& hahser, ::std::basic_string<T> const& str	){ add_to_hash(hahser, str.data(), str.size()); }
template<typename HasherT, typename T, typename U>	void add_to_hash(HasherT& hahser, ::std::pair<T, U> const& pair    	){ add_to_hash(hahser, pair.first); add_to_hash(pair.second); }


#endif
