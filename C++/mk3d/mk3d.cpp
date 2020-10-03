#include "scope_exit.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define CINTERFACE
#define STRICT
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#undef ISOLATION_AWARE_ENABLED
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>


void check_ret_failed(char const* const& file, int const& line, char const* const& expr);
bool mk3d(int const argc, char const* const* const argv);
bool register_window_class(ATOM* const& out_main_window_class);
bool unregister_window_class(ATOM const& main_window_class);
bool create_main_window(ATOM const& main_window_class, HWND* const& out_main_window);
bool flip_surfaces(HWND const& hwnd, LPDIRECTDRAWSURFACE const& dd_primary_surface, LPDIRECTDRAWSURFACE const& dd_back_surface);
extern "C" LRESULT CALLBACK main_window_proc(_In_ HWND const hwnd, _In_ UINT const msg, _In_ WPARAM const w_param, _In_ LPARAM const l_param);
bool run_main_loop(int* const& out_exit_code);
D3DMATRIX const& d3d_matrix_identity();
D3DVECTOR operator-(D3DVECTOR const& a, D3DVECTOR const& b);
D3DVECTOR d3d_vector_normalize(D3DVECTOR const& v);
D3DVECTOR d3d_vector_cross_product(D3DVECTOR const& a, D3DVECTOR const& b);
float d3d_vector_dot_product(D3DVECTOR const& a, D3DVECTOR const& b);
float& d3d_matrix_index(D3DMATRIX& m, int const& row, int const& col);
float const& d3d_matrix_index(D3DMATRIX const& m, int const& row, int const& col);
D3DMATRIX d3d_matrix_translate(float const& dx, float const& dy, float const& dz);
D3DMATRIX d3d_matrix_rotate_z(float const& rads);
D3DMATRIX d3d_matrix_multiply(D3DMATRIX const& a, D3DMATRIX const& b);
D3DMATRIX d3d_matrix_view(D3DVECTOR const& from /* camera location */, D3DVECTOR const& at /* camera look-at target */, D3DVECTOR const& world_up /* world’s up, usually 0, 1, 0 */, float const& roll /* clockwise roll around viewing direction in radians */);
D3DMATRIX d3d_matrix_projection(float const& near_plane /* distance to near clipping plane */, float const& far_plane /* distance to far clipping plane */, float const& fov /* field of view angle, in radians */);
extern "C" void CALLBACK timer_proc(HWND const hwnd, UINT const msg, UINT_PTR const id, DWORD const tick_count);
bool render(DWORD const& tick_count);


#define CHECK_RET(X, R) do{ if(X){}else{ check_ret_failed(__FILE__, __LINE__, #X); return R; } }while(false)
#define CHECK_RET_V(X) do{ if(X){}else{ check_ret_failed(__FILE__, __LINE__, #X); std::exit(EXIT_FAILURE); } }while(false)


static constexpr int const s_width = 800;
static constexpr int const s_height = 800;


LPDIRECTDRAWSURFACE g_dd_primary_surface;
LPDIRECTDRAWSURFACE g_dd_back_surface;
bool g_move_forward = false;
bool g_move_backward = false;
bool g_move_left = false;
bool g_move_right = false;
bool g_move_down = false;
bool g_move_up = false;
bool g_rotate_yaw_left = false;
bool g_rotate_yaw_right = false;
bool g_rotate_pitch_down = false;
bool g_rotate_pitch_up = false;
bool g_rotate_roll_left = false;
bool g_rotate_roll_right = false;
DWORD g_tick_count;
HWND g_main_window;
LPDIRECT3DDEVICE2 g_d3d_device;
D3DMATRIX* g_d3d_transform_world;
D3DMATRIX* g_d3d_transform_view;
D3DMATRIX* g_d3d_transform_projection;


int main(int const argc, char const* const* const argv)
{
	auto something_wrong = mk::make_scope_exit([&](){ std::puts("Oh, no! Someting went wrong!"); });

	bool const ret = mk3d(argc, argv);
	CHECK_RET(ret, EXIT_FAILURE);

	something_wrong.reset();
	std::puts("We didn't crash! Great Success!");
	return EXIT_SUCCESS;
}


void check_ret_failed(char const* const& file, int const& line, char const* const& expr)
{
	std::printf("Failed in file `%s` at line %d with `%s`.\n", file, line, expr);
}

