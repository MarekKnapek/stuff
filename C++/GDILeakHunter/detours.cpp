#include "setup.h"
#if mk_should_enable == 1

#include "overrider.h"
#include "stack_tracer.h"
#include "symbol_locator.h"
#include "hasher.h"
#include "scope_exit.h"

#include <array>
#include <mutex>
#include <unordered_set>
#include <unordered_map>
#include <cassert>

#include <Windows.h>


HBITMAP APIENTRY NtGdiCreateBitmap(__in int cx, __in int cy, __in UINT cPlanes, __in UINT cBPP, __in_opt LPBYTE pjInit);
HBITMAP APIENTRY NtGdiCreateCompatibleBitmap(__in HDC hdc, __in int cx, __in int cy);
HDC APIENTRY NtGdiCreateCompatibleDC(__in_opt HDC hdc);
BOOL APIENTRY NtGdiDeleteObjectApp(__in HANDLE hobj);


class Redirector
{
public:
	template<typename T>
	Redirector(T orig, int prologue, T neu) :
		m_orig(mk::follow_jumps(mk::fn_ptr_to_ptr(orig))),
		m_prologue(prologue),
		m_buff()
	{
		assert(orig != neu);
		static unsigned const buff_size = 4 * 1024;
		m_buff = std::make_unique<char[]>(buff_size);
		std::memset(m_buff.get(), 0xCC, buff_size);
		mk::override_func(m_orig, m_prologue, m_buff.get(), mk::follow_jumps(mk::fn_ptr_to_ptr(neu)));
	}
	Redirector() :
		m_orig(),
		m_prologue(),
		m_buff()
	{
	}
	Redirector(Redirector&& other) :
		Redirector()
	{
		*this = std::move(other);
	}
	Redirector& operator=(Redirector&& other)
	{
		swap(other);
		return *this;
	}
	void swap(Redirector& other)
	{
		using std::swap;
		swap(this->m_orig, other.m_orig);
		swap(this->m_prologue, other.m_prologue);
		swap(this->m_buff, other.m_buff);
	}
	~Redirector()
	{
		if(m_orig)
		{
			mk::unoverride_func(m_orig, m_prologue, m_buff.get());
		}
	}
	void Close()
	{
		*this = Redirector{};
	}
	template<typename T, typename... A>
	auto CallOrig(T t, A... a)
	{
		assert(m_orig);
		return mk::call_backup_as(m_buff.get(), t, a...);
	}
private:
	char* m_orig;
	int m_prologue;
	std::unique_ptr<char[]> m_buff;
};


namespace EGDIAPIFuncs
{
	enum EGDIAPIFuncs
	{
		eNtGdiCreateBitmap,
		eNtGdiCreateCompatibleBitmap,
		eNtGdiCreateCompatibleDC,
		eNtGdiDeleteObjectApp,

		eCreateBitmap,
		eCreateBitmapIndirect,
		eCreateCompatibleBitmap,
		eCreateDIBitmap,
		eCreateDIBSection,
		eCreateDiscardableBitmap,

		eCreateBrushIndirect,
		eCreateDIBPatternBrush,
		eCreateDIBPatternBrushPt,
		eCreateHatchBrush,
		eCreatePatternBrush,
		eCreateSolidBrush,

		eCreateHalftonePalette,
		eCreatePalette,

		eCreateCompatibleDC,
		eCreateDCA,
		eCreateDCW,
		eCreateICA,
		eCreateICW,
		eDeleteDC,
		eDeleteObject,

		eCreateFontA,
		eCreateFontW,
		eCreateFontIndirectA,
		eCreateFontIndirectW,
		eCreateFontIndirectExA,
		eCreateFontIndirectExW,

		eDestroyCursor,
		eDestroyIcon,

		eExtractIconA,
		eExtractIconW,

		eCreateEnhMetaFileA,
		eCreateEnhMetaFileW,
		eCreateMetaFileA,
		eCreateMetaFileW,
		eDeleteEnhMetaFile,
		eDeleteMetaFile,
		eCloseEnhMetaFile,

