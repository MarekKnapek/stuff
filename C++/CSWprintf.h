#pragma once
#ifndef CSW_PRINTF_INCLUDED
#define CSW_PRINTF_INCLUDED


#include <cstdarg>
#include <cstdio>


#ifndef CSW_WANT_RUNTIME_CHECKED_PRINTF
# define CSW_WANT_RUNTIME_CHECKED_PRINTF 1
#endif


#if _MSC_VER == 1600 // MSVC++ 10.0, Visual Studio 2010


template<typename CharT>
void CSWCheckFormat(const CharT* const /* unused */, ...)
{
}

inline void CSWCheckFormat(const UINT /* unused */, ...)
{
}

inline CString CSWFormatCString(const UINT nFormatID, ...)
{
	CString sFormat;
	const BOOL bLoaded = sFormat.LoadString(nFormatID);
	ASSERT(bLoaded);
	CString ret;
	va_list va;
	va_start(va, nFormatID);
	ret.FormatV(sFormat.GetString(), va);
	va_end(va);
	return ret;
}

inline CStringA CSWFormatCStringA(const UINT nFormatID, ...)
{
	CStringA sFormat;
	const BOOL bLoaded = sFormat.LoadString(nFormatID);
	ASSERT(bLoaded);
	CStringA ret;
	va_list va;
	va_start(va, nFormatID);
	ret.FormatV(sFormat.GetString(), va);
	va_end(va);
	return ret;
}

inline CString CSWFormatCString(const wchar_t* const sFormat, ...)
{
	CString ret;
	va_list va;
	va_start(va, sFormat);
	ret.FormatV(sFormat, va);
	va_end(va);
	return ret;
}

inline CStringA CSWFormatCString(const char* const sFormat, ...)
{
	CStringA ret;
	va_list va;
	va_start(va, sFormat);
	ret.FormatV(sFormat, va);
	va_end(va);
	return ret;
}


#elif _MSC_VER == 1900 // MSVC++ 14.0, Visual Studio 2015


#if CSW_WANT_RUNTIME_CHECKED_PRINTF == 1


#include <limits>
#include <string>
#include <type_traits>


#ifndef ASSERT
# pragma message("CSWprintf.h: Warning, no ASSERT macro defined, using custom ASSERT that will crash the application.")
# define CSW_PRINTF_CUSTOM_ASSERT 1
# define ASSERT(A) do{ if(!(A)){ char const* volatile* const p = nullptr; *p = #A; } }while(false)
#else
# define CSW_PRINTF_CUSTOM_ASSERT 0
#endif

#ifndef ASSERT2
# pragma message("CSWprintf.h: Warning, no ASSERT2 macro defined, using custom ASSERT2 that will crash the application.")
# define CSW_PRINTF_CUSTOM_ASSERT2 1
# define ASSERT2(A, B) do{ if(!(A)){ wchar_t const* volatile* const p = nullptr; *p = B; } }while(false)
#else
# define CSW_PRINTF_CUSTOM_ASSERT2 0
#endif


typedef int CSWCheckFormatNumber;
typedef CSWCheckFormatNumber CSWCheckFormatWidth;
typedef CSWCheckFormatNumber CSWCheckFormatPrecision;


enum class ECSWCheckFormatFlag : unsigned
{
	Default = 0u,
	Minus = 1u << 0u,
	Plus = 1u << 1u,
	Space = 1u << 2u,
	Zero = 1u << 3u,
	Hash = 1u << 4u
};

inline ECSWCheckFormatFlag operator|(const ECSWCheckFormatFlag& a, const ECSWCheckFormatFlag b)
{
	return static_cast<ECSWCheckFormatFlag>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}

inline ECSWCheckFormatFlag operator|=(ECSWCheckFormatFlag& a, const ECSWCheckFormatFlag& b)
{
	a = a | b;
	return a;
}

static const CSWCheckFormatNumber s_eCSWCheckFormatConsumeNumberDefault = (std::numeric_limits<CSWCheckFormatNumber>::max)() - 1;

