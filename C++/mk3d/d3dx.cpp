#include "scope_exit.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <optional>
#include <string>
#include <vector>
#include <memory>

#define CINTERFACE
#define NOMINMAX
#define STRICT
#define WIN32_LEAN_AND_MEAN
#undef ISOLATION_AWARE_ENABLED
#undef UNICODE
#undef _UNICODE
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>


static constexpr char const s_yes[] = "yes";
static constexpr char const s_no[] = "no";
static constexpr char const* const s_yes_ptr = s_yes;
static constexpr char const* const s_no_ptr = s_no;
//static constexpr int const s_width = 800;
//static constexpr int const s_height = 800;
static constexpr int const s_flag_size = 10;


enum class driver_mode_e
{
	mode_default,
	mode_software,
	mode_hardware,
	mode_index,
};


struct big_state_t
{
	driver_mode_e m_driver_mode;
	int m_driver_index;
	int m_device_index;
	HWND m_main_window;

	LPDIRECTDRAW7 m_dd7;
	LPDIRECT3D7 m_d3d7;
	LPDIRECTDRAWSURFACE7 m_dd7_primary_surface;
	LPDIRECTDRAWCLIPPER m_dd7_clipper;
	LPDIRECTDRAWSURFACE7 m_dd7_back_surface;
	LPDIRECT3DDEVICE7 m_d3d7_device;
	LPDIRECT3DVERTEXBUFFER7 m_d3d7_vertex_buffer;
	WORD* m_d3d7_vertex_buffer_indices;
	D3DMATRIX m_d3d7_transform_world;
	D3DMATRIX m_d3d7_transform_view;
	D3DMATRIX m_d3d7_transform_projection;
	DWORD m_prev_tick_count;
	DWORD m_prev_resize_tick;
	RECT m_prev_rect;
	bool m_move_forward;
	bool m_move_backward;
	bool m_move_left;
	bool m_move_right;
	bool m_move_down;
	bool m_move_up;
	bool m_rotate_yaw_left;
	bool m_rotate_yaw_right;
	bool m_rotate_pitch_down;
	bool m_rotate_pitch_up;
	bool m_rotate_roll_left;
	bool m_rotate_roll_right;
};

struct big_state_deleter_t
{
	void operator()(big_state_t* const& big_state) const;
};
typedef std::unique_ptr<big_state_t, big_state_deleter_t> big_state_pointer_t;


void check_ret_failed(char const* const& file, int const& line, char const* const& expr);
bool check_ret_hresult_failed(HRESULT const& hr);
bool d3dx_1(int const argc, char const* const* const argv);
bool d3dx(int const argc, char const* const* const argv);
bool register_window_class(ATOM* const& out_main_window_class);
bool unregister_window_class(ATOM const& main_window_class);
bool create_main_window(ATOM const& main_window_class, HWND* const& out_main_window);
bool flip_surfaces(HWND const& hwnd, LPDIRECTDRAWSURFACE7 const& dd_primary_surface, LPDIRECTDRAWSURFACE7 const& dd_back_surface);
extern "C" LRESULT CALLBACK main_window_proc(_In_ HWND const hwnd, _In_ UINT const msg, _In_ WPARAM const w_param, _In_ LPARAM const l_param);
bool run_main_loop(int* const& out_exit_code);
bool on_idle();
float d3d_vector_magnitude_sqr(D3DVECTOR const& v);
float d3d_vector_magnitude(D3DVECTOR const& v);
D3DMATRIX const& d3d_matrix_identity();
D3DVECTOR operator-(D3DVECTOR const& a, D3DVECTOR const& b);
D3DVECTOR operator*(D3DVALUE const& a, D3DVECTOR const& b);
D3DVECTOR d3d_vector_normalize(D3DVECTOR const& v);
D3DVECTOR d3d_vector_cross_product(D3DVECTOR const& a, D3DVECTOR const& b);
float d3d_vector_dot_product(D3DVECTOR const& a, D3DVECTOR const& b);
float& d3d_matrix_index(D3DMATRIX& m, int const& row, int const& col);
float const& d3d_matrix_index(D3DMATRIX const& m, int const& row, int const& col);
D3DMATRIX d3d_matrix_translate(float const& dx, float const& dy, float const& dz);
D3DMATRIX d3d_matrix_rotate_z(float const& rads);
D3DMATRIX d3d_matrix_multiply(D3DMATRIX const& a, D3DMATRIX const& b);
D3DMATRIX d3d_matrix_view(D3DVECTOR const& from, D3DVECTOR const& at, D3DVECTOR const& world_up);
D3DMATRIX d3d_matrix_projection(float const& fov_hor, float const& fov_ver, float const& near_plane, float const& far_plane);
bool render(DWORD* const& prev_tick_count);
bool restore_surface(LPDIRECTDRAWSURFACE7 const& dd_surface);
bool restore_surfaces(LPDIRECTDRAWSURFACE7 const& dd_primary_surface, LPDIRECTDRAWSURFACE7 const& dd_back_surface);
std::string guid_to_string(GUID const& guid);
std::string guid_to_device_type_string(GUID const& guid);
bool big_state_init(driver_mode_e const& driver_mode, int const& driver_index, int const& device_index, HWND const& main_window, big_state_pointer_t* const& out_big_state_pointer);
void big_state_deinit(big_state_t* const& big_state);


void big_state_deleter_t::operator()(big_state_t* const& big_state) const
{
	big_state_deinit(big_state);
}


#define CHECK_RET(X, R) do{ if(X){}else{ [[unlikely]] check_ret_failed(__FILE__, __LINE__, #X); return R; } }while(false)
#define CHECK_RET_V(X) do{ if(X){}else{ [[unlikely]] check_ret_failed(__FILE__, __LINE__, #X); std::exit(EXIT_FAILURE); } }while(false)


static big_state_pointer_t g_big_state;


int main(int const argc, char const* const* const argv)
{
	auto something_wrong = mk::make_scope_exit([&](){ std::puts("Oh, no! Someting went wrong!"); });

	bool const ret = d3dx_1(argc, argv);
	CHECK_RET(ret, EXIT_FAILURE);

	something_wrong.reset();
	std::puts("We didn't crash! Great Success!");
	return EXIT_SUCCESS;
}


void check_ret_failed(char const* const& file, int const& line, char const* const& expr)
{
	std::printf("Failed in file `%s` at line %d with `%s`.\n", file, line, expr);
}