bool mk3d(int const argc, char const* const* const argv)
{
	static constexpr char const s_software_only[] = "/software";
	bool const software_only = [&]()
	{
		for(int i = 1; i != argc; ++i)
		{
			if(std::strcmp(argv[i], s_software_only) == 0)
			{
				return true;
			}
		}
		return false;
	}();

	ATOM main_window_class;
	bool const registered = register_window_class(&main_window_class);
	CHECK_RET(registered, false);
	auto const fn_unregister_window_class = mk::make_scope_exit([&](){ bool const unregistered = unregister_window_class(main_window_class); CHECK_RET_V(unregistered); });

	HWND main_window;
	bool const main_window_created = create_main_window(main_window_class, &main_window);
	CHECK_RET(main_window_created , false);
	g_main_window = main_window;

	HMODULE const ddraw = LoadLibraryW(L"ddraw.dll");
	CHECK_RET(ddraw != nullptr, false);
	auto const ddraw_unload = mk::make_scope_exit([&](){ BOOL const freed = FreeLibrary(ddraw); CHECK_RET_V(freed != 0); });

	auto const dd_create_proc = GetProcAddress(ddraw, "DirectDrawCreate");
	auto const DirectDrawCreate_fn = reinterpret_cast<decltype(&DirectDrawCreate)>(dd_create_proc);
	CHECK_RET(DirectDrawCreate_fn, false);

	LPDIRECTDRAW dd;
	GUID* const driver = software_only ? reinterpret_cast<GUID*>(DDCREATE_EMULATIONONLY) : static_cast<GUID*>(nullptr);
	HRESULT const dd_created = DirectDrawCreate_fn(driver, &dd, nullptr);
	CHECK_RET(dd_created == DD_OK, false);
	auto const dd_release = mk::make_scope_exit([&](){ ULONG const ref_count = dd->lpVtbl->Release(dd); });

	HRESULT const cooperation = dd->lpVtbl->SetCooperativeLevel(dd, main_window, DDSCL_NORMAL);
	CHECK_RET(cooperation == DD_OK, false);

	DDSURFACEDESC dd_primary_surface_description{};
	dd_primary_surface_description.dwSize = sizeof(dd_primary_surface_description);
	dd_primary_surface_description.dwFlags = DDSD_CAPS;
	dd_primary_surface_description.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	LPDIRECTDRAWSURFACE dd_primary_surface;
	HRESULT const dd_primary_surface_created = dd->lpVtbl->CreateSurface(dd, &dd_primary_surface_description, &dd_primary_surface, nullptr);
	CHECK_RET(dd_primary_surface_created == DD_OK, false);
	auto const dd_primary_surface_free = mk::make_scope_exit([&](){ ULONG const ref_count = dd_primary_surface->lpVtbl->Release(dd_primary_surface); });

	LPDIRECTDRAWCLIPPER dd_clipper;
	HRESULT const dd_clipper_created = dd->lpVtbl->CreateClipper(dd, 0, &dd_clipper, nullptr);
	CHECK_RET(dd_clipper_created == DD_OK, false);
	auto const dd_clipper_free = mk::make_scope_exit([&](){ ULONG const ref_count = dd_clipper->lpVtbl->Release(dd_clipper); });

	HRESULT const dd_clipper_set = dd_clipper->lpVtbl->SetHWnd(dd_clipper, 0, main_window);
	CHECK_RET(dd_clipper_set == DD_OK, false);

	HRESULT const dd_clipper_assigned = dd_primary_surface->lpVtbl->SetClipper(dd_primary_surface, dd_clipper);
	CHECK_RET(dd_clipper_assigned == DD_OK, false);

	DDSURFACEDESC dd_back_surface_description{};
	dd_back_surface_description.dwSize = sizeof(dd_back_surface_description);
	dd_back_surface_description.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	dd_back_surface_description.dwHeight = s_height;
	dd_back_surface_description.dwWidth = s_width;
	dd_back_surface_description.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE /*| (software_only ? 0 : DDSCAPS_VIDEOMEMORY)*/;
	LPDIRECTDRAWSURFACE dd_back_surface;
	HRESULT const dd_back_surface_created = dd->lpVtbl->CreateSurface(dd, &dd_back_surface_description, &dd_back_surface, nullptr);
	CHECK_RET(dd_back_surface_created == DD_OK, false);
	auto const dd_back_surface_free = mk::make_scope_exit([&](){ ULONG const ref_count = dd_back_surface->lpVtbl->Release(dd_back_surface); });

	g_dd_primary_surface = dd_primary_surface;
	g_dd_back_surface = dd_back_surface;

	LPDIRECT3D2 d3d;
	HRESULT const casted = dd->lpVtbl->QueryInterface(dd, IID_IDirect3D2, reinterpret_cast<void**>(&d3d));
	CHECK_RET(casted == S_OK, false);
	auto const d3d_free = mk::make_scope_exit([&](){ ULONG const ref_count = d3d->lpVtbl->Release(d3d); });

	D3DFINDDEVICESEARCH search{};
	search.dwSize = sizeof(search);
	search.dwFlags = D3DFDS_HARDWARE;
	search.bHardware = software_only ? FALSE : TRUE;
	D3DFINDDEVICERESULT result{};
	result.dwSize = sizeof(result);
	HRESULT const found = d3d->lpVtbl->FindDevice(d3d, &search, &result);
	CHECK_RET(found == D3D_OK, false);

	LPDIRECT3DDEVICE2 d3d_device;
	HRESULT const created = d3d->lpVtbl->CreateDevice(d3d, result.guid, dd_back_surface, &d3d_device);
	CHECK_RET(created == D3D_OK, false);
	auto const d3d_device_free = mk::make_scope_exit([&](){ ULONG const ref_count = d3d_device->lpVtbl->Release(d3d_device); });
	g_d3d_device = d3d_device;

	LPDIRECT3DVIEWPORT2 d3d_viewport;
	HRESULT const d3d_viewport_created = d3d->lpVtbl->CreateViewport(d3d, &d3d_viewport, nullptr);
	CHECK_RET(d3d_viewport_created == D3D_OK, false);
	auto const d3d_viewport_free = mk::make_scope_exit([&](){ ULONG const ref_count = d3d_viewport->lpVtbl->Release(d3d_viewport); });

	HRESULT const d3d_viewport_added = d3d_device->lpVtbl->AddViewport(d3d_device, d3d_viewport);
	CHECK_RET(d3d_viewport_added == D3D_OK, false);

	D3DVIEWPORT d3d_viewport_settings{};
	d3d_viewport_settings.dwSize = sizeof(d3d_viewport_settings);
	d3d_viewport_settings.dwWidth = s_width;
	d3d_viewport_settings.dwHeight = s_height;
	d3d_viewport_settings.dvScaleX = s_width / 2.0f;
	d3d_viewport_settings.dvScaleY = s_height / 2.0f;
	d3d_viewport_settings.dvMaxX = 1.0f;
	d3d_viewport_settings.dvMaxY = 1.0f;
	HRESULT const d3d_viewport_set = d3d_viewport->lpVtbl->SetViewport(d3d_viewport, &d3d_viewport_settings);
	CHECK_RET(d3d_viewport_set == D3D_OK, false);

	HRESULT const d3d_viewport_set_current = d3d_device->lpVtbl->SetCurrentViewport(d3d_device, d3d_viewport);
	CHECK_RET(d3d_viewport_set_current == D3D_OK, false);

	D3DMATRIX d3d_transform_world = d3d_matrix_identity();
	g_d3d_transform_world = &d3d_transform_world;
	HRESULT const d3d_transformed_world = d3d_device->lpVtbl->SetTransform(d3d_device, D3DTRANSFORMSTATE_WORLD, g_d3d_transform_world);
	CHECK_RET(d3d_transformed_world == D3D_OK, false);

	D3DMATRIX d3d_transform_view = d3d_matrix_view(D3DVECTOR{0.0f, 0.0f, 0.0f}, D3DVECTOR{0.0f, 0.0f, 1.0f}, D3DVECTOR{0.0f, 1.0f, 0.0f}, 0.0f);
	g_d3d_transform_view = &d3d_transform_view;
	HRESULT const d3d_transformed_view = d3d_device->lpVtbl->SetTransform(d3d_device, D3DTRANSFORMSTATE_VIEW, g_d3d_transform_view);
	CHECK_RET(d3d_transformed_view == D3D_OK, false);

	D3DMATRIX d3d_transform_projection =  d3d_matrix_projection(1.0f, 1000.0f, 60.0f * (static_cast<float>(3.14159265358979323846) / 180.0f));
	g_d3d_transform_projection = &d3d_transform_projection;
	HRESULT const d3d_transformed_projection = d3d_device->lpVtbl->SetTransform(d3d_device, D3DTRANSFORMSTATE_PROJECTION, g_d3d_transform_projection);
	CHECK_RET(d3d_transformed_projection == D3D_OK, false);

	g_tick_count = GetTickCount();
	UINT_PTR const timer = SetTimer(main_window, 0, 1000 / 30, timer_proc);
	CHECK_RET(timer != 0, false);

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

	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = 0;
	wc.lpfnWndProc = &main_window_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = self;
	wc.hIcon = LoadIconW(nullptr, reinterpret_cast<wchar_t const*>(IDI_APPLICATION));
	wc.hCursor = LoadCursorW(nullptr, reinterpret_cast<wchar_t const*>(IDC_ARROW));
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = L"main_window";
	wc.hIconSm = LoadIconW(nullptr, reinterpret_cast<wchar_t const*>(IDI_APPLICATION));

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

	RECT r;
	r.left = 0;
	r.top = 0;
	r.right = s_width;
	r.bottom = s_height;
	BOOL const adjusted = AdjustWindowRectEx(&r, style, FALSE, style_ex);
	CHECK_RET(adjusted != 0, false);

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
		r.right - r.left,
		r.bottom - r.top,
		nullptr,
		nullptr,
		self,
		nullptr
	);
	CHECK_RET(main_window != nullptr, false);

	*out_main_window = main_window;
	return true;
}