static const CSWCheckFormatWidth s_eCSWCheckFormatWidthDefault = (std::numeric_limits<CSWCheckFormatNumber>::max)() - 2;
static const CSWCheckFormatWidth s_eCSWCheckFormatWidthAsterisk = (std::numeric_limits<CSWCheckFormatNumber>::max)() - 3;

static const CSWCheckFormatPrecision s_eCSWCheckFormatPrecisionDefault = (std::numeric_limits<CSWCheckFormatPrecision>::max)() - 4;
static const CSWCheckFormatPrecision s_eCSWCheckFormatPrecisionAsterisk = (std::numeric_limits<CSWCheckFormatPrecision>::max)() - 5;

enum class ECSWCheckFormatLength : unsigned
{
	hh,
	h,
	none,
	l,
	ll,
	j,
	z,
	t,
	L
};

enum class ECSWCheckFormatType : unsigned
{
	unspecified,
	c,
	s,
	d,
	i,
	o,
	x,
	X,
	u,
	f,
	F,
	e,
	E,
	a,
	A,
	g,
	G,
	n,
	p
};


inline void CSWCheckFormatReportError(const wchar_t* const sMsg)
{
	ASSERT2(false, sMsg);
}

template<typename CharT>
ECSWCheckFormatFlag CSWCheckFormatConsumeFlags(const CharT*& sFormat)
{
	ECSWCheckFormatFlag eFlag = ECSWCheckFormatFlag::Default;
	for(; *sFormat; ++sFormat)
	{
		switch(*sFormat)
		{
		case CharT('-'):
			eFlag |= ECSWCheckFormatFlag::Minus;
			break;
		case CharT('+'):
			eFlag |= ECSWCheckFormatFlag::Plus;
			break;
		case CharT(' '):
			eFlag |= ECSWCheckFormatFlag::Space;
			break;
		case CharT('0'):
			eFlag |= ECSWCheckFormatFlag::Zero;
			break;
		case ('#'):
			eFlag |= ECSWCheckFormatFlag::Hash;
			break;
		default:
			return eFlag;
			break;
		}
	}
	CSWCheckFormatReportError(L"Format string too short.");
	return ECSWCheckFormatFlag{};
}

template<typename CharT>
CSWCheckFormatNumber CSWCheckFormatConsumeNumber(const CharT*& sFormat)
{
	bool bPositive = true;
	if(*sFormat == CharT('-'))
	{
		bPositive = false;
		++sFormat;
	}
	const CharT* const itBegin = sFormat;
	for(; *sFormat >= CharT('0') && *sFormat <= CharT('9'); ++sFormat)
	{
	}
	const CharT* itEnd = sFormat - 1;
	if(itEnd >= itBegin)
	{
		CSWCheckFormatNumber nNumber = 0;
		int nMagnitude = 1;
		for(; itEnd >= itBegin; --itEnd)
		{
			nNumber += (*itEnd - CharT('0')) * nMagnitude;
			nMagnitude *= 10;
		}
		if(!bPositive)
		{
			nNumber = -nNumber;
		}
		return nNumber;
	}
	return s_eCSWCheckFormatConsumeNumberDefault;
}

template<typename CharT>
CSWCheckFormatWidth CSWCheckFormatConsumeWidth(const CharT*& sFormat)
{
	if(*sFormat == CharT('*'))
	{
		++sFormat;
		return s_eCSWCheckFormatWidthAsterisk;
	}
	const CSWCheckFormatNumber nNumber = CSWCheckFormatConsumeNumber(sFormat);
	if(nNumber == s_eCSWCheckFormatConsumeNumberDefault)
	{
		return s_eCSWCheckFormatWidthDefault;
	}
	else
	{
		const CSWCheckFormatWidth nWidth = nNumber;
		return nWidth;
	}
}