		eCreatePen,
		eCreatePenIndirect,
		eExtCreatePen,

		eCreateEllipticRgn,
		eCreateEllipticRgnIndirect,
		eCreatePolygonRgn,
		eCreatePolyPolygonRgn,
		eCreateRectRgn,
		eCreateRectRgnIndirect,
		eCreateRoundRectRgn,
		eExtCreateRegion,
		ePathToRegion,

		MAX,
	};
}


typedef std::array<void*, 256> Stack;


static std::array<Redirector, EGDIAPIFuncs::MAX> s_redirectors;
static std::recursive_mutex s_gdi_objects_mutex;
static std::unordered_map<HGDIOBJ, Stack> s_gdi_objects;
static bool s_create_recursion = false;


void add_gdi_object(void* const& gdi_object);
void remove_gdi_object(void* const& gdi_object);
void OverrideInit();
void OverrideEnd();
void make_statistics();


void add_gdi_object(void* const& gdi_object)
{
	bool volatile b = false;
	if(b)
	{
		make_statistics();
	}

	auto it = s_gdi_objects.find(gdi_object);
	if(it != s_gdi_objects.end())
	{
		// Problem: We just created new object, but it is already in our map.
		// Reason: This is because we missed destruction of previous object with the same address / handle.
		// We missed it because it was not destroyed by DeleteObject function, but by kernel. Probably.
		// Solution: Just delete it from our map and add it again.
		s_gdi_objects.erase(it);
	}
	auto itb = s_gdi_objects.insert({gdi_object, {}});
	Stack& stck = itb.first->second;
	mk::stack_tracer::get_instance().get_trace(stck.data(), stck.size());
}

void remove_gdi_object(void* const& gdi_object)
{
	auto it = s_gdi_objects.find(gdi_object);
	if(it != s_gdi_objects.end())
	{
		s_gdi_objects.erase(it);
	}
	else
	{
		// assert
		int volatile i = 0;
		++i;
	}
}


HBITMAP APIENTRY NtGdiCreateBitmap_my(__in int cx, __in int cy, __in UINT cPlanes, __in UINT cBPP, __in_opt LPBYTE pjInit)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eNtGdiCreateBitmap].CallOrig(decltype(&NtGdiCreateBitmap){}, cx, cy, cPlanes, cBPP, pjInit);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HBITMAP APIENTRY NtGdiCreateCompatibleBitmap_my(__in HDC hdc, __in int cx, __in int cy)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eNtGdiCreateCompatibleBitmap].CallOrig(decltype(&NtGdiCreateCompatibleBitmap){}, hdc, cx, cy);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HDC APIENTRY NtGdiCreateCompatibleDC_my(__in_opt HDC hdc)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eNtGdiCreateCompatibleDC].CallOrig(decltype(&NtGdiCreateCompatibleDC){}, hdc);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

BOOL APIENTRY NtGdiDeleteObjectApp_my(__in HANDLE hobj)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	remove_gdi_object(hobj);
	auto ret = s_redirectors[EGDIAPIFuncs::eNtGdiDeleteObjectApp].CallOrig(decltype(&NtGdiDeleteObjectApp){}, hobj);
	return ret;
}