bool flip_surfaces(HWND const& hwnd, LPDIRECTDRAWSURFACE const& dd_primary_surface, LPDIRECTDRAWSURFACE const& dd_back_surface)
{
#if 0
	RECT r;
	BOOL const got_rect = GetClientRect(hwnd, &r);
	CHECK_RET(got_rect != 0, false);

	POINT p1;
	p1.x = r.left;
	p1.y = r.top;
	POINT p2;
	p2.x = r.right;
	p2.y = r.bottom;

	BOOL const converted1 = ClientToScreen(hwnd, &p1);
	CHECK_RET(converted1 != 0, false);
	BOOL const converted2 = ClientToScreen(hwnd, &p2);
	CHECK_RET(converted2 != 0, false);

	r.left = p1.x;
	r.top = p1.y;
	r.right = p2.x;
	r.bottom = p2.y;
#else
	RECT r;
	BOOL const got_rect = GetClientRect(hwnd, &r);
	CHECK_RET(got_rect != 0, false);

	POINT p{};
	BOOL const converted = ClientToScreen(hwnd, &p);
	CHECK_RET(converted != 0, false);

	BOOL const offset = OffsetRect(&r, p.x, p.y);
	CHECK_RET(offset != 0, false);
#endif

	HRESULT const blted = dd_primary_surface->lpVtbl->Blt(dd_primary_surface, &r, dd_back_surface, nullptr, DDBLT_WAIT, nullptr);
	CHECK_RET(blted == DD_OK, false);

	return true;
}