template<typename CharT>
CSWCheckFormatPrecision CSWCheckFormatConsumePrecision(const CharT*& sFormat)
{
	if(*sFormat == CharT('.'))
	{
		++sFormat;
		if(*sFormat == CharT('*'))
		{
			++sFormat;
			return s_eCSWCheckFormatPrecisionAsterisk;
		}
		const CSWCheckFormatNumber nNumber = CSWCheckFormatConsumeNumber(sFormat);
		if(nNumber == s_eCSWCheckFormatConsumeNumberDefault)
		{
			return s_eCSWCheckFormatPrecisionDefault;
		}
		else
		{
			const CSWCheckFormatPrecision nPrecision = nNumber;
			return nPrecision;
		}
	}
	return s_eCSWCheckFormatPrecisionDefault;
}

template<typename CharT>
ECSWCheckFormatLength CSWCheckFormatConsumeLength(const CharT*& sFormat)
{
	ECSWCheckFormatLength eLength = ECSWCheckFormatLength::none;
	switch(*sFormat)
	{
	case CharT('h'):
		eLength = ECSWCheckFormatLength::h;
		if(*(sFormat + 1) == CharT('h'))
		{
			++sFormat;
			eLength = ECSWCheckFormatLength::hh;
		}
		break;
	case CharT('l'):
		eLength = ECSWCheckFormatLength::l;
		if(*(sFormat + 1) == CharT('l'))
		{
			++sFormat;
			eLength = ECSWCheckFormatLength::ll;
		}
		break;
	case CharT('j'):
		eLength = ECSWCheckFormatLength::j;
		break;
	case CharT('z'):
		eLength = ECSWCheckFormatLength::z;
		break;
	case CharT('t'):
		eLength = ECSWCheckFormatLength::t;
		break;
	case CharT('L'):
		eLength = ECSWCheckFormatLength::L;
		break;
	default:
		--sFormat;
		break;
	}
	++sFormat;
	return eLength;
}

template<typename CharT>
ECSWCheckFormatType CSWCheckFormatConsumeType(const CharT*& sFormat)
{
	ECSWCheckFormatType eType = ECSWCheckFormatType::unspecified;
	switch(*sFormat)
	{
	case CharT('c'):
		eType = ECSWCheckFormatType::c;
		break;
	case CharT('s'):
		eType = ECSWCheckFormatType::s;
		break;
	case CharT('d'):
		eType = ECSWCheckFormatType::d;
		break;
	case CharT('i'):
		eType = ECSWCheckFormatType::i;
		break;
	case CharT('o'):
		eType = ECSWCheckFormatType::o;
		break;
	case CharT('x'):
		eType = ECSWCheckFormatType::x;
		break;
	case CharT('X'):
		eType = ECSWCheckFormatType::X;
		break;
	case CharT('u'):
		eType = ECSWCheckFormatType::u;
		break;
	case CharT('f'):
		eType = ECSWCheckFormatType::f;
		break;
	case CharT('F'):
		eType = ECSWCheckFormatType::F;
		break;
	case CharT('e'):
		eType = ECSWCheckFormatType::e;
		break;
	case CharT('E'):
		eType = ECSWCheckFormatType::E;
		break;
	case CharT('a'):
		eType = ECSWCheckFormatType::a;
		break;
	case CharT('A'):
		eType = ECSWCheckFormatType::A;
		break;
	case CharT('g'):
		eType = ECSWCheckFormatType::g;
		break;
	case CharT('G'):
		eType = ECSWCheckFormatType::G;
		break;
	case CharT('n'):
		eType = ECSWCheckFormatType::n;
		break;
	case CharT('p'):
		eType = ECSWCheckFormatType::p;
		break;
	default:
		--sFormat;
		break;
	}
	++sFormat;
	return eType;
}


template<typename T>
class csw_remove_const
{
public:
	typedef T type;
};

template<typename T>
class csw_remove_const<const T>
{
public:
	typedef T type;
};

template<typename T>
using csw_remove_const_t = typename csw_remove_const<T>::type;


template<typename T>
class csw_remove_reference
{
public:
	typedef T type;
};