HBITMAP WINAPI CreateBitmap_my( _In_ int nWidth, _In_ int nHeight, _In_ UINT nPlanes, _In_ UINT nBitCount, _In_opt_ CONST VOID *lpBits)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateBitmap].CallOrig(&CreateBitmap, nWidth, nHeight, nPlanes, nBitCount, lpBits);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HBITMAP WINAPI CreateBitmapIndirect_my( _In_ CONST BITMAP *pbm)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateBitmapIndirect].CallOrig(&CreateBitmapIndirect, pbm);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HBITMAP WINAPI CreateCompatibleBitmap_my( _In_ HDC hdc, _In_ int cx, _In_ int cy)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateCompatibleBitmap].CallOrig(&CreateCompatibleBitmap, hdc, cx, cy);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HBITMAP WINAPI CreateDIBitmap_my( __in HDC hdc, __in_opt CONST BITMAPINFOHEADER *pbmih, __in DWORD flInit, __in_opt CONST VOID *pjBits, __in_opt CONST BITMAPINFO *pbmi, __in UINT iUsage)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateDIBitmap].CallOrig(&CreateDIBitmap, hdc, pbmih, flInit, pjBits, pbmi, iUsage);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HBITMAP WINAPI CreateDIBSection_my(__in_opt HDC hdc, __in CONST BITMAPINFO *lpbmi, __in UINT usage, __deref_opt_out VOID **ppvBits, __in_opt HANDLE hSection, __in DWORD offset)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateDIBSection].CallOrig(&CreateDIBSection, hdc, lpbmi, usage, ppvBits, hSection, offset);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HBITMAP WINAPI CreateDiscardableBitmap_my( __in HDC hdc, __in int cx, __in int cy)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateDiscardableBitmap].CallOrig(&CreateDiscardableBitmap, hdc, cx, cy);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}


HBRUSH  WINAPI CreateBrushIndirect_my( __in CONST LOGBRUSH *plbrush)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateBrushIndirect].CallOrig(&CreateBrushIndirect, plbrush);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HBRUSH  WINAPI CreateDIBPatternBrush_my( __in HGLOBAL h, __in UINT iUsage)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateDIBPatternBrush].CallOrig(&CreateDIBPatternBrush, h, iUsage);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HBRUSH  WINAPI CreateDIBPatternBrushPt_my( __in CONST VOID *lpPackedDIB, __in UINT iUsage)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateDIBPatternBrushPt].CallOrig(&CreateDIBPatternBrushPt, lpPackedDIB, iUsage);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HBRUSH  WINAPI CreateHatchBrush_my( __in int iHatch, __in COLORREF color)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateHatchBrush].CallOrig(&CreateHatchBrush, iHatch, color);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HBRUSH  WINAPI CreatePatternBrush_my( __in HBITMAP hbm)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreatePatternBrush].CallOrig(&CreatePatternBrush, hbm);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HBRUSH  WINAPI CreateSolidBrush_my( __in COLORREF color)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateSolidBrush].CallOrig(&CreateSolidBrush, color);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HPALETTE WINAPI CreateHalftonePalette_my( __in_opt HDC hdc)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateHalftonePalette].CallOrig(&CreateHalftonePalette, hdc);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HPALETTE WINAPI CreatePalette_my(CONST LOGPALETTE * plpal)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreatePalette].CallOrig(&CreatePalette, plpal);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}


HDC     WINAPI CreateCompatibleDC_my( __in_opt HDC hdc)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateCompatibleDC].CallOrig(&CreateCompatibleDC, hdc);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HDC     WINAPI CreateDCA_my( __in_opt LPCSTR pwszDriver, __in_opt LPCSTR pwszDevice, __in_opt LPCSTR pszPort, __in_opt CONST DEVMODEA * pdm)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateDCA].CallOrig(&CreateDCA, pwszDriver, pwszDevice, pszPort, pdm);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HDC     WINAPI CreateDCW_my( __in_opt LPCWSTR pwszDriver, __in_opt LPCWSTR pwszDevice, __in_opt LPCWSTR pszPort, __in_opt CONST DEVMODEW * pdm)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateDCW].CallOrig(&CreateDCW, pwszDriver, pwszDevice, pszPort, pdm);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HDC     WINAPI CreateICA_my( __in_opt LPCSTR pszDriver, __in_opt LPCSTR pszDevice, __in_opt LPCSTR pszPort, __in_opt CONST DEVMODEA * pdm)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateICA].CallOrig(&CreateICA, pszDriver, pszDevice, pszPort, pdm);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HDC     WINAPI CreateICW_my( __in_opt LPCWSTR pszDriver, __in_opt LPCWSTR pszDevice, __in_opt LPCWSTR pszPort, __in_opt CONST DEVMODEW * pdm)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateICW].CallOrig(&CreateICW, pszDriver, pszDevice, pszPort, pdm);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