bool check_ret_hresult_failed(HRESULT const& hr)
{
	switch(hr)
	{
		case DDERR_ALREADYINITIALIZED: std::puts("DDERR_ALREADYINITIALIZED"); break;
		case DDERR_CANNOTATTACHSURFACE: std::puts("DDERR_CANNOTATTACHSURFACE"); break;
		case DDERR_CANNOTDETACHSURFACE: std::puts("DDERR_CANNOTDETACHSURFACE"); break;
		case DDERR_CURRENTLYNOTAVAIL: std::puts("DDERR_CURRENTLYNOTAVAIL"); break;
		case DDERR_EXCEPTION: std::puts("DDERR_EXCEPTION"); break;
		case DDERR_GENERIC: std::puts("DDERR_GENERIC"); break;
		case DDERR_HEIGHTALIGN: std::puts("DDERR_HEIGHTALIGN"); break;
		case DDERR_INCOMPATIBLEPRIMARY: std::puts("DDERR_INCOMPATIBLEPRIMARY"); break;
		case DDERR_INVALIDCAPS: std::puts("DDERR_INVALIDCAPS"); break;
		case DDERR_INVALIDCLIPLIST: std::puts("DDERR_INVALIDCLIPLIST"); break;
		case DDERR_INVALIDMODE: std::puts("DDERR_INVALIDMODE"); break;
		case DDERR_INVALIDOBJECT: std::puts("DDERR_INVALIDOBJECT"); break;
		case DDERR_INVALIDPARAMS: std::puts("DDERR_INVALIDPARAMS"); break;
		case DDERR_INVALIDPIXELFORMAT: std::puts("DDERR_INVALIDPIXELFORMAT"); break;
		case DDERR_INVALIDRECT: std::puts("DDERR_INVALIDRECT"); break;
		case DDERR_LOCKEDSURFACES: std::puts("DDERR_LOCKEDSURFACES"); break;
		case DDERR_NO3D: std::puts("DDERR_NO3D"); break;
		case DDERR_NOALPHAHW: std::puts("DDERR_NOALPHAHW"); break;
		case DDERR_NOSTEREOHARDWARE: std::puts("DDERR_NOSTEREOHARDWARE"); break;
		case DDERR_NOSURFACELEFT: std::puts("DDERR_NOSURFACELEFT"); break;
		case DDERR_NOCLIPLIST: std::puts("DDERR_NOCLIPLIST"); break;
		case DDERR_NOCOLORCONVHW: std::puts("DDERR_NOCOLORCONVHW"); break;
		case DDERR_NOCOOPERATIVELEVELSET: std::puts("DDERR_NOCOOPERATIVELEVELSET"); break;
		case DDERR_NOCOLORKEY: std::puts("DDERR_NOCOLORKEY"); break;
		case DDERR_NOCOLORKEYHW: std::puts("DDERR_NOCOLORKEYHW"); break;
		case DDERR_NODIRECTDRAWSUPPORT: std::puts("DDERR_NODIRECTDRAWSUPPORT"); break;
		case DDERR_NOEXCLUSIVEMODE: std::puts("DDERR_NOEXCLUSIVEMODE"); break;
		case DDERR_NOFLIPHW: std::puts("DDERR_NOFLIPHW"); break;
		case DDERR_NOGDI: std::puts("DDERR_NOGDI"); break;
		case DDERR_NOMIRRORHW: std::puts("DDERR_NOMIRRORHW"); break;
		case DDERR_NOTFOUND: std::puts("DDERR_NOTFOUND"); break;
		case DDERR_NOOVERLAYHW: std::puts("DDERR_NOOVERLAYHW"); break;
		case DDERR_OVERLAPPINGRECTS: std::puts("DDERR_OVERLAPPINGRECTS"); break;
		case DDERR_NORASTEROPHW: std::puts("DDERR_NORASTEROPHW"); break;
		case DDERR_NOROTATIONHW: std::puts("DDERR_NOROTATIONHW"); break;
		case DDERR_NOSTRETCHHW: std::puts("DDERR_NOSTRETCHHW"); break;
		case DDERR_NOT4BITCOLOR: std::puts("DDERR_NOT4BITCOLOR"); break;
		case DDERR_NOT4BITCOLORINDEX: std::puts("DDERR_NOT4BITCOLORINDEX"); break;
		case DDERR_NOT8BITCOLOR: std::puts("DDERR_NOT8BITCOLOR"); break;
		case DDERR_NOTEXTUREHW: std::puts("DDERR_NOTEXTUREHW"); break;
		case DDERR_NOVSYNCHW: std::puts("DDERR_NOVSYNCHW"); break;
		case DDERR_NOZBUFFERHW: std::puts("DDERR_NOZBUFFERHW"); break;
		case DDERR_NOZOVERLAYHW: std::puts("DDERR_NOZOVERLAYHW"); break;
		case DDERR_OUTOFCAPS: std::puts("DDERR_OUTOFCAPS"); break;
		case DDERR_OUTOFMEMORY: std::puts("DDERR_OUTOFMEMORY"); break;
		case DDERR_OUTOFVIDEOMEMORY: std::puts("DDERR_OUTOFVIDEOMEMORY"); break;
		case DDERR_OVERLAYCANTCLIP: std::puts("DDERR_OVERLAYCANTCLIP"); break;
		case DDERR_OVERLAYCOLORKEYONLYONEACTIVE: std::puts("DDERR_OVERLAYCOLORKEYONLYONEACTIVE"); break;
		case DDERR_PALETTEBUSY: std::puts("DDERR_PALETTEBUSY"); break;
		case DDERR_COLORKEYNOTSET: std::puts("DDERR_COLORKEYNOTSET"); break;
		case DDERR_SURFACEALREADYATTACHED: std::puts("DDERR_SURFACEALREADYATTACHED"); break;
		case DDERR_SURFACEALREADYDEPENDENT: std::puts("DDERR_SURFACEALREADYDEPENDENT"); break;
		case DDERR_SURFACEBUSY: std::puts("DDERR_SURFACEBUSY"); break;
		case DDERR_CANTLOCKSURFACE: std::puts("DDERR_CANTLOCKSURFACE"); break;
		case DDERR_SURFACEISOBSCURED: std::puts("DDERR_SURFACEISOBSCURED"); break;
		case DDERR_SURFACELOST: std::puts("DDERR_SURFACELOST"); break;
		case DDERR_SURFACENOTATTACHED: std::puts("DDERR_SURFACENOTATTACHED"); break;
		case DDERR_TOOBIGHEIGHT: std::puts("DDERR_TOOBIGHEIGHT"); break;
		case DDERR_TOOBIGSIZE: std::puts("DDERR_TOOBIGSIZE"); break;
		case DDERR_TOOBIGWIDTH: std::puts("DDERR_TOOBIGWIDTH"); break;
		case DDERR_UNSUPPORTED: std::puts("DDERR_UNSUPPORTED"); break;
		case DDERR_UNSUPPORTEDFORMAT: std::puts("DDERR_UNSUPPORTEDFORMAT"); break;
		case DDERR_UNSUPPORTEDMASK: std::puts("DDERR_UNSUPPORTEDMASK"); break;
		case DDERR_INVALIDSTREAM: std::puts("DDERR_INVALIDSTREAM"); break;
		case DDERR_VERTICALBLANKINPROGRESS: std::puts("DDERR_VERTICALBLANKINPROGRESS"); break;
		case DDERR_WASSTILLDRAWING: std::puts("DDERR_WASSTILLDRAWING"); break;
		case DDERR_DDSCAPSCOMPLEXREQUIRED: std::puts("DDERR_DDSCAPSCOMPLEXREQUIRED"); break;
		case DDERR_XALIGN: std::puts("DDERR_XALIGN"); break;
		case DDERR_INVALIDDIRECTDRAWGUID: std::puts("DDERR_INVALIDDIRECTDRAWGUID"); break;
		case DDERR_DIRECTDRAWALREADYCREATED: std::puts("DDERR_DIRECTDRAWALREADYCREATED"); break;
		case DDERR_NODIRECTDRAWHW: std::puts("DDERR_NODIRECTDRAWHW"); break;
		case DDERR_PRIMARYSURFACEALREADYEXISTS: std::puts("DDERR_PRIMARYSURFACEALREADYEXISTS"); break;
		case DDERR_NOEMULATION: std::puts("DDERR_NOEMULATION"); break;
		case DDERR_REGIONTOOSMALL: std::puts("DDERR_REGIONTOOSMALL"); break;
		case DDERR_CLIPPERISUSINGHWND: std::puts("DDERR_CLIPPERISUSINGHWND"); break;
		case DDERR_NOCLIPPERATTACHED: std::puts("DDERR_NOCLIPPERATTACHED"); break;
		case DDERR_NOHWND: std::puts("DDERR_NOHWND"); break;
		case DDERR_HWNDSUBCLASSED: std::puts("DDERR_HWNDSUBCLASSED"); break;
		case DDERR_HWNDALREADYSET: std::puts("DDERR_HWNDALREADYSET"); break;
		case DDERR_NOPALETTEATTACHED: std::puts("DDERR_NOPALETTEATTACHED"); break;
		case DDERR_NOPALETTEHW: std::puts("DDERR_NOPALETTEHW"); break;
		case DDERR_BLTFASTCANTCLIP: std::puts("DDERR_BLTFASTCANTCLIP"); break;
		case DDERR_NOBLTHW: std::puts("DDERR_NOBLTHW"); break;
		case DDERR_NODDROPSHW: std::puts("DDERR_NODDROPSHW"); break;
		case DDERR_OVERLAYNOTVISIBLE: std::puts("DDERR_OVERLAYNOTVISIBLE"); break;
		case DDERR_NOOVERLAYDEST: std::puts("DDERR_NOOVERLAYDEST"); break;
		case DDERR_INVALIDPOSITION: std::puts("DDERR_INVALIDPOSITION"); break;
		case DDERR_NOTAOVERLAYSURFACE: std::puts("DDERR_NOTAOVERLAYSURFACE"); break;
		case DDERR_EXCLUSIVEMODEALREADYSET: std::puts("DDERR_EXCLUSIVEMODEALREADYSET"); break;
		case DDERR_NOTFLIPPABLE: std::puts("DDERR_NOTFLIPPABLE"); break;
		case DDERR_CANTDUPLICATE: std::puts("DDERR_CANTDUPLICATE"); break;
		case DDERR_NOTLOCKED: std::puts("DDERR_NOTLOCKED"); break;
		case DDERR_CANTCREATEDC: std::puts("DDERR_CANTCREATEDC"); break;
		case DDERR_NODC: std::puts("DDERR_NODC"); break;
		case DDERR_WRONGMODE: std::puts("DDERR_WRONGMODE"); break;
		case DDERR_IMPLICITLYCREATED: std::puts("DDERR_IMPLICITLYCREATED"); break;
		case DDERR_NOTPALETTIZED: std::puts("DDERR_NOTPALETTIZED"); break;
		case DDERR_UNSUPPORTEDMODE: std::puts("DDERR_UNSUPPORTEDMODE"); break;
		case DDERR_NOMIPMAPHW: std::puts("DDERR_NOMIPMAPHW"); break;
		case DDERR_INVALIDSURFACETYPE: std::puts("DDERR_INVALIDSURFACETYPE"); break;
		case DDERR_NOOPTIMIZEHW: std::puts("DDERR_NOOPTIMIZEHW"); break;
		case DDERR_NOTLOADED: std::puts("DDERR_NOTLOADED"); break;
		case DDERR_NOFOCUSWINDOW: std::puts("DDERR_NOFOCUSWINDOW"); break;
		case DDERR_NOTONMIPMAPSUBLEVEL: std::puts("DDERR_NOTONMIPMAPSUBLEVEL"); break;
		case DDERR_DCALREADYCREATED: std::puts("DDERR_DCALREADYCREATED"); break;
		case DDERR_NONONLOCALVIDMEM: std::puts("DDERR_NONONLOCALVIDMEM"); break;
		case DDERR_CANTPAGELOCK: std::puts("DDERR_CANTPAGELOCK"); break;
		case DDERR_CANTPAGEUNLOCK: std::puts("DDERR_CANTPAGEUNLOCK"); break;
		case DDERR_NOTPAGELOCKED: std::puts("DDERR_NOTPAGELOCKED"); break;
		case DDERR_MOREDATA: std::puts("DDERR_MOREDATA"); break;
		case DDERR_EXPIRED: std::puts("DDERR_EXPIRED"); break;
		case DDERR_TESTFINISHED: std::puts("DDERR_TESTFINISHED"); break;
		case DDERR_NEWMODE: std::puts("DDERR_NEWMODE"); break;
		case DDERR_D3DNOTINITIALIZED: std::puts("DDERR_D3DNOTINITIALIZED"); break;
		case DDERR_VIDEONOTACTIVE: std::puts("DDERR_VIDEONOTACTIVE"); break;
		case DDERR_NOMONITORINFORMATION: std::puts("DDERR_NOMONITORINFORMATION"); break;
		case DDERR_NODRIVERSUPPORT: std::puts("DDERR_NODRIVERSUPPORT"); break;
		case DDERR_DEVICEDOESNTOWNSURFACE: std::puts("DDERR_DEVICEDOESNTOWNSURFACE"); break;
		case DDERR_NOTINITIALIZED: std::puts("DDERR_NOTINITIALIZED"); break;

		case D3DERR_BADMAJORVERSION: std::puts("D3DERR_BADMAJORVERSION"); break;
		case D3DERR_BADMINORVERSION: std::puts("D3DERR_BADMINORVERSION"); break;
		case D3DERR_INVALID_DEVICE: std::puts("D3DERR_INVALID_DEVICE"); break;
		case D3DERR_INITFAILED: std::puts("D3DERR_INITFAILED"); break;
		case D3DERR_DEVICEAGGREGATED: std::puts("D3DERR_DEVICEAGGREGATED"); break;
		case D3DERR_EXECUTE_CREATE_FAILED: std::puts("D3DERR_EXECUTE_CREATE_FAILED"); break;
		case D3DERR_EXECUTE_DESTROY_FAILED: std::puts("D3DERR_EXECUTE_DESTROY_FAILED"); break;
		case D3DERR_EXECUTE_LOCK_FAILED: std::puts("D3DERR_EXECUTE_LOCK_FAILED"); break;
		case D3DERR_EXECUTE_UNLOCK_FAILED: std::puts("D3DERR_EXECUTE_UNLOCK_FAILED"); break;
		case D3DERR_EXECUTE_LOCKED: std::puts("D3DERR_EXECUTE_LOCKED"); break;
		case D3DERR_EXECUTE_NOT_LOCKED: std::puts("D3DERR_EXECUTE_NOT_LOCKED"); break;
		case D3DERR_EXECUTE_FAILED: std::puts("D3DERR_EXECUTE_FAILED"); break;
		case D3DERR_EXECUTE_CLIPPED_FAILED: std::puts("D3DERR_EXECUTE_CLIPPED_FAILED"); break;
		case D3DERR_TEXTURE_NO_SUPPORT: std::puts("D3DERR_TEXTURE_NO_SUPPORT"); break;
		case D3DERR_TEXTURE_CREATE_FAILED: std::puts("D3DERR_TEXTURE_CREATE_FAILED"); break;
		case D3DERR_TEXTURE_DESTROY_FAILED: std::puts("D3DERR_TEXTURE_DESTROY_FAILED"); break;
		case D3DERR_TEXTURE_LOCK_FAILED: std::puts("D3DERR_TEXTURE_LOCK_FAILED"); break;
		case D3DERR_TEXTURE_UNLOCK_FAILED: std::puts("D3DERR_TEXTURE_UNLOCK_FAILED"); break;
		case D3DERR_TEXTURE_LOAD_FAILED: std::puts("D3DERR_TEXTURE_LOAD_FAILED"); break;
		case D3DERR_TEXTURE_SWAP_FAILED: std::puts("D3DERR_TEXTURE_SWAP_FAILED"); break;
		case D3DERR_TEXTURE_LOCKED: std::puts("D3DERR_TEXTURE_LOCKED"); break;
		case D3DERR_TEXTURE_NOT_LOCKED: std::puts("D3DERR_TEXTURE_NOT_LOCKED"); break;
		case D3DERR_TEXTURE_GETSURF_FAILED: std::puts("D3DERR_TEXTURE_GETSURF_FAILED"); break;
		case D3DERR_MATRIX_CREATE_FAILED: std::puts("D3DERR_MATRIX_CREATE_FAILED"); break;
		case D3DERR_MATRIX_DESTROY_FAILED: std::puts("D3DERR_MATRIX_DESTROY_FAILED"); break;
		case D3DERR_MATRIX_SETDATA_FAILED: std::puts("D3DERR_MATRIX_SETDATA_FAILED"); break;
		case D3DERR_MATRIX_GETDATA_FAILED: std::puts("D3DERR_MATRIX_GETDATA_FAILED"); break;
		case D3DERR_SETVIEWPORTDATA_FAILED: std::puts("D3DERR_SETVIEWPORTDATA_FAILED"); break;
		case D3DERR_INVALIDCURRENTVIEWPORT: std::puts("D3DERR_INVALIDCURRENTVIEWPORT"); break;
		case D3DERR_INVALIDPRIMITIVETYPE: std::puts("D3DERR_INVALIDPRIMITIVETYPE"); break;
		case D3DERR_INVALIDVERTEXTYPE: std::puts("D3DERR_INVALIDVERTEXTYPE"); break;
		case D3DERR_TEXTURE_BADSIZE: std::puts("D3DERR_TEXTURE_BADSIZE"); break;
		case D3DERR_INVALIDRAMPTEXTURE: std::puts("D3DERR_INVALIDRAMPTEXTURE"); break;
		case D3DERR_MATERIAL_CREATE_FAILED: std::puts("D3DERR_MATERIAL_CREATE_FAILED"); break;
		case D3DERR_MATERIAL_DESTROY_FAILED: std::puts("D3DERR_MATERIAL_DESTROY_FAILED"); break;
		case D3DERR_MATERIAL_SETDATA_FAILED: std::puts("D3DERR_MATERIAL_SETDATA_FAILED"); break;
		case D3DERR_MATERIAL_GETDATA_FAILED: std::puts("D3DERR_MATERIAL_GETDATA_FAILED"); break;
		case D3DERR_INVALIDPALETTE: std::puts("D3DERR_INVALIDPALETTE"); break;
		case D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY: std::puts("D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY"); break;
		case D3DERR_ZBUFF_NEEDS_VIDEOMEMORY: std::puts("D3DERR_ZBUFF_NEEDS_VIDEOMEMORY"); break;
		case D3DERR_SURFACENOTINVIDMEM: std::puts("D3DERR_SURFACENOTINVIDMEM"); break;
		case D3DERR_LIGHT_SET_FAILED: std::puts("D3DERR_LIGHT_SET_FAILED"); break;
		case D3DERR_LIGHTHASVIEWPORT: std::puts("D3DERR_LIGHTHASVIEWPORT"); break;
		case D3DERR_LIGHTNOTINTHISVIEWPORT: std::puts("D3DERR_LIGHTNOTINTHISVIEWPORT"); break;
		case D3DERR_SCENE_IN_SCENE: std::puts("D3DERR_SCENE_IN_SCENE"); break;
		case D3DERR_SCENE_NOT_IN_SCENE: std::puts("D3DERR_SCENE_NOT_IN_SCENE"); break;
		case D3DERR_SCENE_BEGIN_FAILED: std::puts("D3DERR_SCENE_BEGIN_FAILED"); break;
		case D3DERR_SCENE_END_FAILED: std::puts("D3DERR_SCENE_END_FAILED"); break;
		case D3DERR_INBEGIN: std::puts("D3DERR_INBEGIN"); break;
		case D3DERR_NOTINBEGIN: std::puts("D3DERR_NOTINBEGIN"); break;
		case D3DERR_NOVIEWPORTS: std::puts("D3DERR_NOVIEWPORTS"); break;
		case D3DERR_VIEWPORTDATANOTSET: std::puts("D3DERR_VIEWPORTDATANOTSET"); break;
		case D3DERR_VIEWPORTHASNODEVICE: std::puts("D3DERR_VIEWPORTHASNODEVICE"); break;
		case D3DERR_NOCURRENTVIEWPORT: std::puts("D3DERR_NOCURRENTVIEWPORT"); break;
		case D3DERR_INVALIDVERTEXFORMAT: std::puts("D3DERR_INVALIDVERTEXFORMAT"); break;
		case D3DERR_COLORKEYATTACHED: std::puts("D3DERR_COLORKEYATTACHED"); break;
		case D3DERR_VERTEXBUFFEROPTIMIZED: std::puts("D3DERR_VERTEXBUFFEROPTIMIZED"); break;
		case D3DERR_VBUF_CREATE_FAILED: std::puts("D3DERR_VBUF_CREATE_FAILED"); break;
		case D3DERR_VERTEXBUFFERLOCKED: std::puts("D3DERR_VERTEXBUFFERLOCKED"); break;
		case D3DERR_VERTEXBUFFERUNLOCKFAILED: std::puts("D3DERR_VERTEXBUFFERUNLOCKFAILED"); break;
		case D3DERR_ZBUFFER_NOTPRESENT: std::puts("D3DERR_ZBUFFER_NOTPRESENT"); break;
		case D3DERR_STENCILBUFFER_NOTPRESENT: std::puts("D3DERR_STENCILBUFFER_NOTPRESENT"); break;
		case D3DERR_WRONGTEXTUREFORMAT: std::puts("D3DERR_WRONGTEXTUREFORMAT"); break;
		case D3DERR_UNSUPPORTEDCOLOROPERATION: std::puts("D3DERR_UNSUPPORTEDCOLOROPERATION"); break;
		case D3DERR_UNSUPPORTEDCOLORARG: std::puts("D3DERR_UNSUPPORTEDCOLORARG"); break;
		case D3DERR_UNSUPPORTEDALPHAOPERATION: std::puts("D3DERR_UNSUPPORTEDALPHAOPERATION"); break;
		case D3DERR_UNSUPPORTEDALPHAARG: std::puts("D3DERR_UNSUPPORTEDALPHAARG"); break;
		case D3DERR_TOOMANYOPERATIONS: std::puts("D3DERR_TOOMANYOPERATIONS"); break;
		case D3DERR_CONFLICTINGTEXTUREFILTER: std::puts("D3DERR_CONFLICTINGTEXTUREFILTER"); break;
		case D3DERR_UNSUPPORTEDFACTORVALUE: std::puts("D3DERR_UNSUPPORTEDFACTORVALUE"); break;
		case D3DERR_CONFLICTINGRENDERSTATE: std::puts("D3DERR_CONFLICTINGRENDERSTATE"); break;
		case D3DERR_UNSUPPORTEDTEXTUREFILTER: std::puts("D3DERR_UNSUPPORTEDTEXTUREFILTER"); break;
		case D3DERR_TOOMANYPRIMITIVES: std::puts("D3DERR_TOOMANYPRIMITIVES"); break;
		case D3DERR_INVALIDMATRIX: std::puts("D3DERR_INVALIDMATRIX"); break;
		case D3DERR_TOOMANYVERTICES: std::puts("D3DERR_TOOMANYVERTICES"); break;
		case D3DERR_CONFLICTINGTEXTUREPALETTE: std::puts("D3DERR_CONFLICTINGTEXTUREPALETTE"); break;
		case D3DERR_INVALIDSTATEBLOCK: std::puts("D3DERR_INVALIDSTATEBLOCK"); break;
		case D3DERR_INBEGINSTATEBLOCK: std::puts("D3DERR_INBEGINSTATEBLOCK"); break;
		case D3DERR_NOTINBEGINSTATEBLOCK: std::puts("D3DERR_NOTINBEGINSTATEBLOCK"); break;

		default:
		{
			std::printf("HRESULT error %d (%x)\n", hr, hr);
			break;
		}
	}
	return false;
}