template<typename T>
class csw_remove_reference<T&>
{
public:
	typedef T type;
};

template<typename T>
using csw_remove_reference_t = typename csw_remove_reference<T>::type;


class csw_true_type
{
public:
	static const bool value = true;
};

class csw_false_type
{
public:
	static const bool value = false;
};


template<typename T, typename U>
class csw_is_same_type
{
public:
	typedef csw_false_type type;
};

template<typename T>
class csw_is_same_type<T, T>
{
public:
	typedef csw_true_type type;
};

template<typename T, typename U>
using csw_is_same_type_t = typename csw_is_same_type<csw_remove_const_t<csw_remove_reference_t<T>>, csw_remove_const_t<csw_remove_reference_t<U>>>::type;


template<typename T, typename U>
class csw_or
{
public:
	typedef csw_false_type type;
};

template<typename T>
class csw_or<T, csw_true_type>
{
public:
	typedef csw_true_type type;
};

template<typename T>
class csw_or<csw_true_type, T>
{
public:
	typedef csw_true_type type;
};

template<typename T, typename U>
using csw_or_t = typename csw_or<T, U>::type;


template<typename T, typename EnableT = void>
class csw_convert
{
public:
	const T operator()(const T& t) const
	{
		return t;
	}
public:
	typedef T type;
};

template<typename CharT, typename TraitsT>
class csw_convert<ATL::CStringT<CharT, TraitsT>>
{
public:
	typename ATL::CStringT<CharT, TraitsT>::PCXSTR operator()(const ATL::CStringT<CharT, TraitsT>& t) const
	{
		return static_cast<typename ATL::CStringT<CharT, TraitsT>::PCXSTR>(t);
	}
public:
	typedef typename ATL::CStringT<CharT, TraitsT>::PCXSTR type;
};

template<typename T>
class csw_convert<T, std::enable_if_t<std::is_enum<T>::value>>
{
public:
	std::underlying_type_t<T> operator()(const T& t) const
	{
		return static_cast<std::underlying_type_t<T>>(t);
	}
public:
	typedef std::underlying_type_t<T> type;
};

template<typename T, unsigned N>
class csw_convert<T[N]>
{
public:
	const T* operator()(const T(&t)[N]) const
	{
		return t;
	}
public:
	typedef const T* type;
};

template<typename T>
class csw_convert<T[]>
{
public:
	const T* operator()(const T(&t)[]) const
	{
		return t;
	}
public:
	typedef const T* type;
};

template<typename T>
using csw_convert_t = typename csw_convert<T>::type;

template<typename T>
csw_convert_t<T> CSWCheckFormatConvert(const T& t)
{
	return csw_convert<csw_remove_const_t<T>>()(t);
}


template<typename T>
class csw_to_int
{
public:
	int operator()(const T& t) const
	{
		ASSERT(false);
		return 0;
	}
};

template<>
class csw_to_int<int>
{
public:
	int operator()(const int& t) const
	{
		return t;
	}
};


#pragma warning(push)
#pragma warning(disable:4793) // 'CSWCheckFormatContinue': function compiled as native: varargs not supported under /clr
inline void CSWCheckFormatContinue(...)
{
	CSWCheckFormatReportError(L"Too few parameters.");
}
#pragma warning(pop)