BOOL WINAPI DeleteDC_my( __in HDC hdc)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	remove_gdi_object(hdc);
	auto ret = s_redirectors[EGDIAPIFuncs::eDeleteDC].CallOrig(&DeleteDC, hdc);
	return ret;
}

BOOL WINAPI DeleteObject_my( _In_ HGDIOBJ ho)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	auto ret = s_redirectors[EGDIAPIFuncs::eDeleteObject].CallOrig(&DeleteObject, ho);
	remove_gdi_object(ho);
	return ret;
}


HFONT   WINAPI CreateFontA_my( __in int cHeight, __in int cWidth, __in int cEscapement, __in int cOrientation, __in int cWeight, __in DWORD bItalic, __in DWORD bUnderline, __in DWORD bStrikeOut, __in DWORD iCharSet, __in DWORD iOutPrecision, __in DWORD iClipPrecision, __in DWORD iQuality, __in DWORD iPitchAndFamily, __in_opt LPCSTR pszFaceName)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateFontA].CallOrig(&CreateFontA, cHeight, cWidth, cEscapement, cOrientation, cWeight, bItalic, bUnderline, bStrikeOut, iCharSet, iOutPrecision, iClipPrecision, iQuality, iPitchAndFamily, pszFaceName);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HFONT   WINAPI CreateFontW_my( __in int cHeight, __in int cWidth, __in int cEscapement, __in int cOrientation, __in int cWeight, __in DWORD bItalic, __in DWORD bUnderline, __in DWORD bStrikeOut, __in DWORD iCharSet, __in DWORD iOutPrecision, __in DWORD iClipPrecision, __in DWORD iQuality, __in DWORD iPitchAndFamily, __in_opt LPCWSTR pszFaceName)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateFontW].CallOrig(&CreateFontW, cHeight, cWidth, cEscapement, cOrientation, cWeight, bItalic, bUnderline, bStrikeOut, iCharSet, iOutPrecision, iClipPrecision, iQuality, iPitchAndFamily, pszFaceName);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HFONT   WINAPI CreateFontIndirectA_my( __in CONST LOGFONTA *lplf)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateFontIndirectA].CallOrig(&CreateFontIndirectA, lplf);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HFONT   WINAPI CreateFontIndirectW_my( __in CONST LOGFONTW *lplf)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateFontIndirectW].CallOrig(&CreateFontIndirectW, lplf);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HFONT  WINAPI CreateFontIndirectExA_my( __in CONST ENUMLOGFONTEXDVA * xxx)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateFontIndirectExA].CallOrig(&CreateFontIndirectExA, xxx);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HFONT  WINAPI CreateFontIndirectExW_my( __in CONST ENUMLOGFONTEXDVW * xxx)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateFontIndirectExW].CallOrig(&CreateFontIndirectExW, xxx);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}


BOOL WINAPI DestroyCursor_my( _In_ HCURSOR hCursor)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	remove_gdi_object(hCursor);
	auto ret = s_redirectors[EGDIAPIFuncs::eDestroyCursor].CallOrig(&DestroyCursor, hCursor);
	return ret;
}

BOOL WINAPI DestroyIcon_my( _In_ HICON hIcon)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	remove_gdi_object(hIcon);
	auto ret = s_redirectors[EGDIAPIFuncs::eDestroyIcon].CallOrig(&DestroyIcon, hIcon);
	return ret;
}

HICON STDAPICALLTYPE ExtractIconA_my(__reserved HINSTANCE hInst, __in LPCSTR lpszExeFileName, __in UINT nIconIndex)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eExtractIconA].CallOrig(&ExtractIconA, hInst, lpszExeFileName, nIconIndex);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HICON STDAPICALLTYPE ExtractIconW_my(__reserved HINSTANCE hInst, __in LPCWSTR lpszExeFileName, __in UINT nIconIndex)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eExtractIconW].CallOrig(&ExtractIconW, hInst, lpszExeFileName, nIconIndex);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}