extern "C" LRESULT CALLBACK main_window_proc(_In_ HWND const hwnd, _In_ UINT const msg, _In_ WPARAM const w_param, _In_ LPARAM const l_param)
{
	switch(msg)
	{
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC const dc = BeginPaint(hwnd, &ps);
			CHECK_RET_V(dc != nullptr);
			auto const end_paint = mk::make_scope_exit([&](){ BOOL const ended = EndPaint(hwnd, &ps); CHECK_RET_V(ended != 0); });

			bool const flipped = flip_surfaces(hwnd, g_dd_primary_surface, g_dd_back_surface);
			CHECK_RET_V(flipped);

			//BOOL const validated = ValidateRect(hwnd, nullptr);
			//CHECK_RET_V(validated != 0);
		}
		break;
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			bool const is_pressed = msg == WM_KEYDOWN;
			static constexpr WPARAM const s_move_forward[] = {'W', VK_UP};
			static constexpr WPARAM const s_move_backward[] = {'S', VK_DOWN};
			static constexpr WPARAM const s_move_left[] = {'A', VK_LEFT};
			static constexpr WPARAM const s_move_right[] = {'D', VK_RIGHT};
			static constexpr WPARAM const s_move_down[] = {'Q', VK_NEXT};
			static constexpr WPARAM const s_move_up[] = {'E', VK_PRIOR};
			static constexpr WPARAM const s_rotate_yaw_left[] = {'J'};
			static constexpr WPARAM const s_rotate_yaw_right[] = {'L'};
			static constexpr WPARAM const s_rotate_pitch_down[] = {'K'};
			static constexpr WPARAM const s_rotate_pitch_up[] = {'I'};
			static constexpr WPARAM const s_rotate_roll_left[] = {'U'};
			static constexpr WPARAM const s_rotate_roll_right[] = {'O'};
			static constexpr WPARAM const s_reset[] = {'R', VK_NUMPAD5};
			if(std::find(std::cbegin(s_move_forward), std::cend(s_move_forward), w_param) != std::cend(s_move_forward)) g_move_forward = is_pressed;
			if(std::find(std::cbegin(s_move_backward), std::cend(s_move_backward), w_param) != std::cend(s_move_backward)) g_move_backward = is_pressed;
			if(std::find(std::cbegin(s_move_left), std::cend(s_move_left), w_param) != std::cend(s_move_left)) g_move_left = is_pressed;
			if(std::find(std::cbegin(s_move_right), std::cend(s_move_right), w_param) != std::cend(s_move_right)) g_move_right = is_pressed;
			if(std::find(std::cbegin(s_move_down), std::cend(s_move_down), w_param) != std::cend(s_move_down)) g_move_down = is_pressed;
			if(std::find(std::cbegin(s_move_up), std::cend(s_move_up), w_param) != std::cend(s_move_up)) g_move_up = is_pressed;
			if(std::find(std::cbegin(s_rotate_yaw_left), std::cend(s_rotate_yaw_left), w_param) != std::cend(s_rotate_yaw_left)) g_rotate_yaw_left = is_pressed;
			if(std::find(std::cbegin(s_rotate_yaw_right), std::cend(s_rotate_yaw_right), w_param) != std::cend(s_rotate_yaw_right)) g_rotate_yaw_right = is_pressed;
			if(std::find(std::cbegin(s_rotate_pitch_down), std::cend(s_rotate_pitch_down), w_param) != std::cend(s_rotate_pitch_down)) g_rotate_pitch_down = is_pressed;
			if(std::find(std::cbegin(s_rotate_pitch_up), std::cend(s_rotate_pitch_up), w_param) != std::cend(s_rotate_pitch_up)) g_rotate_pitch_up = is_pressed;
			if(std::find(std::cbegin(s_rotate_roll_left), std::cend(s_rotate_roll_left), w_param) != std::cend(s_rotate_roll_left)) g_rotate_roll_left = is_pressed;
			if(std::find(std::cbegin(s_rotate_roll_right), std::cend(s_rotate_roll_right), w_param) != std::cend(s_rotate_roll_right)) g_rotate_roll_right = is_pressed;
			if(is_pressed && std::find(std::cbegin(s_reset), std::cend(s_reset), w_param) != std::cend(s_reset)) *g_d3d_transform_view = d3d_matrix_view(D3DVECTOR{0.0f, 0.0f, 0.0f}, D3DVECTOR{0.0f, 0.0f, 1.0f}, D3DVECTOR{0.0f, 1.0f, 0.0f}, 0.0f);
		}
		break;
		case WM_CLOSE:
		{
			PostQuitMessage(0);
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
		BOOL const got = GetMessageW(&msg, nullptr, 0, 0);
		CHECK_RET(got != -1, false);
		if(got == 0)
		{
			CHECK_RET(msg.hwnd == nullptr, false);
			CHECK_RET(msg.message == WM_QUIT, false);
			int const exit_code = static_cast<int>(msg.wParam);
			*out_exit_code = exit_code;
			return true;
		}
		BOOL const translated = TranslateMessage(&msg);
		LRESULT const dispatched = DispatchMessageW(&msg);
	}
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