template<typename CharT, typename T, typename... ArgsT>
void CSWCheckFormatContinue
(
	const ECSWCheckFormatFlag& eFlags,
	const CSWCheckFormatWidth& nWidth,
	const CSWCheckFormatPrecision& nPrecision,
	const ECSWCheckFormatLength& eLength,
	const ECSWCheckFormatType& eType,
	const CharT* sFormat,
	const T& t,
	const ArgsT&... args
)
{
	if(nWidth == s_eCSWCheckFormatWidthAsterisk)
	{
		if(!csw_is_same_type_t<decltype(t), int>::value)
		{
			CSWCheckFormatReportError(L"Width specified with asterisk, int expected.");
		}
		return CSWCheckFormatContinue(eFlags, csw_to_int<T>()(t), nPrecision, eLength, eType, sFormat, args...);
	}
	if(nPrecision == s_eCSWCheckFormatPrecisionAsterisk)
	{
		if(!csw_is_same_type_t<decltype(t), int>::value)
		{
			CSWCheckFormatReportError(L"Precision specified with asterisk, int expected.");
		}
		return CSWCheckFormatContinue(eFlags, nWidth, csw_to_int<T>()(t), eLength, eType, sFormat, args...);
	}
	switch(eType)
	{
	case ECSWCheckFormatType::c:
		{
			// Must be int or convertible to int, not sure about unsigned int.
			// Or must be l length and wint_t type.
			bool bOK = false;
			if(eLength == ECSWCheckFormatLength::none)
			{
				if
				(
					csw_is_same_type_t<decltype(t), char>::value ||
					csw_is_same_type_t<decltype(t), signed char>::value ||
					csw_is_same_type_t<decltype(t), unsigned char>::value ||
					csw_is_same_type_t<decltype(t), signed short>::value ||
					csw_is_same_type_t<decltype(t), unsigned short>::value ||
					csw_is_same_type_t<decltype(t), signed int>::value ||
					csw_is_same_type_t<decltype(t), unsigned int>::value
				)
				{
					bOK = true;
				}
			}
			if(eLength == ECSWCheckFormatLength::l && csw_is_same_type_t<decltype(t), std::wint_t>::value)
			{
				bOK = true;
			}
			// wchar_t
			if(eLength == ECSWCheckFormatLength::l && csw_is_same_type_t<decltype(t), wchar_t>::value)
			{
				bOK = true;
			}
			// Wide char format string can accept wchar_t argument with %c conversion specifier.
			if(csw_is_same_type_t<CharT, wchar_t>::value && eLength == ECSWCheckFormatLength::none && csw_is_same_type_t<decltype(t), wchar_t>::value)
			{
				bOK = true;
			}
			if(!bOK)
			{
				CSWCheckFormatReportError(L"Type c must have integer argument.");
			}
		}
		break;
	case ECSWCheckFormatType::s:
		{
			// String.
			bool bOK = false;
			// Wide %s can accept wide strings. Narrow %s can accept narrow strings.
			if
			(
				eLength == ECSWCheckFormatLength::none &&
				(
					(csw_is_same_type_t<CharT, char>::value && (csw_is_same_type_t<decltype(t), char*>::value || csw_is_same_type_t<decltype(t), const char*>::value)) ||
					(csw_is_same_type_t<CharT, wchar_t>::value && (csw_is_same_type_t<decltype(t), wchar_t*>::value || csw_is_same_type_t<decltype(t), const wchar_t*>::value))
				)
			)
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::l && (csw_is_same_type_t<decltype(t), wchar_t*>::value || csw_is_same_type_t<decltype(t), const wchar_t*>::value))
			{
				bOK = true;
			}
			if(!bOK)
			{
				CSWCheckFormatReportError(L"Type s must have string argument.");
			}
		}
		break;
	case ECSWCheckFormatType::d:
	case ECSWCheckFormatType::i:
		{
			// Must be signed integer, not sure about 'z' length modifier for signed size_t.
			bool bOK = false;
			if(eLength == ECSWCheckFormatLength::hh && csw_is_same_type_t<decltype(t), signed char>::value)
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::h && csw_is_same_type_t<decltype(t), signed short>::value)
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::none && csw_is_same_type_t<decltype(t), signed int>::value)
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::l && csw_is_same_type_t<decltype(t), signed long>::value)
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::ll && csw_is_same_type_t<decltype(t), signed long long>::value)
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::j && csw_is_same_type_t<decltype(t), intmax_t>::value)
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::t && csw_is_same_type_t<decltype(t), ptrdiff_t>::value)
			{
				bOK = true;
			}
			// To integer promotion.
			if
			(
				eLength == ECSWCheckFormatLength::none &&
				(
					csw_is_same_type_t<decltype(t), char>::value ||
					csw_is_same_type_t<decltype(t), signed char>::value ||
					csw_is_same_type_t<decltype(t), unsigned char>::value ||
					csw_is_same_type_t<decltype(t), signed short>::value ||
					csw_is_same_type_t<decltype(t), unsigned short>::value
				)
			)
			{
				bOK = true;
			}
			if(!bOK)
			{
				CSWCheckFormatReportError(L"Type d and i must have signed integer argument.");
			}
		}
		break;
	case ECSWCheckFormatType::o:
	case ECSWCheckFormatType::x:
	case ECSWCheckFormatType::X:
	case ECSWCheckFormatType::u:
		{
			// Must be unsigned integer, not sure about 't' length modifier for unsigned ptrdiff_t.
			bool bOK = false;
			if(eLength == ECSWCheckFormatLength::hh && csw_is_same_type_t<decltype(t), unsigned char>::value)
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::h && csw_is_same_type_t<decltype(t), unsigned short>::value)
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::none && csw_is_same_type_t<decltype(t), unsigned int>::value)
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::l && csw_is_same_type_t<decltype(t), unsigned long>::value)
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::ll && csw_is_same_type_t<decltype(t), unsigned long long>::value)
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::j && csw_is_same_type_t<decltype(t), std::uintmax_t>::value)
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::z && csw_is_same_type_t<decltype(t), std::size_t>::value)
			{
				bOK = true;
			}
			if(!bOK)
			{
				CSWCheckFormatReportError(L"Types o, x, X and u must have unsigned integer argument.");
			}
		}
		break;
	case ECSWCheckFormatType::f:
	case ECSWCheckFormatType::F:
	case ECSWCheckFormatType::e:
	case ECSWCheckFormatType::E:
	case ECSWCheckFormatType::a:
	case ECSWCheckFormatType::A:
	case ECSWCheckFormatType::g:
	case ECSWCheckFormatType::G:
		{
			bool bOK = false;
			if(eLength == ECSWCheckFormatLength::none && (csw_is_same_type_t<decltype(t), float>::value || csw_is_same_type_t<decltype(t), double>::value))
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::l && csw_is_same_type_t<decltype(t), double>::value)
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::L && csw_is_same_type_t<decltype(t), long double>::value)
			{
				bOK = true;
			}
			if(!bOK)
			{
				CSWCheckFormatReportError(L"Types f, F, e, E, a, A, g and G must have floating point number argument.");
			}
		}
		break;
	case ECSWCheckFormatType::n:
		{
			// Not sure about 'z' length modifier for signed size_t.
			bool bOK = false;
			if(eLength == ECSWCheckFormatLength::hh && csw_is_same_type_t<decltype(t), signed char*>::value)
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::h && csw_is_same_type_t<decltype(t), short*>::value)
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::none && csw_is_same_type_t<decltype(t), int*>::value)
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::l && csw_is_same_type_t<decltype(t), long*>::value)
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::ll && csw_is_same_type_t<decltype(t), long long*>::value)
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::j && csw_is_same_type_t<decltype(t), std::intmax_t*>::value)
			{
				bOK = true;
			}
			if(eLength == ECSWCheckFormatLength::t && csw_is_same_type_t<decltype(t), std::ptrdiff_t*>::value)
			{
				bOK = true;
			}
			if(!bOK)
			{
				CSWCheckFormatReportError(L"Invalid argument to n type.");
			}
		}
		break;
	case ECSWCheckFormatType::p:
		{
			bool bOK = false;
			if(eLength == ECSWCheckFormatLength::none && csw_is_same_type_t<decltype(t), void*>::value)
			{
				bOK = true;
			}
			if(!bOK)
			{
				CSWCheckFormatReportError(L"p type must have pointer argument.");
			}
		}
		break;
	case ECSWCheckFormatType::unspecified:
		CSWCheckFormatReportError(L"Unknown data type.");
		break;
	default:
		ASSERT(false);
		break;
	}
	return CSWCheckFormat(sFormat, args...);
}