bool d3dx_1(int const argc, char const* const* const argv)
{
	__try
	{
		return d3dx(argc, argv);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		std::puts("SEH exception!");
		return false;
	}
}

bool d3dx(int const argc, char const* const* const argv)
{
	std::memset(&g_big_state, 0, sizeof(g_big_state));

	static constexpr char const s_mode_default[] = "/default";
	static constexpr char const s_mode_software[] = "/sw";
	static constexpr char const s_mode_hardware[] = "/hw";
	int driver_index;
	driver_mode_e const driver_mode = [&](int* const& out_driver_index) -> driver_mode_e
	{
		if(!(argc >= 2))
		{
			return driver_mode_e::mode_default;
		}
		if(std::strcmp(argv[1], s_mode_default) == 0)
		{
			return driver_mode_e::mode_default;
		}
		if(std::strcmp(argv[1], s_mode_software) == 0)
		{
			return driver_mode_e::mode_software;
		}
		if(std::strcmp(argv[1], s_mode_hardware) == 0)
		{
			return driver_mode_e::mode_hardware;
		}
		#pragma warning(push)
		#pragma warning(disable:4996) // error C4996: 'sscanf': This function or variable may be unsafe. Consider using sscanf_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
		int const scanned = std::sscanf(argv[1], "%d", out_driver_index);
		CHECK_RET_V(scanned == 1);
		#pragma warning(pop)
		return driver_mode_e::mode_index;
	}(&driver_index);

	int const device_index = [&]() -> int
	{
		if(!(argc >= 3))
		{
			return 0;
		}
		#pragma warning(push)
		#pragma warning(disable:4996) // error C4996: 'sscanf': This function or variable may be unsafe. Consider using sscanf_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
		int device_index;
		int const scanned = std::sscanf(argv[2], "%d", &device_index);
		CHECK_RET_V(scanned == 1);
		#pragma warning(pop)
		return device_index;
	}();

	ATOM main_window_class;
	bool const registered = register_window_class(&main_window_class);
	CHECK_RET(registered, false);
	auto const fn_unregister_window_class = mk::make_scope_exit([&](){ bool const unregistered = unregister_window_class(main_window_class); CHECK_RET_V(unregistered); });

	HWND main_window;
	bool const main_window_created = create_main_window(main_window_class, &main_window);
	CHECK_RET(main_window_created , false);

	bool const big_inited = big_state_init(driver_mode, driver_index, device_index, main_window, &g_big_state);
	CHECK_RET(big_inited, false);
	big_state_t& big_state = *g_big_state;

	big_state.m_prev_tick_count = GetTickCount();
	big_state.m_prev_resize_tick = big_state.m_prev_tick_count;

	RECT r;
	BOOL const got_rect = GetClientRect(main_window, &r);
	CHECK_RET(got_rect != 0, false);
	POINT p{};
	BOOL const converted = ClientToScreen(main_window, &p);
	CHECK_RET(converted != 0, false);
	BOOL const offset = OffsetRect(&r, p.x, p.y);
	CHECK_RET(offset != 0, false);
	big_state.m_prev_rect = r;

	BOOL const was_visible = ShowWindow(main_window, SW_SHOW);
	BOOL const updated = UpdateWindow(main_window);
	CHECK_RET(updated != 0, false);

	int exit_code;
	bool const ran = run_main_loop(&exit_code);
	CHECK_RET(ran, false);

	return true;
}