D3DVECTOR d3d_vector_normalize(D3DVECTOR const& v)
{
	float const magnitude = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
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

D3DMATRIX d3d_matrix_view(D3DVECTOR const& from /* camera location */, D3DVECTOR const& at /* camera look-at target */, D3DVECTOR const& world_up /* world’s up, usually 0, 1, 0 */, float const& roll /* clockwise roll around viewing direction in radians */)
{
	D3DVECTOR const view_dir = d3d_vector_normalize(at - from);
	D3DVECTOR right = d3d_vector_cross_product(world_up, view_dir);
	D3DVECTOR up = d3d_vector_cross_product(view_dir, right);
	right = d3d_vector_normalize(right);
	up = d3d_vector_normalize(up);

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
	d3d_matrix_index(view, 3, 0) = -d3d_vector_dot_product(right, from);
	d3d_matrix_index(view, 3, 1) = -d3d_vector_dot_product(up, from);
	d3d_matrix_index(view, 3, 2) = -d3d_vector_dot_product(view_dir, from);

	if(roll != 0.0f)
	{
		view = d3d_matrix_multiply(d3d_matrix_rotate_z(-roll), view); 
	}
	
	return view;
}

D3DMATRIX d3d_matrix_projection(float const& near_plane /* distance to near clipping plane */, float const& far_plane /* distance to far clipping plane */, float const& fov /* field of view angle, in radians */)
{
	float const c = std::cos(fov * 0.5f);
	float const s = std::sin(fov * 0.5f);
	float const q = s / (1.0f - near_plane / far_plane);
	D3DMATRIX ret{};
	d3d_matrix_index(ret, 0, 0) = c;
	d3d_matrix_index(ret, 1, 1) = c;
	d3d_matrix_index(ret, 2, 2) = q;
	d3d_matrix_index(ret, 3, 2) = -q * near_plane;
	d3d_matrix_index(ret, 2, 3) = s;
	return ret;
}

extern "C" void CALLBACK timer_proc(HWND const hwnd, UINT const msg, UINT_PTR const id, DWORD const tick_count)
{
	bool const rendered = render(tick_count);
	CHECK_RET_V(rendered);
}

bool render(DWORD const& tick_count)
{
	DWORD const elapsed_u = tick_count - g_tick_count;
	g_tick_count = tick_count;
	float const elapsed_f = static_cast<float>(elapsed_u);
	float const move_speed = elapsed_f / 100.0f;
	float const rotate_speed = elapsed_f / 500.0f;

	if(g_move_forward) *g_d3d_transform_view = d3d_matrix_multiply(d3d_matrix_translate(0.0f, 0.0f, -1.0f * move_speed), *g_d3d_transform_view);
	if(g_move_backward) *g_d3d_transform_view = d3d_matrix_multiply(d3d_matrix_translate(0.0f, 0.0f, +1.0f * move_speed), *g_d3d_transform_view);
	if(g_move_left) *g_d3d_transform_view = d3d_matrix_multiply(d3d_matrix_translate(+1.0f * move_speed, 0.0f, 0.0f), *g_d3d_transform_view);
	if(g_move_right) *g_d3d_transform_view = d3d_matrix_multiply(d3d_matrix_translate(-1.0f * move_speed, 0.0f, 0.0f), *g_d3d_transform_view);
	if(g_move_down) *g_d3d_transform_view = d3d_matrix_multiply(d3d_matrix_translate(0.0f, +1.0f * move_speed, 0.0f), *g_d3d_transform_view);
	if(g_move_up) *g_d3d_transform_view = d3d_matrix_multiply(d3d_matrix_translate(0.0f, -1.0f * move_speed, 0.0f), *g_d3d_transform_view);
	if(g_rotate_yaw_left) *g_d3d_transform_view = d3d_matrix_multiply(d3d_matrix_rotate_y(-1.0f * rotate_speed), *g_d3d_transform_view);
	if(g_rotate_yaw_right) *g_d3d_transform_view = d3d_matrix_multiply(d3d_matrix_rotate_y(+1.0f * rotate_speed), *g_d3d_transform_view);
	if(g_rotate_pitch_down) *g_d3d_transform_view = d3d_matrix_multiply(d3d_matrix_rotate_x(+1.0f * rotate_speed), *g_d3d_transform_view);
	if(g_rotate_pitch_up) *g_d3d_transform_view = d3d_matrix_multiply(d3d_matrix_rotate_x(-1.0f * rotate_speed), *g_d3d_transform_view);
	if(g_rotate_roll_left) *g_d3d_transform_view = d3d_matrix_multiply(d3d_matrix_rotate_z(+1.0f * rotate_speed), *g_d3d_transform_view);
	if(g_rotate_roll_right) *g_d3d_transform_view = d3d_matrix_multiply(d3d_matrix_rotate_z(-1.0f * rotate_speed), *g_d3d_transform_view);

	HRESULT const d3d_transformed_view = g_d3d_device->lpVtbl->SetTransform(g_d3d_device, D3DTRANSFORMSTATE_VIEW, g_d3d_transform_view);
	CHECK_RET(d3d_transformed_view == D3D_OK, false);

	DDBLTFX dd_bltfx{};
	dd_bltfx.dwSize = sizeof(dd_bltfx);
	HRESULT const cleared = g_dd_back_surface->lpVtbl->Blt(g_dd_back_surface, nullptr, nullptr, nullptr, DDBLT_WAIT | DDBLT_COLORFILL, &dd_bltfx);
	CHECK_RET(cleared == DD_OK, false);

	{
		HRESULT const started = g_d3d_device->lpVtbl->BeginScene(g_d3d_device);
		CHECK_RET(started == D3D_OK, false);
		auto const end_scene = mk::make_scope_exit([&](){ HRESULT const ended = g_d3d_device->lpVtbl->EndScene(g_d3d_device); CHECK_RET_V(ended == D3D_OK); });

		//D3DTLVERTEX v[3];
		//v[0] = D3DTLVERTEX{160.0f, 50.0f, 0.0f, 1.0f, D3DRGB(1, 0, 0), D3DRGB(0, 0, 0), 0.0f, 0.0f};
		//v[1] = D3DTLVERTEX{240.0f, 200.0f, 0.0f, 1.0f, D3DRGB(0, 1, 0), D3DRGB(0, 0, 0), 0.0f, 0.0f};
		//v[2] = D3DTLVERTEX{80.0f, 200.0f, 0.0f, 1.0f, D3DRGB(0, 0, 1), D3DRGB(0, 0, 0), 0.0f, 0.0f};
		//HRESULT const d3d_drawn = d3d_device->lpVtbl->DrawPrimitive(d3d_device, D3DPT_TRIANGLELIST, D3DVT_TLVERTEX, reinterpret_cast<void*>(v), 3, nullptr);
		//CHECK_RET(d3d_drawn == D3D_OK, false);

		//D3DLVERTEX v[3];
		//v[0] = D3DLVERTEX{0.0f, 2.0f, 10.0f, 0, D3DRGB(1, 0, 0), D3DRGB(1, 0, 0), 0.0f, 0.0f};
		//v[1] = D3DLVERTEX{2.0f, -2.0f, 10.0f, 0, D3DRGB(0, 1, 0), D3DRGB(1, 0, 0), 0.0f, 0.0f};
		//v[2] = D3DLVERTEX{-2.0f, -2.0f, 10.0f, 0, D3DRGB(0, 0, 1), D3DRGB(1, 0, 0), 0.0f, 0.0f};
		//HRESULT const d3d_drawn = g_d3d_device->lpVtbl->DrawPrimitive(g_d3d_device, D3DPT_TRIANGLELIST, D3DVT_LVERTEX, reinterpret_cast<void*>(v), std::size(v), 0);
		//CHECK_RET(d3d_drawn == D3D_OK, false);

		#if 1
		D3DLVERTEX v[36];
		// Front
		v[0] = D3DLVERTEX{-1.0f, -1.0f, -1.0f, 0, D3DRGB(1, 0, 0), D3DRGB(1, 0, 0), 0.0f, 0.0f};
		v[1] = D3DLVERTEX{-1.0f, +1.0f, -1.0f, 0, D3DRGB(1, 0, 0), D3DRGB(1, 0, 0), 0.0f, 0.0f};
		v[2] = D3DLVERTEX{+1.0f, +1.0f, -1.0f, 0, D3DRGB(1, 0, 0), D3DRGB(1, 0, 0), 0.0f, 0.0f};
		v[3] = D3DLVERTEX{+1.0f, -1.0f, -1.0f, 0, D3DRGB(1, 0, 0), D3DRGB(1, 0, 0), 0.0f, 0.0f};
		v[4] = v[4 - 4];
		v[5] = v[5 - 3];
		// Right
		v[6] = D3DLVERTEX{+1.0f, -1.0f, -1.0f, 0, D3DRGB(0, 1, 0), D3DRGB(0, 1, 0), 0.0f, 0.0f};
		v[7] = D3DLVERTEX{+1.0f, +1.0f, -1.0f, 0, D3DRGB(0, 1, 0), D3DRGB(0, 1, 0), 0.0f, 0.0f};
		v[8] = D3DLVERTEX{+1.0f, +1.0f, +1.0f, 0, D3DRGB(0, 1, 0), D3DRGB(0, 1, 0), 0.0f, 0.0f};
		v[9] = D3DLVERTEX{+1.0f, -1.0f, +1.0f, 0, D3DRGB(0, 1, 0), D3DRGB(0, 1, 0), 0.0f, 0.0f};
		v[10] = v[10 - 4];
		v[11] = v[11 - 3];
		// Back
		v[12] = D3DLVERTEX{+1.0f, -1.0f, +1.0f, 0, D3DRGB(0, 0, 1), D3DRGB(0, 0, 1), 0.0f, 0.0f};
		v[13] = D3DLVERTEX{+1.0f, +1.0f, +1.0f, 0, D3DRGB(0, 0, 1), D3DRGB(0, 0, 1), 0.0f, 0.0f};
		v[14] = D3DLVERTEX{-1.0f, +1.0f, +1.0f, 0, D3DRGB(0, 0, 1), D3DRGB(0, 0, 1), 0.0f, 0.0f};
		v[15] = D3DLVERTEX{-1.0f, -1.0f, +1.0f, 0, D3DRGB(0, 0, 1), D3DRGB(0, 0, 1), 0.0f, 0.0f};
		v[16] = v[16 - 4];
		v[17] = v[17 - 3];
		// Left
		v[18] = D3DLVERTEX{-1.0f, -1.0f, +1.0f, 0, D3DRGB(1, 1, 0), D3DRGB(1, 1, 0), 0.0f, 0.0f};
		v[19] = D3DLVERTEX{-1.0f, +1.0f, +1.0f, 0, D3DRGB(1, 1, 0), D3DRGB(1, 1, 0), 0.0f, 0.0f};
		v[20] = D3DLVERTEX{-1.0f, +1.0f, -1.0f, 0, D3DRGB(1, 1, 0), D3DRGB(1, 1, 0), 0.0f, 0.0f};
		v[21] = D3DLVERTEX{-1.0f, -1.0f, -1.0f, 0, D3DRGB(1, 1, 0), D3DRGB(1, 1, 0), 0.0f, 0.0f};
		v[22] = v[22 - 4];
		v[23] = v[23 - 3];
		// Bottom
		v[24] = D3DLVERTEX{-1.0f, -1.0f, +1.0f, 0, D3DRGB(1, 0, 1), D3DRGB(1, 0, 1), 0.0f, 0.0f};
		v[25] = D3DLVERTEX{-1.0f, -1.0f, -1.0f, 0, D3DRGB(1, 0, 1), D3DRGB(1, 0, 1), 0.0f, 0.0f};
		v[26] = D3DLVERTEX{+1.0f, -1.0f, -1.0f, 0, D3DRGB(1, 0, 1), D3DRGB(1, 0, 1), 0.0f, 0.0f};
		v[27] = D3DLVERTEX{+1.0f, -1.0f, +1.0f, 0, D3DRGB(1, 0, 1), D3DRGB(1, 0, 1), 0.0f, 0.0f};
		v[28] = v[28 - 4];
		v[29] = v[29 - 3];
		// Top
		v[30] = D3DLVERTEX{-1.0f, +1.0f, -1.0f, 0, D3DRGB(0, 1, 1), D3DRGB(0, 1, 1), 0.0f, 0.0f};
		v[31] = D3DLVERTEX{-1.0f, +1.0f, +1.0f, 0, D3DRGB(0, 1, 1), D3DRGB(0, 1, 1), 0.0f, 0.0f};
		v[32] = D3DLVERTEX{+1.0f, +1.0f, +1.0f, 0, D3DRGB(0, 1, 1), D3DRGB(0, 1, 1), 0.0f, 0.0f};
		v[33] = D3DLVERTEX{+1.0f, +1.0f, -1.0f, 0, D3DRGB(0, 1, 1), D3DRGB(0, 1, 1), 0.0f, 0.0f};
		v[34] = v[34 - 4];
		v[35] = v[35 - 3];
		#else
		D3DLVERTEX v[36];
		// Front
		v[0] = D3DLVERTEX{+1.0f, -1.0f, -1.0f, 0, D3DRGB(1, 0, 0), D3DRGB(1, 0, 0), 0.0f, 0.0f};
		v[1] = D3DLVERTEX{+1.0f, +1.0f, -1.0f, 0, D3DRGB(1, 0, 0), D3DRGB(1, 0, 0), 0.0f, 0.0f};
		v[2] = D3DLVERTEX{-1.0f, +1.0f, -1.0f, 0, D3DRGB(1, 0, 0), D3DRGB(1, 0, 0), 0.0f, 0.0f};
		v[3] = D3DLVERTEX{-1.0f, -1.0f, -1.0f, 0, D3DRGB(1, 0, 0), D3DRGB(1, 0, 0), 0.0f, 0.0f};
		v[4] = v[4 - 4];
		v[5] = v[5 - 3];
		// Right
		v[6] = D3DLVERTEX{+1.0f, -1.0f, +1.0f, 0, D3DRGB(0, 1, 0), D3DRGB(0, 1, 0), 0.0f, 0.0f};
		v[7] = D3DLVERTEX{+1.0f, +1.0f, +1.0f, 0, D3DRGB(0, 1, 0), D3DRGB(0, 1, 0), 0.0f, 0.0f};
		v[8] = D3DLVERTEX{+1.0f, +1.0f, -1.0f, 0, D3DRGB(0, 1, 0), D3DRGB(0, 1, 0), 0.0f, 0.0f};
		v[9] = D3DLVERTEX{+1.0f, -1.0f, -1.0f, 0, D3DRGB(0, 1, 0), D3DRGB(0, 1, 0), 0.0f, 0.0f};
		v[10] = v[10 - 4];
		v[11] = v[11 - 3];
		// Back
		v[12] = D3DLVERTEX{-1.0f, -1.0f, +1.0f, 0, D3DRGB(0, 0, 1), D3DRGB(0, 0, 1), 0.0f, 0.0f};
		v[13] = D3DLVERTEX{-1.0f, +1.0f, +1.0f, 0, D3DRGB(0, 0, 1), D3DRGB(0, 0, 1), 0.0f, 0.0f};
		v[14] = D3DLVERTEX{+1.0f, +1.0f, +1.0f, 0, D3DRGB(0, 0, 1), D3DRGB(0, 0, 1), 0.0f, 0.0f};
		v[15] = D3DLVERTEX{+1.0f, -1.0f, +1.0f, 0, D3DRGB(0, 0, 1), D3DRGB(0, 0, 1), 0.0f, 0.0f};
		v[16] = v[16 - 4];
		v[17] = v[17 - 3];
		// Left
		v[18] = D3DLVERTEX{-1.0f, -1.0f, -1.0f, 0, D3DRGB(1, 1, 0), D3DRGB(1, 1, 0), 0.0f, 0.0f};
		v[19] = D3DLVERTEX{-1.0f, +1.0f, -1.0f, 0, D3DRGB(1, 1, 0), D3DRGB(1, 1, 0), 0.0f, 0.0f};
		v[20] = D3DLVERTEX{-1.0f, +1.0f, +1.0f, 0, D3DRGB(1, 1, 0), D3DRGB(1, 1, 0), 0.0f, 0.0f};
		v[21] = D3DLVERTEX{-1.0f, -1.0f, +1.0f, 0, D3DRGB(1, 1, 0), D3DRGB(1, 1, 0), 0.0f, 0.0f};
		v[22] = v[22 - 4];
		v[23] = v[23 - 3];
		// Bottom
		v[24] = D3DLVERTEX{+1.0f, -1.0f, +1.0f, 0, D3DRGB(1, 0, 1), D3DRGB(1, 0, 1), 0.0f, 0.0f};
		v[25] = D3DLVERTEX{+1.0f, -1.0f, -1.0f, 0, D3DRGB(1, 0, 1), D3DRGB(1, 0, 1), 0.0f, 0.0f};
		v[26] = D3DLVERTEX{-1.0f, -1.0f, -1.0f, 0, D3DRGB(1, 0, 1), D3DRGB(1, 0, 1), 0.0f, 0.0f};
		v[27] = D3DLVERTEX{-1.0f, -1.0f, +1.0f, 0, D3DRGB(1, 0, 1), D3DRGB(1, 0, 1), 0.0f, 0.0f};
		v[28] = v[28 - 4];
		v[29] = v[29 - 3];
		// Top
		v[30] = D3DLVERTEX{+1.0f, +1.0f, -1.0f, 0, D3DRGB(0, 1, 1), D3DRGB(0, 1, 1), 0.0f, 0.0f};
		v[31] = D3DLVERTEX{+1.0f, +1.0f, +1.0f, 0, D3DRGB(0, 1, 1), D3DRGB(0, 1, 1), 0.0f, 0.0f};
		v[32] = D3DLVERTEX{-1.0f, +1.0f, +1.0f, 0, D3DRGB(0, 1, 1), D3DRGB(0, 1, 1), 0.0f, 0.0f};
		v[33] = D3DLVERTEX{-1.0f, +1.0f, -1.0f, 0, D3DRGB(0, 1, 1), D3DRGB(0, 1, 1), 0.0f, 0.0f};
		v[34] = v[34 - 4];
		v[35] = v[35 - 3];
		#endif

		HRESULT const d3d_drawn = g_d3d_device->lpVtbl->DrawPrimitive(g_d3d_device, D3DPT_TRIANGLELIST, D3DVT_LVERTEX, reinterpret_cast<void*>(v), std::size(v), 0);
		CHECK_RET(d3d_drawn == D3D_OK, false);
	}

	BOOL const invalidated = InvalidateRect(g_main_window, nullptr, FALSE);
	CHECK_RET(invalidated != 0, false);
	//BOOL const updated = UpdateWindow(g_main_window);
	//CHECK_RET(updated != 0, false);

	return true;
}