HDC   WINAPI CreateEnhMetaFileA_my( __in_opt HDC hdc, __in_opt LPCSTR lpFilename, __in_opt CONST RECT *lprc, __in_opt LPCSTR lpDesc)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateEnhMetaFileA].CallOrig(&CreateEnhMetaFileA, hdc, lpFilename, lprc, lpDesc);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HDC   WINAPI CreateEnhMetaFileW_my( __in_opt HDC hdc, __in_opt LPCWSTR lpFilename, __in_opt CONST RECT *lprc, __in_opt LPCWSTR lpDesc)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateEnhMetaFileW].CallOrig(&CreateEnhMetaFileW, hdc, lpFilename, lprc, lpDesc);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HDC     WINAPI CreateMetaFileA_my( __in_opt LPCSTR pszFile)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateMetaFileA].CallOrig(&CreateMetaFileA, pszFile);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HDC     WINAPI CreateMetaFileW_my( __in_opt LPCWSTR pszFile)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateMetaFileW].CallOrig(&CreateMetaFileW, pszFile);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

BOOL  WINAPI DeleteEnhMetaFile_my( __in_opt HENHMETAFILE hmf)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	remove_gdi_object(hmf);
	auto ret = s_redirectors[EGDIAPIFuncs::eDeleteEnhMetaFile].CallOrig(&DeleteEnhMetaFile, hmf);
	return ret;
}

BOOL WINAPI DeleteMetaFile_my( __in HMETAFILE hmf)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	remove_gdi_object(hmf);
	auto ret = s_redirectors[EGDIAPIFuncs::eDeleteMetaFile].CallOrig(&DeleteMetaFile, hmf);
	return ret;
}

HENHMETAFILE WINAPI CloseEnhMetaFile_my( __in HDC hdc)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	remove_gdi_object(hdc);
	auto ret = s_redirectors[EGDIAPIFuncs::eCloseEnhMetaFile].CallOrig(&CloseEnhMetaFile, hdc);
	return ret;
}


HPEN    WINAPI CreatePen_my( __in int iStyle, __in int cWidth, __in COLORREF color)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreatePen].CallOrig(&CreatePen, iStyle, cWidth, color);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HPEN    WINAPI CreatePenIndirect_my( __in CONST LOGPEN *plpen)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreatePenIndirect].CallOrig(&CreatePenIndirect, plpen);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HPEN WINAPI ExtCreatePen_my( __in DWORD iPenStyle, __in DWORD cWidth, __in CONST LOGBRUSH *plbrush, __in DWORD cStyle, __in_ecount_opt(cStyle) CONST DWORD *pstyle)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eExtCreatePen].CallOrig(&ExtCreatePen, iPenStyle, cWidth, plbrush, cStyle, pstyle);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}


HRGN    WINAPI CreateEllipticRgn_my( __in int x1, __in int y1, __in int x2, __in int y2)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateEllipticRgn].CallOrig(&CreateEllipticRgn, x1, y1, x2, y2);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HRGN    WINAPI CreateEllipticRgnIndirect_my( __in CONST RECT *lprect)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateEllipticRgnIndirect].CallOrig(&CreateEllipticRgnIndirect, lprect);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HRGN  WINAPI CreatePolygonRgn_my(    __in_ecount(cPoint) CONST POINT *pptl, __in int cPoint, __in int iMode)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreatePolygonRgn].CallOrig(&CreatePolygonRgn, pptl, cPoint, iMode);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HRGN    WINAPI CreatePolyPolygonRgn_my(  __in CONST POINT *pptl, __in CONST INT  *pc, __in int cPoly, __in int iMode)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreatePolyPolygonRgn].CallOrig(&CreatePolyPolygonRgn, pptl, pc, cPoly, iMode);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HRGN    WINAPI CreateRectRgn_my( __in int x1, __in int y1, __in int x2, __in int y2)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateRectRgn].CallOrig(&CreateRectRgn, x1, y1, x2, y2);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HRGN    WINAPI CreateRectRgnIndirect_my( __in CONST RECT *lprect)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateRectRgnIndirect].CallOrig(&CreateRectRgnIndirect, lprect);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HRGN    WINAPI CreateRoundRectRgn_my( __in int x1, __in int y1, __in int x2, __in int y2, __in int w, __in int h)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eCreateRoundRectRgn].CallOrig(&CreateRoundRectRgn, x2, y1, x2, y2, w, h);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HRGN WINAPI ExtCreateRegion_my( __in_opt CONST XFORM * lpx, __in DWORD nCount, __in_bcount(nCount) CONST RGNDATA * lpData)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::eExtCreateRegion].CallOrig(&ExtCreateRegion, lpx, nCount, lpData);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}