bool register_window_class(ATOM* const& out_main_window_class)
{
	HMODULE const self = GetModuleHandleW(nullptr);
	CHECK_RET(self != nullptr, false);

	#pragma warning(push)
	#pragma warning(disable:4302) // warning C4302: 'type cast': truncation from 'LPSTR' to 'WORD'

	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = 0;
	wc.lpfnWndProc = &main_window_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = self;
	wc.hIcon = LoadIconW(nullptr, MAKEINTRESOURCEW(IDI_APPLICATION));
	wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(IDC_ARROW));
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = L"main_window";
	wc.hIconSm = LoadIconW(nullptr, MAKEINTRESOURCEW(IDI_APPLICATION));

	#pragma warning(pop)

	ATOM const main_window_class = RegisterClassExW(&wc);
	CHECK_RET(main_window_class != 0, false);

	*out_main_window_class = main_window_class;
	return true;
}

bool unregister_window_class(ATOM const& main_window_class)
{
	HMODULE const self = GetModuleHandleW(nullptr);
	CHECK_RET(self != nullptr, false);

	BOOL const unregistered = UnregisterClassW(reinterpret_cast<wchar_t const*>(main_window_class), self);
	CHECK_RET(unregistered != 0, false);

	return true;
}

bool create_main_window(ATOM const& main_window_class, HWND* const& out_main_window)
{
	DWORD const style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
	DWORD const style_ex = 0;

	/*RECT r;
	r.left = 0;
	r.top = 0;
	r.right = s_width;
	r.bottom = s_height;
	BOOL const adjusted = AdjustWindowRectEx(&r, style, FALSE, style_ex);
	CHECK_RET(adjusted != 0, false);*/

	HMODULE const self = GetModuleHandleW(nullptr);
	CHECK_RET(self != nullptr, false);

	HWND const main_window = CreateWindowExW
	(
		style_ex,
		reinterpret_cast<wchar_t const*>(main_window_class),
		L"Main Window",
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		/*r.right - r.left*/CW_USEDEFAULT,
		/*r.bottom - r.top*/CW_USEDEFAULT,
		nullptr,
		nullptr,
		self,
		nullptr
	);
	CHECK_RET(main_window != nullptr, false);

	*out_main_window = main_window;
	return true;
}

bool flip_surfaces(HWND const& hwnd, LPDIRECTDRAWSURFACE7 const& dd_primary_surface, LPDIRECTDRAWSURFACE7 const& dd_back_surface)
{
	RECT r;
	BOOL const got_rect = GetClientRect(hwnd, &r);
	CHECK_RET(got_rect != 0, false);
	POINT p{};
	BOOL const converted = ClientToScreen(hwnd, &p);
	CHECK_RET(converted != 0, false);
	BOOL const offset = OffsetRect(&r, p.x, p.y);
	CHECK_RET(offset != 0, false);

	big_state_t& big_state = *g_big_state;
	if(std::memcmp(&r, &big_state.m_prev_rect, sizeof(r)) != 0 && GetTickCount() - big_state.m_prev_resize_tick > 1000)
	{
		driver_mode_e const driver_mode = big_state.m_driver_mode;
		int const driver_index = big_state.m_driver_index;
		int const device_index = big_state.m_device_index;
		HWND const main_window = big_state.m_main_window;
		D3DMATRIX const view_matrix = big_state.m_d3d7_transform_view;

		g_big_state.reset();
		bool const big_inited = big_state_init(driver_mode, driver_index, device_index, main_window, &g_big_state);
		CHECK_RET(big_inited, false);

		big_state_t& big_state_new = *g_big_state;
		big_state_new.m_prev_tick_count = GetTickCount();
		big_state_new.m_prev_resize_tick = GetTickCount();
		big_state_new.m_prev_rect = r;
		big_state_new.m_d3d7_transform_view = view_matrix;
		return true;
	}

	bool const restored = restore_surfaces(dd_primary_surface, dd_back_surface);
	CHECK_RET(restored, false);

	HRESULT const blted = dd_primary_surface->lpVtbl->Blt(dd_primary_surface, &r, dd_back_surface, nullptr, DDBLT_WAIT, nullptr);
	CHECK_RET(blted == DD_OK || check_ret_hresult_failed(blted), false);

	return true;
}

extern "C" LRESULT CALLBACK main_window_proc(_In_ HWND const hwnd, _In_ UINT const msg, _In_ WPARAM const w_param, _In_ LPARAM const l_param)
{
	switch(msg)
	{
		case WM_DESTROY:
		{
			PostQuitMessage(0);
		}
		break;
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			big_state_t& big_state = *g_big_state;
			bool const is_pressed = msg == WM_KEYDOWN;
			static constexpr WPARAM const s_move_forward[] = {'W'};
			static constexpr WPARAM const s_move_backward[] = {'S'};
			static constexpr WPARAM const s_move_left[] = {'A'};
			static constexpr WPARAM const s_move_right[] = {'D'};
			static constexpr WPARAM const s_move_down[] = {'Q'};
			static constexpr WPARAM const s_move_up[] = {'E'};
			static constexpr WPARAM const s_rotate_yaw_left[] = {'J'};
			static constexpr WPARAM const s_rotate_yaw_right[] = {'L'};
			static constexpr WPARAM const s_rotate_pitch_down[] = {'K'};
			static constexpr WPARAM const s_rotate_pitch_up[] = {'I'};
			static constexpr WPARAM const s_rotate_roll_left[] = {'U'};
			static constexpr WPARAM const s_rotate_roll_right[] = {'O'};
			static constexpr WPARAM const s_reset[] = {'R'};
			if(std::find(std::cbegin(s_move_forward), std::cend(s_move_forward), w_param) != std::cend(s_move_forward)) big_state.m_move_forward = is_pressed;
			if(std::find(std::cbegin(s_move_backward), std::cend(s_move_backward), w_param) != std::cend(s_move_backward)) big_state.m_move_backward = is_pressed;
			if(std::find(std::cbegin(s_move_left), std::cend(s_move_left), w_param) != std::cend(s_move_left)) big_state.m_move_left = is_pressed;
			if(std::find(std::cbegin(s_move_right), std::cend(s_move_right), w_param) != std::cend(s_move_right)) big_state.m_move_right = is_pressed;
			if(std::find(std::cbegin(s_move_down), std::cend(s_move_down), w_param) != std::cend(s_move_down)) big_state.m_move_down = is_pressed;
			if(std::find(std::cbegin(s_move_up), std::cend(s_move_up), w_param) != std::cend(s_move_up)) big_state.m_move_up = is_pressed;
			if(std::find(std::cbegin(s_rotate_yaw_left), std::cend(s_rotate_yaw_left), w_param) != std::cend(s_rotate_yaw_left)) big_state.m_rotate_yaw_left = is_pressed;
			if(std::find(std::cbegin(s_rotate_yaw_right), std::cend(s_rotate_yaw_right), w_param) != std::cend(s_rotate_yaw_right)) big_state.m_rotate_yaw_right = is_pressed;
			if(std::find(std::cbegin(s_rotate_pitch_down), std::cend(s_rotate_pitch_down), w_param) != std::cend(s_rotate_pitch_down)) big_state.m_rotate_pitch_down = is_pressed;
			if(std::find(std::cbegin(s_rotate_pitch_up), std::cend(s_rotate_pitch_up), w_param) != std::cend(s_rotate_pitch_up)) big_state.m_rotate_pitch_up = is_pressed;
			if(std::find(std::cbegin(s_rotate_roll_left), std::cend(s_rotate_roll_left), w_param) != std::cend(s_rotate_roll_left)) big_state.m_rotate_roll_left = is_pressed;
			if(std::find(std::cbegin(s_rotate_roll_right), std::cend(s_rotate_roll_right), w_param) != std::cend(s_rotate_roll_right)) big_state.m_rotate_roll_right = is_pressed;
			if(is_pressed && std::find(std::cbegin(s_reset), std::cend(s_reset), w_param) != std::cend(s_reset)) big_state.m_d3d7_transform_view = d3d_matrix_view(D3DVECTOR{0.0f, 0.0f, 0.0f}, D3DVECTOR{0.0f, 0.0f, 1.0f}, D3DVECTOR{0.0f, 1.0f, 0.0f});
		}
		break;
	}
	LRESULT const ret = DefWindowProcW(hwnd, msg, w_param, l_param);
	return ret;
}