template<typename CharT>
void CSWCheckFormat(const CharT* sFormat)
{
	for(; *sFormat; ++sFormat)
	{
		if(*sFormat != CharT('%') || *++sFormat == CharT('%'))
		{
			continue;
		}
		CSWCheckFormatReportError(L"Invalid format character.");
	}
}

template<typename CharT, typename ArgT, typename... ArgsT>
void CSWCheckFormat(const CharT* sFormat, const ArgT& head, const ArgsT&... tail)
{
	// This function peels one argument out of argument pack (known as 'head')
	// processes it and later calls recursive itself with rest (known as 'tail').
	// This recursive call peels one argument out of argument pack (known as 'head')
	// processes it and later calls recursive itself with rest (known as 'tail').
	// ...
	const auto& t = CSWCheckFormatConvert(head);

	for(; *sFormat; ++sFormat)
	{
		if(*sFormat != CharT('%') || *++sFormat == CharT('%'))
		{
			continue;
		}
		const auto& eFlags = CSWCheckFormatConsumeFlags(sFormat);
		const auto& nWidth = CSWCheckFormatConsumeWidth(sFormat);
		const auto& nPrecision = CSWCheckFormatConsumePrecision(sFormat);
		const auto& eLength = CSWCheckFormatConsumeLength(sFormat);
		const auto& eType = CSWCheckFormatConsumeType(sFormat);

		return CSWCheckFormatContinue(eFlags, nWidth, nPrecision, eLength, eType, sFormat, t, tail...);
	}
	CSWCheckFormatReportError(L"Too few format specifiers or too many parameters.");
}