HRGN WINAPI PathToRegion_my(__in HDC hdc)
{
	std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
	bool const create_recursion = s_create_recursion;
	auto const fn_reverse_recursion = mk::make_scope_exit([&](){ s_create_recursion = create_recursion; });
	s_create_recursion = true;
	auto ret = s_redirectors[EGDIAPIFuncs::ePathToRegion].CallOrig(&PathToRegion, hdc);
	if(!create_recursion)
	{
		add_gdi_object(ret);
	}
	return ret;
}


void OverrideInit()
{
	//mk::symbol_locator locator;

	//s_redirectors[EGDIAPIFuncs::eNtGdiCreateBitmap] = Redirector(reinterpret_cast<decltype(&NtGdiCreateBitmap)>(locator.locate_symbol(L"gdi32!_NtGdiCreateBitmap@20")), 5, &NtGdiCreateBitmap_my);
	//s_redirectors[EGDIAPIFuncs::eNtGdiCreateCompatibleBitmap] = Redirector(reinterpret_cast<decltype(&NtGdiCreateCompatibleBitmap)>(locator.locate_symbol(L"gdi32!_NtGdiCreateCompatibleBitmap@12")), 5, &NtGdiCreateCompatibleBitmap_my);
	//s_redirectors[EGDIAPIFuncs::eNtGdiCreateCompatibleDC] = Redirector(reinterpret_cast<decltype(&NtGdiCreateCompatibleDC)>(locator.locate_symbol(L"gdi32!_NtGdiCreateCompatibleDC@4")), 5, &NtGdiCreateCompatibleDC_my);
	//s_redirectors[EGDIAPIFuncs::eNtGdiDeleteObjectApp] = Redirector(reinterpret_cast<decltype(&NtGdiDeleteObjectApp)>(locator.locate_symbol(L"gdi32!_NtGdiDeleteObjectApp@4")), 5, &NtGdiDeleteObjectApp_my);

	s_redirectors[EGDIAPIFuncs::eCreateBitmap] = Redirector(&CreateBitmap, 5, &CreateBitmap_my);
	s_redirectors[EGDIAPIFuncs::eCreateBitmapIndirect] = Redirector(&CreateBitmapIndirect, 5, &CreateBitmapIndirect_my);
	s_redirectors[EGDIAPIFuncs::eCreateCompatibleBitmap] = Redirector(&CreateCompatibleBitmap, 5, &CreateCompatibleBitmap_my);
	s_redirectors[EGDIAPIFuncs::eCreateDIBitmap] = Redirector(&CreateDIBitmap, 5, &CreateDIBitmap_my);
	s_redirectors[EGDIAPIFuncs::eCreateDIBSection] = Redirector(&CreateDIBSection, 5, &CreateDIBSection_my);
	s_redirectors[EGDIAPIFuncs::eCreateDiscardableBitmap] = Redirector(&CreateDiscardableBitmap, 5, CreateDiscardableBitmap_my);

	s_redirectors[EGDIAPIFuncs::eCreateBrushIndirect] = Redirector(&CreateBrushIndirect, 5, &CreateBrushIndirect_my);
	s_redirectors[EGDIAPIFuncs::eCreateDIBPatternBrush] = Redirector(&CreateDIBPatternBrush, 5, &CreateDIBPatternBrush_my);
	s_redirectors[EGDIAPIFuncs::eCreateDIBPatternBrushPt] = Redirector(&CreateDIBPatternBrushPt, 5, &CreateDIBPatternBrushPt_my);
	s_redirectors[EGDIAPIFuncs::eCreateHatchBrush] = Redirector(&CreateHatchBrush, 5, &CreateHatchBrush_my);
	s_redirectors[EGDIAPIFuncs::eCreatePatternBrush] = Redirector(&CreatePatternBrush, 5, &CreatePatternBrush_my);
	s_redirectors[EGDIAPIFuncs::eCreateSolidBrush] = Redirector(&CreateSolidBrush, 5, &CreateSolidBrush_my);

	s_redirectors[EGDIAPIFuncs::eCreateHalftonePalette] = Redirector(&CreateHalftonePalette, 5, &CreateHalftonePalette_my);
	s_redirectors[EGDIAPIFuncs::eCreatePalette] = Redirector(&CreatePalette, 5, &CreatePalette_my);

	s_redirectors[EGDIAPIFuncs::eCreateCompatibleDC] = Redirector(&CreateCompatibleDC, 5, &CreateCompatibleDC_my);
	s_redirectors[EGDIAPIFuncs::eCreateDCA] = Redirector(&CreateDCA, 5, &CreateDCA_my);
	s_redirectors[EGDIAPIFuncs::eCreateDCW] = Redirector(&CreateDCW, 5, &CreateDCW_my);
	s_redirectors[EGDIAPIFuncs::eCreateICA] = Redirector(&CreateICA, 5, &CreateICA_my);
	s_redirectors[EGDIAPIFuncs::eCreateICW] = Redirector(&CreateICW, 5, &CreateICW_my);
	s_redirectors[EGDIAPIFuncs::eDeleteDC] = Redirector(&DeleteDC, 5, &DeleteDC_my);
	s_redirectors[EGDIAPIFuncs::eDeleteObject] = Redirector(&DeleteObject, 5, &DeleteObject_my);

	s_redirectors[EGDIAPIFuncs::eCreateFontA] = Redirector(&CreateFontA, 5, &CreateFontA_my);
	s_redirectors[EGDIAPIFuncs::eCreateFontW] = Redirector(&CreateFontW, 5, &CreateFontW_my);
	s_redirectors[EGDIAPIFuncs::eCreateFontIndirectA] = Redirector(&CreateFontIndirectA, 5, &CreateFontIndirectA_my);
	s_redirectors[EGDIAPIFuncs::eCreateFontIndirectW] = Redirector(&CreateFontIndirectW, 5, &CreateFontIndirectW_my);
	s_redirectors[EGDIAPIFuncs::eCreateFontIndirectExA] = Redirector(&CreateFontIndirectExA, 5, &CreateFontIndirectExA_my);
	s_redirectors[EGDIAPIFuncs::eCreateFontIndirectExW] = Redirector(&CreateFontIndirectExW, 5, &CreateFontIndirectExW_my);

	//s_redirectors[EGDIAPIFuncs::eDestroyCursor] = Redirector(&DestroyCursor, 5, &DestroyCursor_my);
	//s_redirectors[EGDIAPIFuncs::eDestroyIcon] = Redirector(&DestroyIcon, 9, &DestroyIcon_my);

	//s_redirectors[EGDIAPIFuncs::eExtractIconA] = Redirector(reinterpret_cast<decltype(&ExtractIconA_my)>(GetProcAddress(GetModuleHandleW(L"shell32.dll"), "ExtractIconA")), 5, &ExtractIconA_my);
	//s_redirectors[EGDIAPIFuncs::eExtractIconW] = Redirector(reinterpret_cast<decltype(&ExtractIconW_my)>(GetProcAddress(GetModuleHandleW(L"shell32.dll"), "ExtractIconW")), 5, &ExtractIconW_my);

	s_redirectors[EGDIAPIFuncs::eCreateEnhMetaFileA] = Redirector(&CreateEnhMetaFileA, 5, &CreateEnhMetaFileA_my);
	s_redirectors[EGDIAPIFuncs::eCreateEnhMetaFileW] = Redirector(&CreateEnhMetaFileW, 5, &CreateEnhMetaFileW_my);
	s_redirectors[EGDIAPIFuncs::eCreateMetaFileA] = Redirector(&CreateMetaFileA, 5, &CreateMetaFileA_my);
	s_redirectors[EGDIAPIFuncs::eCreateMetaFileW] = Redirector(&CreateMetaFileW, 5, &CreateMetaFileW_my);
	s_redirectors[EGDIAPIFuncs::eDeleteEnhMetaFile] = Redirector(&DeleteEnhMetaFile, 5, &DeleteEnhMetaFile_my);
	s_redirectors[EGDIAPIFuncs::eDeleteMetaFile] = Redirector(&DeleteMetaFile, 5, &DeleteMetaFile_my);
	s_redirectors[EGDIAPIFuncs::eCloseEnhMetaFile] = Redirector(&CloseEnhMetaFile, 5, &CloseEnhMetaFile_my);

	s_redirectors[EGDIAPIFuncs::eCreatePen] = Redirector(&CreatePen, 5, &CreatePen_my);
	s_redirectors[EGDIAPIFuncs::eCreatePenIndirect] = Redirector(&CreatePenIndirect, 5, &CreatePenIndirect_my);
	s_redirectors[EGDIAPIFuncs::eExtCreatePen] = Redirector(&ExtCreatePen, 5, &ExtCreatePen_my);

	s_redirectors[EGDIAPIFuncs::eCreateEllipticRgn] = Redirector(&CreateEllipticRgn, 5, &CreateEllipticRgn_my);
	s_redirectors[EGDIAPIFuncs::eCreateEllipticRgnIndirect] = Redirector(&CreateEllipticRgnIndirect, 5, &CreateEllipticRgnIndirect_my);
	s_redirectors[EGDIAPIFuncs::eCreatePolygonRgn] = Redirector(&CreatePolygonRgn, 5, &CreatePolygonRgn_my);
	s_redirectors[EGDIAPIFuncs::eCreatePolyPolygonRgn] = Redirector(&CreatePolyPolygonRgn, 5, &CreatePolyPolygonRgn_my);
	s_redirectors[EGDIAPIFuncs::eCreateRectRgn] = Redirector(&CreateRectRgn, 5, &CreateRectRgn_my);
	s_redirectors[EGDIAPIFuncs::eCreateRectRgnIndirect] = Redirector(&CreateRectRgnIndirect, 5, &CreateRectRgnIndirect_my);
	s_redirectors[EGDIAPIFuncs::eCreateRoundRectRgn] = Redirector(&CreateRoundRectRgn, 5, &CreateRoundRectRgn_my);
	s_redirectors[EGDIAPIFuncs::eExtCreateRegion] = Redirector(&ExtCreateRegion, 5, &ExtCreateRegion_my);
	s_redirectors[EGDIAPIFuncs::ePathToRegion] = Redirector(&PathToRegion, 5, &PathToRegion_my);
}

void OverrideEnd()
{
	for(auto&& e : s_redirectors)
	{
		e.Close();
	}
	make_statistics();
}

void make_statistics()
{
	std::unordered_map<Stack, int, mk::std_hasher<mk::fnv1a>> stacks1;
	{
		std::lock_guard<std::recursive_mutex> lck(s_gdi_objects_mutex);
		for(auto&& e : s_gdi_objects)
		{
			++stacks1[e.second];
		}
	}
	std::vector<std::pair<Stack const, int> const*> stacks2;
	stacks2.reserve(stacks1.size());
	for(auto&& e : stacks1)
	{
		stacks2.push_back(&e);
	}
	std::sort(begin(stacks2), end(stacks2), [](auto const& a, auto const& b){ return b->second < a->second; });
	int volatile iii = 0;
	++iii;
}


static int s_i = []()
{
	MessageBoxA(0,0,0,0);
	OverrideInit();
	std::atexit([]()
	{
		OverrideEnd();
	});
	return 0;
}();


#endif