bool run_main_loop(int* const& out_exit_code)
{
	for(;;)
	{
		MSG msg;
		BOOL const got = PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE);
		if(got != 0)
		{
			if(msg.message != WM_QUIT)
			{
				BOOL const translated = TranslateMessage(&msg);
				LRESULT const dispatched = DispatchMessageW(&msg);
			}
			else
			{
				CHECK_RET(msg.hwnd == nullptr, false);
				int const exit_code = static_cast<int>(msg.wParam);
				*out_exit_code = exit_code;
				return true;
			}
		}
		else
		{
			bool const idled = on_idle();
			CHECK_RET(idled, false);
		}
	}
}

bool on_idle()
{
	big_state_t& big_state = *g_big_state;
	bool const rendered = render(&big_state.m_prev_tick_count);
	CHECK_RET(rendered, false);

	return true;
}

float d3d_vector_magnitude_sqr(D3DVECTOR const& v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

float d3d_vector_magnitude(D3DVECTOR const& v)
{
	return std::sqrt(d3d_vector_magnitude_sqr(v));
}

D3DMATRIX const& d3d_matrix_identity()
{
	static constexpr D3DMATRIX const s_d3d_matrix_identity
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	return s_d3d_matrix_identity;
}

D3DVECTOR operator-(D3DVECTOR const& a, D3DVECTOR const& b)
{
	return D3DVECTOR{a.x - b.x, a.y - b.y, a.z - b.z};
}

D3DVECTOR operator*(D3DVALUE const& a, D3DVECTOR const& b)
{
	return D3DVECTOR{a * b.x, a * b.y, a * b.z};
}

D3DVECTOR d3d_vector_normalize(D3DVECTOR const& v)
{
	float const magnitude = d3d_vector_magnitude(v);
	assert(magnitude >= 1e-6f);
	return D3DVECTOR{v.x / magnitude, v.y / magnitude, v.z / magnitude};
}

D3DVECTOR d3d_vector_cross_product(D3DVECTOR const& a, D3DVECTOR const& b)
{
	return D3DVECTOR
	{
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	};
}

float d3d_vector_dot_product(D3DVECTOR const& a, D3DVECTOR const& b)
{
	return
		a.x * b.x +
		a.y * b.y +
		a.z * b.z;
}

float& d3d_matrix_index(D3DMATRIX& m, int const& row, int const& col)
{
	struct my_matrix_t
	{
		float m_data[4][4];
	};
	return reinterpret_cast<my_matrix_t&>(m).m_data[row][col];
}

float const& d3d_matrix_index(D3DMATRIX const& m, int const& row, int const& col)
{
	struct my_matrix_t
	{
		float m_data[4][4];
	};
	return reinterpret_cast<my_matrix_t const&>(m).m_data[row][col];
}

D3DMATRIX d3d_matrix_translate(float const& dx, float const& dy, float const& dz)
{
	D3DMATRIX ret = d3d_matrix_identity();
	d3d_matrix_index(ret, 3, 0) = dx;
	d3d_matrix_index(ret, 3, 1) = dy;
	d3d_matrix_index(ret, 3, 2) = dz;
	return ret;
}

D3DMATRIX d3d_matrix_rotate_x(float const& rads)
{
	float const cosine = std::cos(rads);
	float const sine = std::sin(rads);
	D3DMATRIX ret = d3d_matrix_identity();
	d3d_matrix_index(ret, 0, 0) = 1.0f;
	d3d_matrix_index(ret, 1, 1) = cosine;
	d3d_matrix_index(ret, 1, 2) = -sine;
	d3d_matrix_index(ret, 2, 1) = sine;
	d3d_matrix_index(ret, 2, 2) = cosine;
	d3d_matrix_index(ret, 3, 3) = 1.0f;
	return ret;
}

D3DMATRIX d3d_matrix_rotate_y(float const& rads)
{
	float const cosine = std::cos(rads);
	float const sine = std::sin(rads);
	D3DMATRIX ret = d3d_matrix_identity();
	d3d_matrix_index(ret, 0, 0) = cosine;
	d3d_matrix_index(ret, 0, 2) = sine;
	d3d_matrix_index(ret, 1, 1) = 1.0f;
	d3d_matrix_index(ret, 2, 0) = -sine;
	d3d_matrix_index(ret, 2, 2) = cosine;
	d3d_matrix_index(ret, 3, 3) = 1.0f;
	return ret;
}

D3DMATRIX d3d_matrix_rotate_z(float const& rads)
{
	float const cosine = std::cos(rads);
	float const sine = std::sin(rads);
	D3DMATRIX ret{};
	d3d_matrix_index(ret, 0, 0) = cosine;
	d3d_matrix_index(ret, 0, 1) = -sine;
	d3d_matrix_index(ret, 1, 0) = sine;
	d3d_matrix_index(ret, 1, 1) = cosine;
	d3d_matrix_index(ret, 2, 2) = 1.0f;
	d3d_matrix_index(ret, 3, 3) = 1.0f;
	return ret;
}

D3DMATRIX d3d_matrix_multiply(D3DMATRIX const& a, D3DMATRIX const& b)
{
	D3DMATRIX ret{};
	for(int i = 0; i < 4; i++)
	{
		for(int j = 0; j < 4; j++)
		{
			for(int k = 0; k < 4; k++)
			{
				d3d_matrix_index(ret, i, j) += d3d_matrix_index(a, k, j) * d3d_matrix_index(b, i, k);
			}
		}
	}
	return ret;
}

D3DMATRIX d3d_matrix_view(D3DVECTOR const& from, D3DVECTOR const& at, D3DVECTOR const& world_up)
{
	D3DVECTOR const view_dir = d3d_vector_normalize(at - from);
	D3DVECTOR const up = d3d_vector_normalize(world_up - d3d_vector_dot_product(world_up, view_dir) * view_dir);
	D3DVECTOR const right = d3d_vector_cross_product(up, view_dir);

	D3DMATRIX view = d3d_matrix_identity();
	d3d_matrix_index(view, 0, 0) = right.x;
	d3d_matrix_index(view, 1, 0) = right.y;
	d3d_matrix_index(view, 2, 0) = right.z;
	d3d_matrix_index(view, 0, 1) = up.x;
	d3d_matrix_index(view, 1, 1) = up.y;
	d3d_matrix_index(view, 2, 1) = up.z;
	d3d_matrix_index(view, 0, 2) = view_dir.x;
	d3d_matrix_index(view, 1, 2) = view_dir.y;
	d3d_matrix_index(view, 2, 2) = view_dir.z;
	d3d_matrix_index(view, 3, 0) = -d3d_vector_dot_product(from, right);
	d3d_matrix_index(view, 3, 1) = -d3d_vector_dot_product(from, up);
	d3d_matrix_index(view, 3, 2) = -d3d_vector_dot_product(from, view_dir);

	return view;
}

D3DMATRIX d3d_matrix_projection(float const& fov_hor, float const& fov_ver, float const& near_plane, float const& far_plane)
{
	assert(std::abs(far_plane - near_plane) >= 0.01f);

	float const w = std::cos(fov_hor * 0.5f) / std::sin(fov_hor * 0.5f);
	float const h = std::cos(fov_ver * 0.5f) / std::sin(fov_ver * 0.5f);
	float const q = far_plane / (far_plane - near_plane);
	D3DMATRIX ret{};
	d3d_matrix_index(ret, 0, 0) = w;
	d3d_matrix_index(ret, 1, 1) = h;
	d3d_matrix_index(ret, 2, 2) = q;
	d3d_matrix_index(ret, 2, 3) = 1.0f;
	d3d_matrix_index(ret, 3, 2) = -q * near_plane;
	return ret;
}

bool render(DWORD* const& prev_tick_count)
{
	big_state_t& big_state = *g_big_state;

	DWORD const tick_count = GetTickCount();
	DWORD const elapsed_u = tick_count - *prev_tick_count + 1;
	*prev_tick_count = tick_count;
	float const elapsed_f = static_cast<float>(elapsed_u);
	float const move_speed = elapsed_f / 100.0f;
	float const rotate_speed = elapsed_f / 500.0f;

	bool const restored = restore_surfaces(big_state.m_dd7_primary_surface, big_state.m_dd7_back_surface);
	CHECK_RET(restored, false);

	if(g_big_state->m_move_forward) big_state.m_d3d7_transform_view = d3d_matrix_multiply(d3d_matrix_translate(0.0f, 0.0f, -move_speed), big_state.m_d3d7_transform_view);
	if(g_big_state->m_move_backward) big_state.m_d3d7_transform_view = d3d_matrix_multiply(d3d_matrix_translate(0.0f, 0.0f, +move_speed), big_state.m_d3d7_transform_view);
	if(g_big_state->m_move_left) big_state.m_d3d7_transform_view = d3d_matrix_multiply(d3d_matrix_translate(+move_speed, 0.0f, 0.0f), big_state.m_d3d7_transform_view);
	if(g_big_state->m_move_right) big_state.m_d3d7_transform_view = d3d_matrix_multiply(d3d_matrix_translate(-move_speed, 0.0f, 0.0f), big_state.m_d3d7_transform_view);
	if(g_big_state->m_move_down) big_state.m_d3d7_transform_view = d3d_matrix_multiply(d3d_matrix_translate(0.0f, +move_speed, 0.0f), big_state.m_d3d7_transform_view);
	if(g_big_state->m_move_up) big_state.m_d3d7_transform_view = d3d_matrix_multiply(d3d_matrix_translate(0.0f, -move_speed, 0.0f), big_state.m_d3d7_transform_view);
	if(g_big_state->m_rotate_yaw_left) big_state.m_d3d7_transform_view = d3d_matrix_multiply(d3d_matrix_rotate_y(-rotate_speed), big_state.m_d3d7_transform_view);
	if(g_big_state->m_rotate_yaw_right) big_state.m_d3d7_transform_view = d3d_matrix_multiply(d3d_matrix_rotate_y(+rotate_speed), big_state.m_d3d7_transform_view);
	if(g_big_state->m_rotate_pitch_down) big_state.m_d3d7_transform_view = d3d_matrix_multiply(d3d_matrix_rotate_x(+rotate_speed), big_state.m_d3d7_transform_view);
	if(g_big_state->m_rotate_pitch_up) big_state.m_d3d7_transform_view = d3d_matrix_multiply(d3d_matrix_rotate_x(-rotate_speed), big_state.m_d3d7_transform_view);
	if(g_big_state->m_rotate_roll_left) big_state.m_d3d7_transform_view = d3d_matrix_multiply(d3d_matrix_rotate_z(+rotate_speed), big_state.m_d3d7_transform_view);
	if(g_big_state->m_rotate_roll_right) big_state.m_d3d7_transform_view = d3d_matrix_multiply(d3d_matrix_rotate_z(-rotate_speed), big_state.m_d3d7_transform_view);

	HRESULT const d3d_transformed_view = big_state.m_d3d7_device->lpVtbl->SetTransform(big_state.m_d3d7_device, D3DTRANSFORMSTATE_VIEW, &big_state.m_d3d7_transform_view);
	CHECK_RET(d3d_transformed_view == D3D_OK || check_ret_hresult_failed(d3d_transformed_view), false);

	{
		HRESULT const started = big_state.m_d3d7_device->lpVtbl->BeginScene(big_state.m_d3d7_device);
		CHECK_RET(started == D3D_OK || check_ret_hresult_failed(started), false);
		auto const end_scene = mk::make_scope_exit([&](){ HRESULT const ended = big_state.m_d3d7_device->lpVtbl->EndScene(big_state.m_d3d7_device); CHECK_RET_V(ended == D3D_OK || check_ret_hresult_failed(ended)); });

		DDBLTFX dd_bltfx{};
		dd_bltfx.dwSize = sizeof(dd_bltfx);
		dd_bltfx.dwFillColor = RGB(255, 0, 0);
		HRESULT const cleared = big_state.m_dd7_back_surface->lpVtbl->Blt(big_state.m_dd7_back_surface, nullptr, nullptr, nullptr, DDBLT_WAIT | DDBLT_COLORFILL, &dd_bltfx);
		CHECK_RET(cleared == DD_OK || check_ret_hresult_failed(cleared), false);

		HRESULT const d3d7_texture_set = big_state.m_d3d7_device->lpVtbl->SetTexture(big_state.m_d3d7_device, 0, nullptr);
		CHECK_RET(d3d7_texture_set == D3D_OK || check_ret_hresult_failed(d3d7_texture_set), false);

		HRESULT const d3d7_vertex_buffer_drawn = big_state.m_d3d7_device->lpVtbl->DrawIndexedPrimitiveVB(big_state.m_d3d7_device, D3DPT_TRIANGLELIST, big_state.m_d3d7_vertex_buffer, 0, (s_flag_size + 1) * (s_flag_size + 1), big_state.m_d3d7_vertex_buffer_indices, s_flag_size * s_flag_size * 6, D3DDP_WAIT);
		CHECK_RET(d3d7_vertex_buffer_drawn == D3D_OK || check_ret_hresult_failed(d3d7_vertex_buffer_drawn), false);
	}

	bool const flipped = flip_surfaces(big_state.m_main_window, big_state.m_dd7_primary_surface, big_state.m_dd7_back_surface);
	CHECK_RET(flipped, false);

	return true;
}

bool restore_surface(LPDIRECTDRAWSURFACE7 const& dd_surface)
{
	HRESULT const is_lost = dd_surface->lpVtbl->IsLost(dd_surface);
	CHECK_RET(is_lost == DD_OK || is_lost == DDERR_SURFACELOST || check_ret_hresult_failed(is_lost), false);
	if(is_lost == DDERR_SURFACELOST)
	{
		HRESULT const restored = dd_surface->lpVtbl->Restore(dd_surface);
		CHECK_RET(restored == DD_OK || check_ret_hresult_failed(restored), false);
	}
	return true;
}

bool restore_surfaces(LPDIRECTDRAWSURFACE7 const& dd_primary_surface, LPDIRECTDRAWSURFACE7 const& dd_back_surface)
{
	bool const primary_restored = restore_surface(dd_primary_surface);
	CHECK_RET(primary_restored, false);

	bool const back_restored = restore_surface(dd_back_surface);
	CHECK_RET(primary_restored, false);

	return true;
}

std::string guid_to_string(GUID const& guid)
{
	static constexpr int const s_capacity = 1 + 8 + 1 + 4 + 1 + 4 + 1 + 2 + 2 + 1 + 2 + 2 + 2 + 2 + 2 + 2 + 1;
	static constexpr int const s_buff_size = s_capacity + 1;
	std::string ret;
	ret.resize(s_capacity);
	int const printed = std::snprintf(ret.data(), s_buff_size, "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
	assert(printed >= 0 && printed < s_buff_size);
	return ret;
}

std::string guid_to_device_type_string(GUID const& guid)
{
	struct name_len_t
	{
		char const* const m_name;
		unsigned int const m_len;
	};
	static constexpr GUID const* const s_guids[] =
	{
		&IID_IDirect3DRampDevice,
		&IID_IDirect3DRGBDevice,
		&IID_IDirect3DHALDevice,
		&IID_IDirect3DMMXDevice,
		&IID_IDirect3DRefDevice,
		&IID_IDirect3DNullDevice,
		&IID_IDirect3DTnLHalDevice,
	};
	static constexpr char const s_0_name[] = "IDirect3DRampDevice";
	static constexpr char const s_1_name[] = "IDirect3DRGBDevice";
	static constexpr char const s_2_name[] = "IDirect3DHALDevice";
	static constexpr char const s_3_name[] = "IDirect3DMMXDevice";
	static constexpr char const s_4_name[] = "IDirect3DRefDevice";
	static constexpr char const s_5_name[] = "IDirect3DNullDevice";
	static constexpr char const s_6_name[] = "IDirect3DTnLHalDevice";
	static constexpr name_len_t const s_names[] =
	{
		{s_0_name, static_cast<unsigned int>(std::size(s_0_name)) - 1},
		{s_1_name, static_cast<unsigned int>(std::size(s_1_name)) - 1},
		{s_2_name, static_cast<unsigned int>(std::size(s_2_name)) - 1},
		{s_3_name, static_cast<unsigned int>(std::size(s_3_name)) - 1},
		{s_4_name, static_cast<unsigned int>(std::size(s_4_name)) - 1},
		{s_5_name, static_cast<unsigned int>(std::size(s_5_name)) - 1},
		{s_6_name, static_cast<unsigned int>(std::size(s_6_name)) - 1},
	};
	static constexpr char const s_x_name[] = "IUnknown";
	static constexpr unsigned int const s_x_name_len = static_cast<unsigned int>(std::size(s_x_name)) - 1;
	using std::cbegin;
	using std::cend;
	auto const it = std::find_if(cbegin(s_guids), cend(s_guids), [&](auto const& e){ return *e == guid; });
	if(it == cend(s_guids))
	{
		return std::string{s_x_name, s_x_name_len};
	}
	auto const idx = it - cbegin(s_guids);
	return std::string{s_names[idx].m_name, s_names[idx].m_len};
}

bool big_state_init(driver_mode_e const& driver_mode, int const& driver_index, int const& device_index, HWND const& main_window, big_state_pointer_t* const& out_big_state_pointer)
{
	big_state_pointer_t big_state_pointer{new big_state_t};
	big_state_t& big_state = *big_state_pointer;
	std::memset(&big_state, 0, sizeof(big_state));
	big_state.m_driver_mode = driver_mode;
	big_state.m_driver_index = driver_index;
	big_state.m_device_index = device_index;
	big_state.m_main_window = main_window;

	struct driver_description_t
	{
		std::optional<GUID> m_dd_guid;
		std::string m_driver_description;
		std::string m_driver_name;
		HMONITOR m_monitor;
	};
	std::vector<driver_description_t> drivers;
	static constexpr auto const dd_enum_callback_fn = [](GUID* const guid, LPSTR const driver_description, LPSTR const driver_name, LPVOID const context, HMONITOR const monitor) -> BOOL
	{
		std::vector<driver_description_t>& drivers = *static_cast<std::vector<driver_description_t>*>(context);
		drivers.push_back(driver_description_t{guid ? std::optional<GUID>{*guid} : std::optional<GUID>{std::nullopt}, std::string{driver_description}, std::string{driver_name}, HMONITOR{monitor}});
		return DDENUMRET_OK;
	};
	LPDDENUMCALLBACKEXA const dd_enum_callback = dd_enum_callback_fn;
	LPVOID const dd_enum_callback_context = &drivers;
	DWORD const dd_enum_callback_flags = DDENUM_ATTACHEDSECONDARYDEVICES | DDENUM_DETACHEDSECONDARYDEVICES | DDENUM_NONDISPLAYDEVICES;
	HRESULT const dd_enumerated = DirectDrawEnumerateExA(dd_enum_callback, dd_enum_callback_context, dd_enum_callback_flags);
	CHECK_RET(dd_enumerated == DD_OK || check_ret_hresult_failed(dd_enumerated), false);
	CHECK_RET(!drivers.empty(), false);

	static constexpr char const s_null[] = "null";
	static constexpr char const* const s_null_ptr = s_null;

	std::printf("Found %d drivers.\n", static_cast<int>(drivers.size()));
	for(auto const& driver : drivers)
	{
		std::printf
		(
			"Name `%s`, description `%s`, guid `%s`, monitor %s.\n",
			driver.m_driver_name.c_str(),
			driver.m_driver_description.c_str(), driver.m_dd_guid ? guid_to_string(*driver.m_dd_guid).c_str() : s_null_ptr,
			driver.m_monitor != nullptr ? s_yes_ptr : s_no_ptr
		);
	}

	GUID* const driver = [&]() -> GUID*
	{
		if(big_state.m_driver_mode == driver_mode_e::mode_default)
		{
			return static_cast<GUID*>(nullptr);
		}
		if(big_state.m_driver_mode == driver_mode_e::mode_software)
		{
			return reinterpret_cast<GUID*>(DDCREATE_EMULATIONONLY);
		}
		if(big_state.m_driver_mode == driver_mode_e::mode_hardware)
		{
			return reinterpret_cast<GUID*>(DDCREATE_HARDWAREONLY);
		}
		if(big_state.m_driver_mode == driver_mode_e::mode_index)
		{
			CHECK_RET_V(big_state.m_driver_index >= 0 && big_state.m_driver_index < static_cast<int>(drivers.size()));
			if(drivers[big_state.m_driver_index].m_dd_guid)
			{
				return &*drivers[big_state.m_driver_index].m_dd_guid;
			}
			else
			{
				return nullptr;
			}
		}
		CHECK_RET_V(false);
	}();

	LPDIRECTDRAW7 dd7;
	HRESULT const dd7_created = DirectDrawCreateEx(driver, reinterpret_cast<void**>(&dd7), IID_IDirectDraw7, nullptr);
	CHECK_RET(dd7_created == DD_OK || check_ret_hresult_failed(dd7_created), false);
	big_state.m_dd7 = dd7;

	LPDIRECT3D7 d3d7;
	HRESULT const d3d7_casted = dd7->lpVtbl->QueryInterface(dd7, IID_IDirect3D7, reinterpret_cast<void**>(&d3d7));
	CHECK_RET(d3d7_casted == S_OK || check_ret_hresult_failed(d3d7_casted), false);
	big_state.m_d3d7 = d3d7;

	DDCAPS dd7_caps_hw{};
	dd7_caps_hw.dwSize = sizeof(dd7_caps_hw);
	DDCAPS dd7_caps_hel{};
	dd7_caps_hel.dwSize = sizeof(dd7_caps_hel);
	HRESULT const dd7_got_caps = dd7->lpVtbl->GetCaps(dd7, &dd7_caps_hw, &dd7_caps_hel);
	CHECK_RET(dd7_got_caps == DD_OK || check_ret_hresult_failed(dd7_got_caps), false);
	CHECK_RET((dd7_caps_hw.dwCaps2 & DDCAPS2_CANRENDERWINDOWED) != 0, false);

	struct device_description_t
	{
		std::string m_device_description;
		std::string m_device_name;
		D3DDEVICEDESC7 m_d3d7_device_capabilities;
	};
	std::vector<device_description_t> devices;
	static constexpr auto const d3d7_enum_callback_fn = [](char* const device_description, char* const device_name, LPD3DDEVICEDESC7 const device_capabilities, void* const context) -> HRESULT
	{
		std::vector<device_description_t>& devices = *static_cast<std::vector<device_description_t>*>(context);
		devices.push_back(device_description_t{std::string{device_description}, std::string{device_name}, D3DDEVICEDESC7{*device_capabilities}});
		return D3DENUMRET_OK;
	};
	LPD3DENUMDEVICESCALLBACK7 const d3d7_enum_callback = d3d7_enum_callback_fn;
	void* const d3d7_enum_callback_ctx = &devices;
	HRESULT const d3d7_enumed = d3d7->lpVtbl->EnumDevices(d3d7, d3d7_enum_callback, d3d7_enum_callback_ctx);
	CHECK_RET(d3d7_enumed == D3D_OK || check_ret_hresult_failed(d3d7_enumed), false);
	CHECK_RET(!devices.empty(), false);
	CHECK_RET(big_state.m_device_index >= 0 && big_state.m_device_index < static_cast<int>(devices.size()), false);

	std::printf("Found %d devices.\n", static_cast<int>(devices.size()));
	for(auto const& device : devices)
	{
		std::printf
		(
			"Name `%s`, description `%s`, guid `%s`, interface `%s`, device capabilities D3DDEVCAPS_HWRASTERIZATION %s.\n",
			device.m_device_name.c_str(),
			device.m_device_description.c_str(),
			guid_to_string(device.m_d3d7_device_capabilities.deviceGUID).c_str(),
			guid_to_device_type_string(device.m_d3d7_device_capabilities.deviceGUID).c_str(),
			((device.m_d3d7_device_capabilities.dwDevCaps & D3DDEVCAPS_HWRASTERIZATION) != 0) ? s_yes_ptr : s_no_ptr
		);
	}

	bool const d3d7_device_is_hw = (devices[big_state.m_device_index].m_d3d7_device_capabilities.dwDevCaps & D3DDEVCAPS_HWRASTERIZATION) != 0;

	HRESULT const dd7_cooperation = dd7->lpVtbl->SetCooperativeLevel(dd7, big_state.m_main_window, DDSCL_NORMAL | DDSCL_FPUSETUP);
	CHECK_RET(dd7_cooperation == DD_OK || check_ret_hresult_failed(dd7_cooperation), false);

	DDSURFACEDESC2 dd7_surface_desc{};
	dd7_surface_desc.dwSize = sizeof(dd7_surface_desc);
	HRESULT const dd7_got_display_mode = dd7->lpVtbl->GetDisplayMode(dd7, &dd7_surface_desc);
	CHECK_RET(dd7_got_display_mode == DD_OK || check_ret_hresult_failed(dd7_got_display_mode), false);
	CHECK_RET(dd7_surface_desc.ddpfPixelFormat.dwRGBBitCount > 8, false);

	LPDIRECTDRAWSURFACE7 dd7_primary_surface;
	DDSURFACEDESC2 dd7_primary_surface_description{};
	dd7_primary_surface_description.dwSize = sizeof(dd7_primary_surface_description);
	dd7_primary_surface_description.dwFlags = DDSD_CAPS;
	dd7_primary_surface_description.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	HRESULT const dd7_primary_surface_created = dd7->lpVtbl->CreateSurface(dd7, &dd7_primary_surface_description, &dd7_primary_surface, nullptr);
	CHECK_RET(dd7_primary_surface_created == DD_OK || check_ret_hresult_failed(dd7_primary_surface_created), false);
	big_state.m_dd7_primary_surface = dd7_primary_surface;

	LPDIRECTDRAWCLIPPER dd7_clipper;
	HRESULT const dd7_clipper_created = dd7->lpVtbl->CreateClipper(dd7, 0, &dd7_clipper, nullptr);
	CHECK_RET(dd7_clipper_created == DD_OK || check_ret_hresult_failed(dd7_clipper_created), false);
	big_state.m_dd7_clipper = dd7_clipper;

	HRESULT const dd7_clipper_set = dd7_clipper->lpVtbl->SetHWnd(dd7_clipper, 0, big_state.m_main_window);
	CHECK_RET(dd7_clipper_set == DD_OK || check_ret_hresult_failed(dd7_clipper_set), false);

	HRESULT const dd7_clipper_assigned = dd7_primary_surface->lpVtbl->SetClipper(dd7_primary_surface, dd7_clipper);
	CHECK_RET(dd7_clipper_assigned == DD_OK || check_ret_hresult_failed(dd7_clipper_assigned), false);

	bool const use_video_memory =
		devices[big_state.m_device_index].m_d3d7_device_capabilities.deviceGUID == IID_IDirect3DTnLHalDevice ||
		devices[big_state.m_device_index].m_d3d7_device_capabilities.deviceGUID == IID_IDirect3DHALDevice;

	RECT r;
	BOOL const got_rect = GetClientRect(main_window, &r);
	CHECK_RET(got_rect != 0, false);
	int const width = r.right - r.left;
	int const height = r.bottom - r.top;

	DDSURFACEDESC2 dd7_back_surface_description{};
	dd7_back_surface_description.dwSize = sizeof(dd7_back_surface_description);
	dd7_back_surface_description.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	dd7_back_surface_description.dwHeight = height;
	dd7_back_surface_description.dwWidth = width;
	dd7_back_surface_description.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE | (use_video_memory ? DDSCAPS_VIDEOMEMORY : DDSCAPS_SYSTEMMEMORY);
	LPDIRECTDRAWSURFACE7 dd7_back_surface;
	HRESULT const dd7_back_surface_created = dd7->lpVtbl->CreateSurface(dd7, &dd7_back_surface_description, &dd7_back_surface, nullptr);
	CHECK_RET(dd7_back_surface_created == DD_OK || check_ret_hresult_failed(dd7_back_surface_created), false);
	big_state.m_dd7_back_surface = dd7_back_surface;

	LPDIRECT3DDEVICE7 d3d7_device;
	HRESULT const d3d7_device_created = d3d7->lpVtbl->CreateDevice(d3d7, devices[big_state.m_device_index].m_d3d7_device_capabilities.deviceGUID, dd7_back_surface, &d3d7_device);
	CHECK_RET(d3d7_device_created == D3D_OK || check_ret_hresult_failed(d3d7_device_created), false);
	big_state.m_d3d7_device = d3d7_device;

	D3DVIEWPORT7 d3d7_viewport;
	d3d7_viewport.dwX = 0;
	d3d7_viewport.dwY = 0;
	d3d7_viewport.dwWidth = width;
	d3d7_viewport.dwHeight = height;
	d3d7_viewport.dvMinZ = 0.0f;
	d3d7_viewport.dvMaxZ = 1.0f;
	HRESULT const d3d7_set_viewport = d3d7_device->lpVtbl->SetViewport(d3d7_device, &d3d7_viewport);
	CHECK_RET(d3d7_set_viewport == D3D_OK || check_ret_hresult_failed(d3d7_set_viewport), false);

	LPDIRECT3DVERTEXBUFFER7 d3d7_vertex_buffer;
	D3DVERTEXBUFFERDESC d3d7_vertex_buffer_description{};
	d3d7_vertex_buffer_description.dwSize = sizeof(d3d7_vertex_buffer_description);
	d3d7_vertex_buffer_description.dwCaps = D3DVBCAPS_DONOTCLIP | (big_state.m_driver_mode == driver_mode_e::mode_software ? D3DVBCAPS_SYSTEMMEMORY : 0) | D3DVBCAPS_WRITEONLY;
	d3d7_vertex_buffer_description.dwFVF = D3DFVF_VERTEX;
	d3d7_vertex_buffer_description.dwNumVertices = (s_flag_size + 1) * (s_flag_size + 1);
	HRESULT const d3d7_vertex_buffer_created = d3d7->lpVtbl->CreateVertexBuffer(d3d7, &d3d7_vertex_buffer_description, &d3d7_vertex_buffer, 0);
	CHECK_RET(d3d7_vertex_buffer_created == D3D_OK || check_ret_hresult_failed(d3d7_vertex_buffer_created), false);
	big_state.m_d3d7_vertex_buffer = d3d7_vertex_buffer;

	{
		void* d3d7_vertex_buffer_memory;
		DWORD d3d7_vertex_buffer_memory_size;
		DWORD const d3d7_vertex_buffer_lock_flags = DDLOCK_SURFACEMEMORYPTR | DDLOCK_NOSYSLOCK | DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_DISCARDCONTENTS;
		HRESULT const d3d7_vertex_buffer_locked = d3d7_vertex_buffer->lpVtbl->Lock(d3d7_vertex_buffer, d3d7_vertex_buffer_lock_flags, &d3d7_vertex_buffer_memory, &d3d7_vertex_buffer_memory_size);
		CHECK_RET(d3d7_vertex_buffer_locked == D3D_OK || check_ret_hresult_failed(d3d7_vertex_buffer_locked), false);
		CHECK_RET(d3d7_vertex_buffer_memory_size == (s_flag_size + 1) * (s_flag_size + 1) * sizeof(D3DVERTEX), false);
		CHECK_RET((reinterpret_cast<std::uintptr_t>(d3d7_vertex_buffer_memory) % alignof(D3DVERTEX)) == 0, false);
		auto const d3d7_vertex_buffer_unlock = mk::make_scope_exit([&](){ HRESULT const d3d7_vertex_buffer_unlocked = d3d7_vertex_buffer->lpVtbl->Unlock(d3d7_vertex_buffer); CHECK_RET_V(d3d7_vertex_buffer_unlocked == D3D_OK || check_ret_hresult_failed(d3d7_vertex_buffer_unlocked)); });

		D3DVERTEX* const vertices = static_cast<D3DVERTEX*>(d3d7_vertex_buffer_memory);
		for(int ix = 0; ix != s_flag_size + 1; ++ix)
		{
			for(int iy = 0; iy != s_flag_size + 1; ++iy)
			{
				float const tu = static_cast<float>(ix) / static_cast<float>(s_flag_size);
				float const tv = static_cast<float>(iy) / static_cast<float>(s_flag_size);
				float const x = 0.2f + tu * 3.31f;
				float const y = 8.0f + tv * 1.82f;
				vertices[ix + iy * (s_flag_size + 1)] = D3DVERTEX{static_cast<float>(x), static_cast<float>(y), 0.0f, 0.0f, 0.0f, -1.0f, tu, 1.0f - tv};
			}
		}
	}

	HRESULT const d3d7_vertex_buffer_optimized = d3d7_vertex_buffer->lpVtbl->Optimize(d3d7_vertex_buffer, d3d7_device, 0);
	CHECK_RET(d3d7_vertex_buffer_optimized == D3D_OK || check_ret_hresult_failed(d3d7_vertex_buffer_optimized), false);

	WORD* d3d7_vertex_buffer_indices = new WORD[s_flag_size * s_flag_size * 6];
	big_state.m_d3d7_vertex_buffer_indices = d3d7_vertex_buffer_indices;
	std::array<WORD, s_flag_size * s_flag_size * 6>& d3d7_vertex_buffer_indices_view = *reinterpret_cast<std::array<WORD, s_flag_size * s_flag_size * 6>*>(d3d7_vertex_buffer_indices);
	for(int i = 0, ix = 0; ix != s_flag_size; ++ix)
	{
		for(int iy = 0; iy != s_flag_size; ++iy)
		{
			d3d7_vertex_buffer_indices_view[i++] = (ix + 0) + (iy + 1) * (s_flag_size + 1);
			d3d7_vertex_buffer_indices_view[i++] = (ix + 1) + (iy + 0) * (s_flag_size + 1);
			d3d7_vertex_buffer_indices_view[i++] = (ix + 0) + (iy + 0) * (s_flag_size + 1);
			d3d7_vertex_buffer_indices_view[i++] = (ix + 0) + (iy + 1) * (s_flag_size + 1);
			d3d7_vertex_buffer_indices_view[i++] = (ix + 1) + (iy + 1) * (s_flag_size + 1);
			d3d7_vertex_buffer_indices_view[i++] = (ix + 1) + (iy + 0) * (s_flag_size + 1);
		}
	}

	D3DMATRIX& d3d7_transform_world = big_state.m_d3d7_transform_world;
	d3d7_transform_world = d3d_matrix_translate(0.0f, 0.0f, 0.0f);
	HRESULT const d3d7_set_transform_world = d3d7_device->lpVtbl->SetTransform(d3d7_device, D3DTRANSFORMSTATE_WORLD, &d3d7_transform_world);
	CHECK_RET(d3d7_set_transform_world == D3D_OK || check_ret_hresult_failed(d3d7_set_transform_world), false);

	D3DMATRIX& d3d7_transform_view = big_state.m_d3d7_transform_view;
	d3d7_transform_view = d3d_matrix_view(D3DVECTOR{-1.0f, 7.5f, -3.0f}, D3DVECTOR{2.0f, 7.5f, 0.0f}, D3DVECTOR{0.0f, 1.0f, 0.0f});
	HRESULT const d3d7_set_transform_view = d3d7_device->lpVtbl->SetTransform(d3d7_device, D3DTRANSFORMSTATE_VIEW, &d3d7_transform_view);
	CHECK_RET(d3d7_set_transform_view == D3D_OK || check_ret_hresult_failed(d3d7_set_transform_view), false);

	D3DMATRIX& d3d7_transform_projection = big_state.m_d3d7_transform_projection;
	d3d7_transform_projection = d3d_matrix_projection(90.0f * (static_cast<float>(3.14159265358979323846) / 180.0f), 90.0f * (static_cast<float>(3.14159265358979323846) / 180.0f), 1.0f, 1000.0f);
	HRESULT const d3d7_set_transform_projection = d3d7_device->lpVtbl->SetTransform(d3d7_device, D3DTRANSFORMSTATE_PROJECTION, &d3d7_transform_projection);
	CHECK_RET(d3d7_set_transform_projection == D3D_OK || check_ret_hresult_failed(d3d7_set_transform_projection), false);

	*out_big_state_pointer = std::move(big_state_pointer);
	return true;
}

void big_state_deinit(big_state_t* const& big_state)
{
	static constexpr auto const wrapper = [](big_state_t const& big_state) -> bool
	{
		auto const dd7_free = mk::make_scope_exit([&](){ if(!big_state.m_dd7) return; ULONG const ref_count = big_state.m_dd7->lpVtbl->Release(big_state.m_dd7); CHECK_RET_V(ref_count == 0); });
		auto const d3d7_free = mk::make_scope_exit([&](){ if(!big_state.m_d3d7) return; ULONG const ref_count = big_state.m_d3d7->lpVtbl->Release(big_state.m_d3d7); CHECK_RET_V(ref_count == 1); });
		auto const dd7_primary_surface_free = mk::make_scope_exit([&](){ if(!big_state.m_dd7_primary_surface) return; ULONG const ref_count = big_state.m_dd7_primary_surface->lpVtbl->Release(big_state.m_dd7_primary_surface); CHECK_RET_V(ref_count == 0); });
		auto const dd7_clipper_free = mk::make_scope_exit([&](){ if(!big_state.m_dd7_clipper) return; ULONG const ref_count = big_state.m_dd7_clipper->lpVtbl->Release(big_state.m_dd7_clipper); CHECK_RET_V(ref_count == 1); });
		auto const dd7_back_surface_free = mk::make_scope_exit([&](){ if(!big_state.m_dd7_back_surface) return; ULONG const ref_count = big_state.m_dd7_back_surface->lpVtbl->Release(big_state.m_dd7_back_surface); CHECK_RET_V(ref_count == 0); });
		auto const d3d7_device_free = mk::make_scope_exit([&](){ if(!big_state.m_d3d7_device) return; ULONG const ref_count = big_state.m_d3d7_device->lpVtbl->Release(big_state.m_d3d7_device); CHECK_RET_V(ref_count == 0); });
		auto const d3d7_vertex_buffer_free = mk::make_scope_exit([&](){ if(!big_state.m_d3d7_vertex_buffer) return; ULONG const ref_count = big_state.m_d3d7_vertex_buffer->lpVtbl->Release(big_state.m_d3d7_vertex_buffer); /*CHECK_RET_V(ref_count == 0);*/ });
		auto const m_d3d7_vertex_buffer_indices_free = mk::make_scope_exit([&](){ if(!big_state.m_d3d7_vertex_buffer_indices) return; delete[] big_state.m_d3d7_vertex_buffer_indices; });
		return true;
	};
	bool const destroyed = wrapper(*big_state);
	CHECK_RET_V(destroyed);
}