template<typename... ArgsT>
void CSWCheckFormat(const UINT nFormatID, const ArgsT&... args)
{
	CString sFormat;
	const BOOL bLoaded = sFormat.LoadString(nFormatID);
	ASSERT(bLoaded);
	return CSWCheckFormat(sFormat.GetString(), args...);
}


#if CSW_PRINTF_CUSTOM_ASSERT == 1
# undef ASSERT
#endif
#undef CSW_PRINTF_CUSTOM_ASSERT

#if CSW_PRINTF_CUSTOM_ASSERT2 == 1
# undef ASSERT2
#endif
#undef CSW_PRINTF_CUSTOM_ASSERT2


#else // CSW_WANT_RUNTIME_CHECKED_PRINTF


inline void CSWCheckFormat(...)
{
}


#endif // CSW_WANT_RUNTIME_CHECKED_PRINTF


template<typename... ArgsT>
CString CSWFormatCString(const UINT nFormatID, const ArgsT&... args)
{
	CString sFormat;
	const BOOL bLoaded = sFormat.LoadString(nFormatID);
	ASSERT(bLoaded);
	CSWCheckFormat(nFormatID, args...);
	CString ret;
	ret.Format(sFormat, args...);
	return ret;
}

template<typename... ArgsT>
CStringA CSWFormatCStringA(const UINT nFormatID, const ArgsT&... args)
{
	CStringA sFormat;
	const BOOL bLoaded = sFormat.LoadString(nFormatID);
	ASSERT(bLoaded);
	CSWCheckFormat(nFormatID, args...);
	CStringA ret;
	ret.Format(sFormat, args...);
	return ret;
}

template<typename... ArgsT>
CString CSWFormatCString(const wchar_t* const sFormat, const ArgsT&... args)
{
	CSWCheckFormat(sFormat, args...);
	CString ret;
	ret.Format(sFormat, args...);
	return ret;
}

template<typename... ArgsT>
CStringA CSWFormatCString(const char* const sFormat, const ArgsT&... args)
{
	CSWCheckFormat(sFormat, args...);
	CStringA ret;
	ret.Format(sFormat, args...);
	return ret;
}


#else // _MSC_VER


#error Unknown Visual Studio.


#endif // _MSC_VER


#endif // CSW_PRINTF_INCLUDED
