/*
The MIT License (MIT)

Copyright (c) 2025 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <cstdint>
#include <algorithm>
#include <utility>
#include <cmath>
#include <limits>
#include <bit>
#include <concepts>
#include <memory>
#include <vector>
#include <set>
#include <tuple>
#include <string>
#include <string_view>
#include <cassert>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32")

#include "plugin2.h"
#include "logger2.h"


////////////////////////////////
// plugin info.
////////////////////////////////
#define PLUGIN_NAME		L"TLショトカ移動2"
#define PLUGIN_VERSION	"v1.21-beta5 (for beta25)"
#define PLUGIN_AUTHOR	"sigma-axis"
#define LEAST_VER_STR	"version 2.0beta25"
constexpr uint32_t least_ver_num = 2002500;


////////////////////////////////
// globals.
////////////////////////////////
constinit HMODULE dll_hinst = nullptr;
constinit LOG_HANDLE* logger = nullptr;
constinit EDIT_HANDLE* edit_handle = nullptr;
constinit struct Settings {
#define decl_prop(type, name, def)	\
	type name = name##_def; \
	constexpr static type name##_def = (def); \
	constexpr static std::wstring_view name##_key = L## #name
#define decl_prop_minmax(type, name, def, min, max)	\
	decl_prop(type, name, def); \
	constexpr static type name##_min = (min), name##_max = (max);

	struct {
		decl_prop_minmax(double, page_rate, 0.25, 0.01, 1.0);
		decl_prop_minmax(int, bpm_grid_div, 4, 1, 16);
		decl_prop(bool, suppress_shift, false);
		decl_prop(bool, focus_follows, false);

		constexpr static std::wstring_view section = L"search";
	} search{};

	struct {
		decl_prop_minmax(int, queue_size, 1 << 8, 1 << 4, 1 << 14);
		decl_prop_minmax(double, polling_period, 2.0, 0.5, 60.0);

		constexpr static std::wstring_view section = L"cursor_undo";
	} cursor_undo;

#undef decl_prop_minmax
#undef decl_prop

private:
	std::wstring ini_path() const
	{
		wchar_t buf[MAX_PATH];
		::GetModuleFileNameW(dll_hinst, buf, std::size(buf));
		std::wstring path{ buf };
		auto pos = path.rfind(L'.');
		if (pos != path.npos) {
			path = path.substr(0, pos) + L".ini";
		}
		else {
			path += L".ini";
		}
		return path;
	}
	template<size_t N>
	std::wstring read_ini_string(std::wstring_view const& section, std::wstring_view const& key, std::wstring_view const& default_value, std::wstring const& path) const
	{
		wchar_t buf[N];
		auto len = ::GetPrivateProfileStringW(
			section.data(), key.data(), default_value.data(),
			buf, static_cast<DWORD>(std::size(buf)),
			path.data());
		return std::wstring{ buf, len };
	}
	template<size_t N>
	void write_ini_value(std::wstring_view const& section, std::wstring_view const& key, auto val, wchar_t const* fmt, std::wstring const& path) const
	{
		wchar_t buf[N];
		::swprintf_s(buf, fmt, val);
		::WritePrivateProfileStringW(section.data(), key.data(), buf, path.c_str());
	}

public:
	void load()
	{
		// load from ini file.
		auto path = ini_path();
	#define read_double(sec, key) \
		do { \
			auto raw = read_ini_string<32>(sec.section, sec.key##_key, L"", path); \
			wchar_t* e; double v; \
			sec.key = raw.size() > 0 && (v = std::wcstod(raw.c_str(), &e), *e == L'\0') ? v : sec.key##_def; \
			sec.key = std::clamp(sec.key, sec.key##_min, sec.key##_max); \
		} while (false)
	#define read_int(sec, key) \
		do { \
			sec.key = ::GetPrivateProfileIntW(sec.section.data(), sec.key##_key.data(), sec.key##_def, path.c_str()); \
			sec.key = std::clamp(sec.key, sec.key##_min, sec.key##_max); \
		} while (false)
	#define read_bool(sec, key) sec.key = ::GetPrivateProfileIntW(sec.section.data(), sec.key##_key.data(), sec.key##_def ? 1 : 0, path.c_str()) != 0

		read_double	(search, page_rate);
		read_int	(search, bpm_grid_div);
		read_bool	(search, suppress_shift);
		read_bool	(search, focus_follows);

		read_int	(cursor_undo, queue_size);
		read_double	(cursor_undo, polling_period);

	#undef read_bool
	#undef read_int
	#undef read_double

		// logging.
		logger->verbose(logger, L"Settings loaded.");
	}

	void save() const
	{
		// save to ini file.
		auto path = ini_path();
	#define write_val(sec, key, proj, fmt) write_ini_value<16>(sec.section, sec.key##_key, proj(sec.key), fmt, path)
	#define write_int(sec, key) write_val(sec, key, , L"%d")
	#define write_bool(sec, key) write_val(sec, key, [](auto x) { return x ? 1 : 0; } , L"%d")

		write_val	(search, page_rate, , L"%.3f");
		write_int	(search, bpm_grid_div);
		write_bool	(search, suppress_shift);
		write_bool	(search, focus_follows);

		// even though cursor_undo can't change during runtime, save it
		// so missing .ini file will be fully generated.
		write_int	(cursor_undo, queue_size);
		write_val	(cursor_undo, polling_period, , L"%.3f");

	#undef write_bool
	#undef write_int
	#undef write_val

		// logging.
		logger->verbose(logger, L"Settings saved.");
	}
} settings{};

template<class T>
struct undo_history {
	size_t size() const { return n; }
	size_t max_size() const { return N; }
	void forward(T v)
	{
		n++;
		if (n > N) {
			i0 = (i0 + n) % N;
			n = N;
		}
		size_t i = (i0 + n - 1) % N;
		vals[i] = v;
		m = n;
	}
	bool check_forward(T v)
	{
		if (v == peek()) return false;
		forward(v);
		return true;
	}
	bool repush(T& v)
	{
		if (m <= n) return false;
		n++;
		size_t i = (i0 + n - 1) % N;
		v = vals[i];
		return true;
	}
	T peek() const
	{
		if (n == 0) return T{};
		size_t i = (i0 + n - 1) % N;
		return vals[i];
	}
	T backward()
	{
		if (n == 0) return T{};
		else if (n > 1) n--;
		size_t i = (i0 + n - 1) % N;
		return vals[i];
	}
	void clear(T v)
	{
		i0 = n = m = 0;
		forward(v);
	}
	constexpr undo_history(size_t N) : undo_history{}
	{
		this->N = N;
		vals = std::make_unique_for_overwrite<int[]>(N);
	}
	constexpr undo_history() : N{ 0 }, i0{ 0 }, n{ 0 }, m{ 0 }, vals{} {}
private:
	size_t i0, n, m, N;
	std::unique_ptr<T[]> vals;
};

constinit undo_history<int> cursor_undo_queue{};


////////////////////////////////
// helper functions.
////////////////////////////////
template<std::signed_integral IntT>
constexpr IntT normal_modulo(IntT a, IntT b)
{
	// returns a mod b in [0, b).
	// assumes b > 0.
	a %= b;
	if (a < 0) a += b;
	return a;
}

template<size_t N>
bool check_window_class(HWND hwnd, wchar_t const(&class_name)[N])
{
	wchar_t buf[N + 1];
	return N - 1 == ::GetClassNameW(hwnd, buf, std::size(buf))
		&& std::wcsncmp(buf, class_name, N - 1) == 0;
}

static bool write_key_state(uint8_t vk_code, uint8_t& state)
{
	// changes the key state and returns true if changed.
	// state is updated to the previous state.
	uint8_t keystates[256];
	std::ignore = ::GetKeyboardState(keystates);
	if (keystates[vk_code] == state) return false; // no change.

	std::swap(keystates[vk_code], state);
	::SetKeyboardState(keystates);
	return true; // changed.
}

static EDIT_INFO get_edit_info()
{
	EDIT_INFO info;
	edit_handle->get_edit_info(&info, sizeof(info));
	return info;
};


////////////////////////////////
// window message handler.
////////////////////////////////
constinit struct PluginWindow {
	HWND root = nullptr;

private:
	static constexpr std::wstring_view
		class_name = L"TLWalkaround2Client";
	struct ctrl_ids {
		enum id : uint32_t {
			page_rate_label = 1001,
			page_rate_slider,
			page_rate_edit,

			bpm_div_label,
			bpm_div_edit,
			bpm_div_spin,

			suppress_shift_check,
			focus_follows_check,

			timer_cursor_poll,
		};
	};
	struct prv_mes {
		enum id : uint32_t {
			request_callback = WM_USER + 64,
		};
	};
	struct {
		struct {
			HWND label = nullptr;
			HWND slider = nullptr;
			HWND edit = nullptr;

			constexpr static int slider_resolution = 1000, slider_tick_count = 10;
		} page_rate{};
		struct {
			HWND label = nullptr;
			HWND edit = nullptr;
			HWND spin = nullptr;
		} bpm_div{};
		struct {
			HWND check = nullptr;
		} suppress_shift{};
		struct {
			HWND check = nullptr;
		} focus_follows{};
		HFONT gui_font = nullptr;
		bool initialized() const { return gui_font != nullptr; }

		void layout(HWND root)
		{
			constexpr int margin_0 = 6,
				slider_height_0 = 32,
				unit_height_0 = 26,
				label_width_0 = 80,
				edit_width_0 = 56;

			auto const dpi = ::GetDpiForWindow(root);
			RECT client; ::GetWindowRect(root, &client);
			int const pad_y = ::GetSystemMetricsForDpi(SM_CYBORDER, dpi) + 1;
			int const edit_height = font_height(root) + 2 * pad_y;

			auto rescale = [dpi](int x) -> int { return x * dpi / 96; };
			auto repos = [](HWND ctrl, int w, int h, int x, int y) {
				::SetWindowPos(ctrl, nullptr, x, y, w, h,
					SWP_NOZORDER | SWP_NOACTIVATE);
				return w;
			};
			margin_1 = rescale(margin_0);
			int const
				unit_height_1 = rescale(unit_height_0),
				slider_height_1 = rescale(slider_height_0),
				label_width_1 = rescale(label_width_0),
				edit_width_1 = rescale(edit_width_0);

			// page-rate controls.
			int X = margin_1, Y = margin_1;
			X += repos(page_rate.label, label_width_1, edit_height - pad_y, X, Y + pad_y) + margin_1;
			X += repos(page_rate.edit, edit_width_1, edit_height, X, Y) + margin_1;
			pos_slider.left = X; pos_slider.top = Y - margin_1 / 2;
			pos_slider.right = client.right - margin_1;
			pos_slider.bottom = pos_slider.top + slider_height_1;
			repos(page_rate.slider, pos_slider.right - pos_slider.left, pos_slider.bottom - pos_slider.top,
				pos_slider.left, pos_slider.top);

			// bpm-grid-division controls.
			X = margin_1; Y += unit_height_1;
			X += repos(bpm_div.label, label_width_1, edit_height - pad_y, X, Y + pad_y) + margin_1;
			X += repos(bpm_div.edit, edit_width_1 - edit_height, edit_height, X, Y);
			repos(bpm_div.spin, edit_height, edit_height, X, Y);

			// suppress-shift control.
			X = margin_1; Y += unit_height_1;
			X += repos(suppress_shift.check, label_width_1 + edit_width_1, edit_height, X, Y);

			// focus-follows control.
			X = margin_1; Y += unit_height_1;
			X += repos(focus_follows.check, label_width_1 + edit_width_1, edit_height, X, Y);
		}
		void size_changed(int width, int height)
		{
			// resize the slider w.r.t. the window size.
			pos_slider.right = width - margin_1;
			::SetWindowPos(page_rate.slider, nullptr,
				pos_slider.left, pos_slider.top,
				pos_slider.right - pos_slider.left, pos_slider.bottom - pos_slider.top,
				SWP_NOZORDER | SWP_NOACTIVATE);
		}

	private:
		int font_height(HWND root) const
		{
			auto dc = ::GetDC(root);
			auto old_font = ::SelectObject(dc, gui_font);
			TEXTMETRICW tm; ::GetTextMetricsW(dc, &tm);
			::SelectObject(dc, old_font);
			::ReleaseDC(root, dc);
			return tm.tmHeight;
		}

		int margin_1 = {};
		RECT pos_slider = {};
	} ctrl{};

	void create_window_content()
	{
		if (ctrl.initialized()) return;

		HINSTANCE const hinst = reinterpret_cast<HINSTANCE>(::GetWindowLongPtrW(root, GWLP_HINSTANCE));
		constexpr auto id = [](ctrl_ids::id id) -> HMENU
		{
			return reinterpret_cast<HMENU>(static_cast<uintptr_t>(id));
		};

		NONCLIENTMETRICSW ncm{ .cbSize = sizeof(ncm) };
		::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
		ctrl.gui_font = ::CreateFontIndirectW(&ncm.lfCaptionFont);

		// create controls.
		// page-rate controls.
		ctrl.page_rate.label = ::CreateWindowExW(
			0, WC_STATICW, L"移動量(%):",
			WS_VISIBLE | WS_CHILD | SS_SIMPLE,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			root, id(ctrl_ids::page_rate_label), hinst, nullptr);
		ctrl.page_rate.edit = ::CreateWindowExW(
			0, WC_EDITW, L"",
			WS_VISIBLE | WS_CHILD | WS_BORDER | ES_RIGHT/* | ES_NUMBER*/, // ES_NUMBER blocks decimal point.
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			root, id(ctrl_ids::page_rate_edit), hinst, nullptr);
		ctrl.page_rate.slider = ::CreateWindowExW(
			0, TRACKBAR_CLASSW, nullptr,
			WS_VISIBLE | WS_CHILD | TBS_TRANSPARENTBKGND | TBS_AUTOTICKS | TBS_HORZ | TBS_BOTTOM,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			root, id(ctrl_ids::page_rate_slider), hinst, nullptr);

		// bpm-grid-division controls.
		ctrl.bpm_div.label = ::CreateWindowExW(
			0, WC_STATICW, L"BPM移動分母:",
			WS_VISIBLE | WS_CHILD | SS_SIMPLE,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			root, id(ctrl_ids::bpm_div_label), hinst, nullptr);
		ctrl.bpm_div.edit = ::CreateWindowExW(
			0, WC_EDITW, L"",
			WS_VISIBLE | WS_CHILD | WS_BORDER | ES_RIGHT | ES_NUMBER,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			root, id(ctrl_ids::bpm_div_edit), hinst, nullptr);
		ctrl.bpm_div.spin = ::CreateWindowExW(
			0, UPDOWN_CLASSW, nullptr,
			WS_VISIBLE | WS_CHILD | UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_SETBUDDYINT | UDS_ARROWKEYS | UDS_NOTHOUSANDS,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			root, id(ctrl_ids::bpm_div_spin), hinst, nullptr);

		// suppress-shift control.
		ctrl.suppress_shift.check = ::CreateWindowExW(
			0, WC_BUTTONW, L"範囲選択を抑制",
			WS_VISIBLE | WS_CHILD | BS_CHECKBOX | BS_AUTOCHECKBOX,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			root, id(ctrl_ids::suppress_shift_check), hinst, nullptr);

		// focus-follows control.
		ctrl.focus_follows.check = ::CreateWindowExW(
			0, WC_BUTTONW, L"選択オブジェクトの追従",
			WS_VISIBLE | WS_CHILD | BS_CHECKBOX | BS_AUTOCHECKBOX,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			root, id(ctrl_ids::focus_follows_check), hinst, nullptr);

		// set slider properties.
		::SendMessageW(ctrl.page_rate.slider, TBM_SETRANGE, TRUE, MAKELPARAM(
			static_cast<int>(settings.search.page_rate_min * ctrl.page_rate.slider_resolution),
			ctrl.page_rate.slider_resolution));
		::SendMessageW(ctrl.page_rate.slider, TBM_SETTICFREQ,
			ctrl.page_rate.slider_resolution / ctrl.page_rate.slider_tick_count, 0);
		::SendMessageW(ctrl.page_rate.slider, TBM_SETPOS, TRUE,
			static_cast<int>(std::round(settings.search.page_rate * ctrl.page_rate.slider_resolution)));

		// set spin properties.
		::SendMessageW(ctrl.bpm_div.spin, UDM_SETRANGE32, settings.search.bpm_grid_div_min, settings.search.bpm_grid_div_max);
		::SendMessageW(ctrl.bpm_div.spin, UDM_SETPOS32, 0, settings.search.bpm_grid_div);

		// set checkbox state.
		::SendMessageW(ctrl.suppress_shift.check, BM_SETCHECK, settings.search.suppress_shift ? BST_CHECKED : BST_UNCHECKED, 0);
		::SendMessageW(ctrl.focus_follows.check, BM_SETCHECK, settings.search.focus_follows ? BST_CHECKED : BST_UNCHECKED, 0);

		// set fonts.
		for (HWND control : {
			ctrl.page_rate.label,
				ctrl.page_rate.edit,
				ctrl.bpm_div.label,
				ctrl.bpm_div.edit,
				ctrl.suppress_shift.check,
				ctrl.focus_follows.check,
		}) ::SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(ctrl.gui_font), TRUE);

		// set subclass procedures for tab-navigation.
		for (HWND control : {
			ctrl.page_rate.edit,
				ctrl.page_rate.slider,
				ctrl.bpm_div.edit,
				ctrl.suppress_shift.check,
				ctrl.focus_follows.check,
		}) ::SetWindowSubclass(control, tab_navigation_proc, reinterpret_cast<UINT_PTR>(this), {});

		// initialize the layout.
		ctrl.layout(root);

		// first sync.
		sync_page_rate(true);

		// logging.
		logger->verbose(logger, L"Created controls on the client window.");
	}

public:
	bool create_register_window(HOST_APP_TABLE* host, wchar_t const* caption) const
	{
		WNDCLASSEXW wcex = {
			.cbSize = sizeof(wcex),
			.lpfnWndProc = [](auto... args)
			{
				return plugin_window.wnd_proc(args...);
			},
			.hInstance = ::GetModuleHandleW(nullptr),
			.hCursor = ::LoadCursorW(nullptr, IDC_ARROW),
			.hbrBackground = ::GetSysColorBrush(COLOR_3DFACE),
			.lpszClassName = class_name.data(),
		};
		if (::RegisterClassExW(&wcex) == 0) return false;
		::CreateWindowExW(0, wcex.lpszClassName, caption,
			WS_POPUP,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			nullptr, nullptr, ::GetModuleHandleW(nullptr), nullptr);

		host->register_window_client(caption, root);

		// logging.
		logger->verbose(logger, L"Created the client window.");
		return true;
	}
	template<class ParamT> requires(sizeof(ParamT) == sizeof(uintptr_t))
	void post_callback(void(*callback)(ParamT), ParamT param) const
	{
		post_callback(reinterpret_cast<void(*)(uintptr_t)>(callback), std::bit_cast<uintptr_t>(param));
	}
	void post_callback(void(*callback)(uintptr_t), uintptr_t param) const
	{
		::PostMessageW(root, prv_mes::request_callback, param, reinterpret_cast<LPARAM>(callback));
	}

private:
	bool sync_page_rate(bool from_slider) const
	{
		if (from_slider) {
			// page rate slider changed.
			int pos = static_cast<int>(::SendMessageW(ctrl.page_rate.slider, TBM_GETPOS, 0, 0));
			double rate = std::clamp(static_cast<double>(pos) / ctrl.page_rate.slider_resolution,
				settings.search.page_rate_min, settings.search.page_rate_max);

			// update edit box.
			wchar_t buf[16]; ::swprintf_s(buf, L"%.1f", 100 * rate);
			::SetWindowTextW(ctrl.page_rate.edit, buf);

			// update settings.
			settings.search.page_rate = rate;

			// logging.
			logger->verbose(logger, L"Settings synchronized: settings.search.page_rate.");
			return true;
		}
		else {
			// page rate edit changed.
			wchar_t buf[16]; wchar_t* buf_end;
			::GetWindowTextW(ctrl.page_rate.edit, buf, static_cast<int>(std::size(buf)));
			double val = std::wcstod(buf, &buf_end);
			if (*buf_end != L'\0' || std::isinf(val)) {
				// parsing failed.
				sync_page_rate(true);
				return false;
			}
			val /= 100;
			val = std::clamp(val, settings.search.page_rate_min, settings.search.page_rate_max);

			// update slider.
			::SendMessageW(ctrl.page_rate.slider, TBM_SETPOS, TRUE,
				static_cast<LPARAM>(std::round(val * ctrl.page_rate.slider_resolution)));

			// re-format the text.
			sync_page_rate(true);
			return true;
		}
	}

	bool sync_bpm_div() const
	{
		// bpm grid division edit changed.
		wchar_t buf[16];
		::GetWindowTextW(ctrl.bpm_div.edit, buf, static_cast<int>(std::size(buf)));
		int val = std::wcstol(buf, nullptr, 10);
		if (val == 0 || val == LONG_MAX) {
			// parsing failed.
			::swprintf_s(buf, L"%d", settings.search.bpm_grid_div);
			::SetWindowTextW(ctrl.bpm_div.edit, buf);
			return false;
		}

		val = std::clamp(val, settings.search.bpm_grid_div_min, settings.search.bpm_grid_div_max);
		settings.search.bpm_grid_div = val;

		// logging.
		logger->verbose(logger, L"Settings synchronized: settings.search.bpm_grid_div.");
		return true;
	}

	void sync_suppress_shift() const
	{
		// suppress shift checkbox changed.
		settings.search.suppress_shift =
			::SendMessageW(ctrl.suppress_shift.check, BM_GETCHECK, 0, 0) == BST_CHECKED;

		// logging.
		logger->verbose(logger, L"Settings synchronized: settings.search.suppress_shift.");
	}

	void sync_focus_follows() const
	{
		// focus follows checkbox changed.
		settings.search.focus_follows =
			::SendMessageW(ctrl.focus_follows.check, BM_GETCHECK, 0, 0) == BST_CHECKED;

		// logging.
		logger->verbose(logger, L"Settings synchronized: settings.search.focus_follows.");
	}

	static LRESULT CALLBACK tab_navigation_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, UINT_PTR id, DWORD_PTR data)
	{
		switch (message) {
		case WM_KEYDOWN:
		{
			// for tab key, move to the next element.
			switch (wparam) {
			case VK_TAB:
			{
				PluginWindow* that = reinterpret_cast<PluginWindow*>(id);
				HWND const controls[] = {
					that->ctrl.page_rate.edit,
					that->ctrl.page_rate.slider,
					that->ctrl.bpm_div.edit,
					that->ctrl.suppress_shift.check,
					that->ctrl.focus_follows.check,
				};
				size_t i = std::find(std::begin(controls), std::end(controls), hwnd) - std::begin(controls);
				if (i >= std::size(controls)) [[unlikely]] break; // not found.

				// determine the next control.
				i += ::GetKeyState(VK_SHIFT) < 0 ? std::size(controls) - 1 : 1;
				i %= std::size(controls);
				::SetFocus(controls[i]);

				// eliminate WM_CHAR message from the message queue.
				for (MSG msg; ::PeekMessageW(&msg, hwnd, WM_CHAR, WM_CHAR, PM_REMOVE) != FALSE; );
				return 0;
			}
			case VK_RETURN:
			{
				if (!check_window_class(hwnd, WC_EDITW)) break;

				// confirm the input of the text box,
				// by sending a fake EN_KILLFOCUS notification message.
				::SendMessageW(::GetParent(hwnd), WM_COMMAND,
					MAKEWPARAM(::GetDlgCtrlID(hwnd), EN_KILLFOCUS),
					reinterpret_cast<LPARAM>(hwnd));

				// eliminate WM_CHAR message from the message queue.
				for (MSG msg; ::PeekMessageW(&msg, hwnd, WM_CHAR, WM_CHAR, PM_REMOVE) != FALSE; );
				return 0;
			}
			}
			break;
		}
		case WM_SETFOCUS:
		{
			// select all text if it's an edit control.
			if (check_window_class(hwnd, WC_EDITW))
				::PostMessageW(hwnd, EM_SETSEL, 0, -1);
			break;
		}
		case WM_DESTROY:
		{
			::RemoveWindowSubclass(hwnd, tab_navigation_proc, id);
			break;
		}
		}
		return ::DefSubclassProc(hwnd, message, wparam, lparam);
	}

	LRESULT wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
	{
		switch (message) {
		case WM_CREATE:
		{
			root = hwnd;

			// setup polling timer.
			::SetTimer(root, ctrl_ids::timer_cursor_poll,
				static_cast<uint32_t>(std::round(1000 * settings.cursor_undo.polling_period)), nullptr);
			return 0;
		}
		case WM_NCDESTROY:
		{
			// delete the font.
			if (ctrl.initialized()) {
				::DeleteObject(ctrl.gui_font);
				ctrl.gui_font = nullptr;
			}

			// turn the timer off.
			::KillTimer(root, ctrl_ids::timer_cursor_poll);

			// save the setting on terminating.
			settings.save();
			break;
		}
		// initialize and re-layout controls.
		case WM_SHOWWINDOW:
		{
			if (wparam == FALSE || ::IsWindowVisible(root) == FALSE) break;

			// fake the parameters.
			wparam = SIZE_RESTORED;
			RECT rc;
			::GetClientRect(root, &rc);
			lparam = MAKELPARAM(rc.right, rc.bottom);
			[[fallthrough]];
		}
		case WM_SIZE:
		{
			if (::IsWindowVisible(root) == FALSE) break;
			create_window_content();

			ctrl.size_changed(LOWORD(lparam), HIWORD(lparam));
			return 0;
		}
		case WM_DPICHANGED:
		{
			if (ctrl.initialized()) ctrl.layout(root);
			break;
		}
		// messages from children.
		case WM_HSCROLL:
		{
			if (!ctrl.initialized()) break;

			if (reinterpret_cast<HWND>(lparam) == ctrl.page_rate.slider) {
				sync_page_rate(true);
				return 0;
			}
			break;
		}
		case WM_COMMAND:
		{
			if (!ctrl.initialized()) break;

			// synchronize setting values with the controls.
			HWND src = reinterpret_cast<HWND>(lparam);
			switch (auto id_cmd = LOWORD(wparam)) {
			case ctrl_ids::page_rate_edit:
			{
				switch (HIWORD(wparam)) {
				case EN_KILLFOCUS:
				{
					if (sync_page_rate(false)) return 0;
					break;
				}
				}
				break;
			}
			case ctrl_ids::bpm_div_edit:
			{
				switch (HIWORD(wparam)) {
				case EN_KILLFOCUS:
				{
					if (sync_bpm_div()) return 0;
					break;
				}
				}
				break;
			}
			case ctrl_ids::suppress_shift_check:
			{
				switch (HIWORD(wparam)) {
				case BN_CLICKED:
				{
					sync_suppress_shift();
					return 0;
				}
				}
				break;
			}
			case ctrl_ids::focus_follows_check:
			{
				switch (HIWORD(wparam)) {
				case BN_CLICKED:
				{
					sync_focus_follows();
					return 0;
				}
				}
				break;
			}
			}
			break;
		}
		case WM_TIMER:
		{
			switch (static_cast<uint32_t>(wparam)) {
			case ctrl_ids::timer_cursor_poll:
			{
				// periodically record the current frame.
				cursor_undo_queue.check_forward(get_edit_info().frame);
				return 0;
			}
			}
			break;
		}
		// private messages.
		case prv_mes::request_callback:
		{
			// call the requested function.
			auto const callback = reinterpret_cast<void(*)(uintptr_t)>(lparam);
			if (callback != nullptr) callback(wparam);
			return 0;
		}
		default:
			break;
		}
		return ::DefWindowProcW(hwnd, message, wparam, lparam);
	}
} plugin_window{};


////////////////////////////////
// timeline searching functions.
////////////////////////////////
static std::vector<int> find_midpoints(std::string_view const& alias)
{
	constexpr std::string_view token1 = "\nframe=", token2 = "\n";
	if (auto pos = alias.find(token1, 0);
		pos != alias.npos) {
		auto l = alias.substr(pos + token1.size());
		l = l.substr(0, l.find(token2, 0));

		auto const* p = reinterpret_cast<char const*>(&*l.begin());
		auto const* const p_end = p + l.size();
		std::vector<int> ret{};
		while (p < p_end) {
			char* next;
			int f = std::strtol(p, &next, 10);
			if (p == next || f == LONG_MAX) return {};
			ret.push_back(f);
			p = next + 1;
		}
		if (ret.size() >= 2) {
			ret.back()++;
			return ret;
		}
	}
	logger->error(logger, L"midpoints could not be identified!");
	return {};
}

static std::tuple<OBJECT_HANDLE, int, int> find_next_obj(EDIT_SECTION* edit, int layer, int frame)
{
	if (frame > edit->info->frame_max) return { nullptr, 0, 0 };

	// firstly find an object from `frame - 1` (whose end frame + 1 might be `frame`).
	auto const obj = edit->find_object(layer, frame - 1);
	if (obj == nullptr) return { nullptr, 0, 0 };

	// if found, then it's the desired one.
	auto const [_, st, ed] = edit->get_object_layer_frame(obj);
	return { obj, st, ed + 1 };
}

static std::tuple<OBJECT_HANDLE, int, int> find_prev_obj(EDIT_SECTION* edit, int layer, int frame)
{
	if (frame < 0) return { nullptr, 0, 0 };

	// firstly find an object from `frame`.
	auto const obj_r = edit->find_object(layer, frame);
	if (obj_r != nullptr) {
		// see if it starts before that frame.
		auto const [_, start, end] = edit->get_object_layer_frame(obj_r);
		if (start <= frame)
			return { obj_r, start, end + 1 };
	}

	// secondly, search from the beginning.
	auto const obj_l = edit->find_object(layer, 0);
	if (obj_l == nullptr || obj_l == obj_r) return { nullptr, 0, 0 };
	// see if it's relevant to the desired frame.
	auto I = edit->get_object_layer_frame(obj_l);
	std::tuple<OBJECT_HANDLE, int, int> ret = { obj_l, I.start, I.end + 1 };
	if (I.end + 1 >= frame) return ret;

	// then perform binary search.
	int frame_l = std::get<2>(ret), frame_r = frame;
	while (frame_l + 1 < frame_r) {
		int frame_m = (frame_l + frame_r) >> 1;
		auto obj_m = edit->find_object(layer, frame_m);
		if (obj_m == obj_r) frame_r = frame_m;
		else {
			// obj_m must be different from both std::get<0>(ret) and obj_r.
			I = edit->get_object_layer_frame(obj_m);
			ret = { obj_m, I.start, I.end + 1 };
			if (I.end + 1 >= frame) break;

			frame_l = std::get<2>(ret);
		}
	}
	return ret;
}

static size_t find_next_midpoint(std::vector<int> const& midpoints, int frame)
{
	// find the least midpoint which is >= `frame`.
	// `midpoints` is in ascending order, so find by binary search.
	// `frame` is assumed to be contained in the range.
	// returns the index of the found midpoint.
	int l = -1, r = static_cast<int>(midpoints.size());
	while (l + 1 < r) {
		int m = (l + r) >> 1;
		if (midpoints[m] >= frame) r = m;
		else l = m;
	}
	assert(r != static_cast<int>(midpoints.size()));
	return r;
}

static size_t find_prev_midpoint(std::vector<int> const& midpoints, int frame)
{
	// find the greatest midpoint which is <= `frame`.
	// `midpoints` is in ascending order, so find by binary search.
	// `frame` is assumed to be contained in the range.
	// returns the index of the found midpoint.
	auto idx = find_next_midpoint(midpoints, frame);
	if (midpoints[idx] > frame) idx--;
	return idx;
}

static int find_boundary(EDIT_SECTION* edit, int layer, int frame, bool forward, bool allow_midpt)
{
	if (forward) {
		// search forward
		auto const [obj, obj_start, obj_end] = find_next_obj(edit, layer, frame);
		if (obj == nullptr) return edit->info->frame_max; // no more object
		else if (allow_midpt && frame > obj_start) {
			// see if there is a midpoint on the found object.
			auto const alias = edit->get_object_alias(obj);
			auto const midpoints = find_midpoints(alias);
			if (midpoints.size() > 2) {
				auto const idx = find_next_midpoint(midpoints, frame);
				return midpoints[idx];
			}
		}
		return frame <= obj_start ? obj_start : obj_end;
	}
	else {
		// search backward
		auto const [obj, obj_start, obj_end] = find_prev_obj(edit, layer, frame);
		if (obj == nullptr) return 0; // no more object
		else if (allow_midpt && obj_end > frame) {
			// see if there is a midpoint on the found object.
			auto const alias = edit->get_object_alias(obj);
			auto const midpoints = find_midpoints(alias);
			if (midpoints.size() > 2) {
				auto const idx = find_prev_midpoint(midpoints, frame);
				return midpoints[idx];
			}
		}
		return obj_end <= frame ? obj_end : obj_start;
	}
}


////////////////////////////////
// BPM calculation helpers.
////////////////////////////////
struct Timeline_calc {
	int rate, scale; // numerator and denominator of framerate.
	constexpr Timeline_calc(int r, int s) : rate(r), scale(s) {}
	constexpr double frame_to_second(double frame_num) const
	{
		return frame_num * scale / rate;
	}
	constexpr double second_to_frame(double second_num) const
	{
		return second_num * rate / scale;
	}
};
struct BPM_grid_calc : Timeline_calc {
	float tempo; float offset;
	constexpr BPM_grid_calc(float t, float o, int r, int s)
		: Timeline_calc{ r, s }
		, tempo(t), offset(o) {}

	constexpr double beat_to_frame(double beat_num) const
	{
		return second_to_frame(beat_to_second(beat_num));
	}
	int beat_to_frame_int(double beat_num) const
	{
		// rounding is downward.
		return static_cast<int>(std::floor(beat_to_frame(beat_num)));
	}
	constexpr double frame_to_beat(double frame_num) const
	{
		return second_to_beat(frame_to_second(frame_num));
	}

	constexpr double beat_to_second(double beat_num) const
	{
		return 60.0 * beat_num / tempo + offset;
	}
	constexpr double second_to_beat(double second_num) const
	{
		return (second_num - offset) * tempo / 60.0f;
	}
};


////////////////////////////////
// moving cursor functions.
////////////////////////////////
static void move_frame_wrap(EDIT_SECTION* edit, int layer, int frame)
{
	frame = std::clamp(frame, 0, edit->info->frame_max);
	int const d_layer = layer - edit->info->layer,
		d_frame = frame - edit->info->frame;
	if (d_layer == 0 && d_frame == 0) return;

	cursor_undo_queue.check_forward(frame);
	if (settings.search.suppress_shift) {
		// fake keyboard state.
		uint8_t key_state = 0x00;
		if (write_key_state(VK_SHIFT, key_state)) {
			// rewind the key state after the callback returns.
			plugin_window.post_callback([](uintptr_t key_state) static
			{
				uint8_t state = static_cast<uint8_t>(key_state);
				write_key_state(VK_SHIFT, state);
			}, key_state);
		}
	}
	edit->set_cursor_layer_frame(layer, frame);

	// let the focus follow the cursor.
	if (d_frame != 0 && settings.search.focus_follows) {
		auto const obj = edit->get_focus_object();
		if (obj != nullptr) {
			auto pos = edit->get_object_layer_frame(obj);
			if (frame < pos.start || pos.end < frame) {
				auto const tgt = edit->find_object(pos.layer, frame);
				if (tgt != nullptr && tgt != obj) {
					pos = edit->get_object_layer_frame(tgt);
					if (pos.start <= frame && frame <= pos.end) {
						edit->set_focus_object(tgt);
					}
				}
			}
		}
	}
}
static void move_layer_core(EDIT_SECTION* edit, bool forward, bool allow_midpt)
{
	// move to the next point of the layer the focused object is on.
	// (if there's no focused object, fallback to the currently selected layer.)
	auto const obj = edit->get_focus_object();
	int const frame = edit->info->frame + (forward ? +1 : -1),
		layer = obj != nullptr ?
			edit->get_object_layer_frame(obj).layer :
			edit->info->layer;
	int next_frame = find_boundary(edit, layer, frame, forward, allow_midpt);
	move_frame_wrap(edit, edit->info->layer, next_frame);
}

static void move_scene_core(EDIT_SECTION* edit, bool forward, bool allow_midpt)
{
	// move to the next point of the entire scene.
	int const frame = edit->info->frame + (forward ? +1 : -1);
	int next_frame = forward ? edit->info->frame_max : 0;
	for (int layer = edit->info->layer_max; layer >= 0; layer--) {
		auto f = find_boundary(edit, layer, frame, forward, allow_midpt);
		next_frame = (next_frame > f) == forward ? f : next_frame;
	}
	move_frame_wrap(edit, edit->info->layer, next_frame);
}

static void move_per_page(EDIT_SECTION* edit, double rate)
{
	// move by page.
	int const delta = static_cast<int>(std::round(edit->info->display_frame_num * rate));
	move_frame_wrap(edit, edit->info->layer, edit->info->frame + delta);
}

static void move_to_focused(EDIT_SECTION* edit)
{
	// move to the start or end of the focused object, if the cursor is out of the range.
	auto const obj = edit->get_focus_object();
	if (obj != nullptr) {
		auto const [layer, start, end] = edit->get_object_layer_frame(obj);
		int next_frame = edit->info->frame;
		if (next_frame < start) next_frame = start;
		else if (next_frame > end) next_frame = end;
		move_frame_wrap(edit, layer, next_frame);
	}
}

static void move_to_timeline_center(EDIT_SECTION* edit)
{
	// move to the center of the timeline view.
	int const next_frame = edit->info->display_frame_start + (edit->info->display_frame_num >> 1);
	move_frame_wrap(edit, edit->info->layer, next_frame);
}


////////////////////////////////
// horizontal scroll functions.
////////////////////////////////
static void scroll_horiz_per_page(EDIT_SECTION* edit, double rate)
{
	int const delta = static_cast<int>(std::round(edit->info->display_frame_num * rate));
	if (delta != 0)
		edit->set_display_layer_frame(edit->info->display_layer_start, edit->info->display_frame_start + delta);
}

static void scroll_horiz_to_start_end(EDIT_SECTION* edit, bool to_start)
{
	int const frame = to_start ? 0 : (edit->info->frame_max - (edit->info->display_frame_num >> 1));
	edit->set_display_layer_frame(edit->info->display_layer_start, frame);
}

static void scroll_horiz_to_cursor(EDIT_SECTION* edit)
{
	// scroll so the cursor is centered.
	int const next_frame = edit->info->frame - (edit->info->display_frame_num >> 1);
	if (next_frame != edit->info->display_frame_start)
		edit->set_display_layer_frame(edit->info->display_layer_start, next_frame);
}

static void scroll_to_focused(EDIT_SECTION* edit)
{
	// scroll so the focused object is centered.
	auto const obj = edit->get_focus_object();
	if (obj != nullptr) {
		auto [layer, start, end] = edit->get_object_layer_frame(obj);
		int next_frame = edit->info->display_frame_start, next_layer = edit->info->display_layer_start;
		if (int const center_frame = edit->info->display_frame_start + (edit->info->display_frame_num >> 1);
			end < center_frame || center_frame < start) {
			// move so the nearest edge is centered.
			next_frame = (end < center_frame ? end : start) - (edit->info->display_frame_num >> 1);
		}
		// move so the target layer is centered.
		next_layer = layer - (edit->info->display_layer_num >> 1);

		// then scroll.
		if (next_frame != edit->info->display_frame_start || next_layer != edit->info->display_layer_start)
			edit->set_display_layer_frame(next_layer, next_frame);
	}
}


////////////////////////////////
// vertical scroll functions.
////////////////////////////////
static void scroll_vert_to_top_bottom(EDIT_SECTION* edit, bool to_top)
{
	int const layer = to_top ? 0 : (edit->info->layer_max - (edit->info->display_layer_num >> 1));
	edit->set_display_layer_frame(layer, edit->info->display_frame_start);
}

static void scroll_vert_to_selected(EDIT_SECTION* edit)
{
	// scroll so the target layer is centered.
	int const next_layer = edit->info->layer - (edit->info->display_layer_num >> 1);
	if (next_layer != edit->info->display_layer_start)
		edit->set_display_layer_frame(next_layer, edit->info->display_frame_start);
}


////////////////////////////////
// object focus functions.
////////////////////////////////
static void focus_cursor_object_rev(EDIT_SECTION* edit)
{
	int const num_layers = edit->info->layer_max + 1, cursor_frame = edit->info->frame;
	if (num_layers <= 0) return;

	// see if the current focused object is on the cursor.
	auto obj_curr = edit->get_focus_object();
	int layer_end = 0;
	if (obj_curr != nullptr) {
		auto [layer, start, end] = edit->get_object_layer_frame(obj_curr);
		if (start <= cursor_frame && cursor_frame <= end)
			// if on the cursor, search starts from above this layer.
			layer_end = layer;
		else obj_curr = nullptr; // otherwise, search from the bottom.
	}

	// search for the object on the cursor, in backward order of layers.
	int layer = obj_curr != nullptr ? layer_end : num_layers;
	do {
		layer = normal_modulo(layer - 1, num_layers);

		// get the object on the layer from the cursor position.
		auto obj = edit->find_object(layer, cursor_frame);
		if (obj == nullptr) continue;

		// see if it's on the cursor.
		if (edit->get_object_layer_frame(obj).start <= cursor_frame) {
			// then set focus.
			if (obj != obj_curr) edit->set_focus_object(obj);
			return;
		}
	} while (layer != layer_end);
}

static void focus_left_right_object(EDIT_SECTION* edit, bool right)
{
	OBJECT_HANDLE target_obj;
	auto const curr_obj = edit->get_focus_object();
	if (curr_obj == nullptr) {
		// focus the object at the current layer/frame position.
		target_obj = edit->find_object(edit->info->layer, edit->info->frame);
	}
	else {
		auto const pos = edit->get_object_layer_frame(curr_obj);
		target_obj = right ? edit->find_object(pos.layer, pos.end + 1) :
			std::get<0>(find_prev_obj(edit, pos.layer, pos.start - 1));
	}

	if (target_obj != nullptr) edit->set_focus_object(target_obj);
}

static void focus_above_below_layer_object(EDIT_SECTION* edit, bool below)
{
	int const layer_end = below ? edit->info->layer_max + 1 : -1, layer_delta = below ? +1 : -1;
	auto const curr_obj = edit->get_focus_object();
	if (curr_obj == nullptr) return;

	// find the nearest object to the mid_frame.
	auto const [curr_layer, start, end] = edit->get_object_layer_frame(curr_obj);
	int const mid_frame = (start + end) >> 1;
	std::pair<OBJECT_HANDLE, int> minimum = { nullptr, std::numeric_limits<int>::max() }; // (obj, distance)
	for (int layer = curr_layer + layer_delta; minimum.first == nullptr && layer != layer_end; layer += layer_delta) {
		// make the list of candidate objects, which has intersection with the [start, end] range.
		for (int frame = start; frame <= mid_frame; ) {
			auto const obj = edit->find_object(layer, frame);
			if (obj == nullptr) break;
			auto const [_, obj_start, obj_end] = edit->get_object_layer_frame(obj);
			if (obj_start > end) break; // no more intersection

			// measure the distance to the mid_frame.
			int const distance = std::max({ 0, obj_start - mid_frame, mid_frame - obj_end });
			if (distance < minimum.second) minimum = { obj, distance };
			frame = obj_end + 1;
		}
	}

	// then set the focus.
	if (minimum.first != nullptr)
		edit->set_focus_object(minimum.first);
}


////////////////////////////////
// BPM grid operations.
////////////////////////////////
static void move_to_bpm_grid(EDIT_SECTION* edit, int tempo_factor_num, int tempo_factor_den, bool forward)
{
	// move to the nearest BPM grid point.
	BPM_grid_calc const bpm_calc{
		edit->info->grid_bpm_tempo * tempo_factor_num / tempo_factor_den,
		edit->info->grid_bpm_offset,
		edit->info->rate,
		edit->info->scale
	};
	double target_beat;
	if (forward) // calculate from the next frame.
		target_beat = std::ceil(bpm_calc.frame_to_beat(edit->info->frame + 1));
	else // calculate from the previous frame.
		target_beat = std::ceil(bpm_calc.frame_to_beat(edit->info->frame)) - 1;

	// then move to the BPM grid.
	int const next_frame = bpm_calc.beat_to_frame_int(target_beat);
	move_frame_wrap(edit, edit->info->layer, next_frame);
}

static void shift_bpm_grid_nearest_measure_to_cursor(EDIT_SECTION* edit)
{
	// shift the BPM grid offset so that the nearest measure line is on the cursor.
	BPM_grid_calc const bpm_calc{
		edit->info->grid_bpm_tempo / edit->info->grid_bpm_beat,
		edit->info->grid_bpm_offset,
		edit->info->rate,
		edit->info->scale
	};

	// find the nearest measure.
	double const curr_sec = bpm_calc.frame_to_second(edit->info->frame);
	double const measure = std::round(bpm_calc.second_to_beat(curr_sec));

	// calculate the frame at that measure.
	int const measure_frame = bpm_calc.beat_to_frame_int(measure);

	// then move the BPM grid.
	edit->set_grid_bpm(
		edit->info->grid_bpm_tempo,
		edit->info->grid_bpm_beat,
		static_cast<float>(edit->info->grid_bpm_offset
		+ bpm_calc.frame_to_second(edit->info->frame - measure_frame))
	);
}

static void shift_bpm_grid_offset(EDIT_SECTION* edit, int frames)
{
	Timeline_calc const tl_calc{
		edit->info->rate,
		edit->info->scale
	};

	// move the BPM grid.
	edit->set_grid_bpm(
		edit->info->grid_bpm_tempo,
		edit->info->grid_bpm_beat,
		static_cast<float>(edit->info->grid_bpm_offset
		+ tl_calc.frame_to_second(frames))
	);
}


////////////////////////////////
// repositioning objects.
////////////////////////////////
enum class Direction {
	Left, Right, Up, Down,
};
static void move_selected_objects(EDIT_SECTION* edit, Direction dir)
{
	std::vector<std::pair<OBJECT_HANDLE, OBJECT_LAYER_FRAME>> targets{};

	// collect target objects.
	if (int const n = edit->get_selected_object_num(); n > 0) {
		for (int i = 0; i < n; i++)
			targets.emplace_back(edit->get_selected_object(i), OBJECT_LAYER_FRAME{});
	}
	else if (auto const obj = edit->get_focus_object(); obj != nullptr)
		targets.emplace_back(obj, OBJECT_LAYER_FRAME{});
	if (targets.empty()) return; // no operation.

	// get their positions and min/max of the range.
	std::set<OBJECT_HANDLE> target_set{};
	int layer_min = edit->info->layer_max,
		layer_max = 0,
		frame_min = edit->info->frame_max,
		frame_max = 0;
	for (auto& [obj, pos] : targets) {
		pos = edit->get_object_layer_frame(obj);
		layer_min = std::min(layer_min, pos.layer);
		layer_max = std::max(layer_max, pos.layer);
		frame_min = std::min(frame_min, pos.start);
		frame_max = std::max(frame_max, pos.end);
		target_set.emplace(obj);
	}

	// sort these objects by layer, and then by frame.
	std::sort(targets.begin(), targets.end(), [](auto const& p1, auto const& p2) -> bool {
		auto const& pos1 = p1.second;
		auto const& pos2 = p2.second;
		return pos1.layer < pos2.layer ||
			(pos1.layer == pos2.layer && pos1.start < pos2.start);
	});

	// then move.
	uint32_t moved_count = 0, left_behind = 0;
	switch (dir) {
	case Direction::Left:
	{
		int offset = frame_min;

		// find the maximum offset that each object does not collide with others.
		for (auto const& [_, pos] : targets) {
			auto const [o, s, e] = find_prev_obj(edit, pos.layer, pos.start - 1);
			if (o == nullptr) continue;
			if (target_set.contains(o)) continue; // ignore target objects.
			offset = std::min(offset, pos.start - e);
		}

		if (offset == 0 && frame_min > 0) {
			// if no space is found, "jump over" the objects to left
			// and find the nearest possible space.
			for (int ofs = 2, prev = ofs; ofs <= frame_min; prev = ofs) {
				for (auto const& [_, pos] : targets) {
					for (int f = pos.start - ofs; f <= pos.end - ofs; ) {
						auto const o = edit->find_object(pos.layer, f);
						if (o == nullptr) break;
						auto const p = edit->get_object_layer_frame(o);
						f = p.end + 1;
						if (target_set.contains(o)) continue; // ignore target objects.
						if (std::max(pos.start - ofs, p.start) <= std::min(pos.end - ofs, p.end)) {
							// objects will overlap. find the next candidate.
							ofs = pos.end + 1 - p.start;
							break;
						}
					}
				}
				if (prev == ofs) {
					offset = ofs;
					break;
				}
			}
		}

		// move left if suitable space is found.
		if (offset > 0) {
			for (auto const& [obj, pos] : targets) {
				if (edit->move_object(obj, pos.layer, pos.start - offset))
					moved_count++;
			}
			left_behind = static_cast<uint32_t>(targets.size()) - moved_count;
		}
		break;
	}
	case Direction::Right:
	{
		std::reverse(targets.begin(), targets.end());
		int offset = edit->info->frame_max - frame_min + 1;

		// find the maximum offset that each object does not collide with others.
		for (auto const& [_, pos] : targets) {
			auto const o = edit->find_object(pos.layer, pos.end + 1);
			if (o == nullptr) continue;
			if (target_set.contains(o)) continue; // ignore target objects.
			auto const p = edit->get_object_layer_frame(o);
			offset = std::min(offset, p.start - pos.end - 1);
		}

		if (offset == 0) {
			// if no space is found, "jump over" the objects to right
			// and find the nearest possible space.
			for (int ofs = 2, prev = ofs; ; prev = ofs) {
				for (auto const& [_, pos] : targets) {
					for (int f = pos.start + ofs; f <= pos.end + ofs; ) {
						auto const o = edit->find_object(pos.layer, f);
						if (o == nullptr) break;
						auto const p = edit->get_object_layer_frame(o);
						f = p.end + 1;
						if (target_set.contains(o)) continue; // ignore target objects.
						if (std::max(pos.start + ofs, p.start) <= std::min(pos.end + ofs, p.end)) {
							// objects will overlap. find the next candidate.
							ofs = p.end + 1 - pos.start;
							// keep searching.
						}
					}
				}
				if (prev == ofs) {
					offset = ofs;
					break;
				}
			}
		}
		else if (offset + frame_min > edit->info->frame_max)
			// if no objects are on the way,
			// move to the right-most of the timeline.
			offset = edit->info->frame_max - frame_max;

		// move right if suitable space is found.
		if (offset > 0) {
			for (auto const& [obj, pos] : targets) {
				if (edit->move_object(obj, pos.layer, pos.start + offset))
					moved_count++;
			}
			left_behind = static_cast<uint32_t>(targets.size()) - moved_count;
		}
		break;
	}
	case Direction::Up:
	case Direction::Down:
	{
		int const d = dir == Direction::Up ? -1 : +1;
		if (d > 0) std::reverse(targets.begin(), targets.end());

		// step by 1 layer and check for collisions.
		int diff = d;
		for (; diff + layer_min >= 0; diff += d) {
			for (auto const& [_, pos] : targets) {
				for (int f = pos.start; f <= pos.end;) {
					auto const o = edit->find_object(pos.layer + diff, f);
					if (o == nullptr) break;
					auto const p = edit->get_object_layer_frame(o);
					f = p.end + 1;
					if (target_set.contains(o)) continue; // ignore target objects.
					if (std::max(pos.start, p.start) <= std::min(pos.end, p.end))
						goto found_collision_v;
				}
			}
			break;
		found_collision_v:;
		}

		// move up or down if no collision found.
		if (diff + layer_min >= 0) {
			for (auto const& [obj, pos] : targets) {
				if (edit->move_object(obj, pos.layer + diff, pos.start))
					moved_count++;
			}
			left_behind = static_cast<uint32_t>(targets.size()) - moved_count;
		}
		break;
	}
	}

	// output an information message.
	if (moved_count + left_behind > 0) {
		constexpr std::wstring_view
			pat1 = L"Moved %d object(s).", pat2 = L" (%d left behind.)";
		constexpr size_t len_num = 11; // "-2147483648"
		wchar_t buf[std::bit_ceil(pat1.size() + pat2.size() + 2 * (len_num - 2) + 1)];
		auto const len = ::swprintf_s(buf, pat1.data(), moved_count);
		if (left_behind > 0) {
			::swprintf_s(buf + len, std::size(buf) - len, pat2.data(), left_behind);
			logger->warn(logger, buf);
		}
		else logger->info(logger, buf);
	}
	else logger->info(logger, L"Found no space to move the object(s).");
}


////////////////////////////////
// cursor undo operations.
////////////////////////////////
static void cursor_undo(EDIT_SECTION* edit)
{
	cursor_undo_queue.check_forward(edit->info->frame);
	if (cursor_undo_queue.size() <= 1) return;
	move_frame_wrap(edit, edit->info->layer, cursor_undo_queue.backward());
}

static void cursor_redo(EDIT_SECTION* edit)
{
	cursor_undo_queue.check_forward(edit->info->frame);
	if (int next_frame; cursor_undo_queue.repush(next_frame))
		move_frame_wrap(edit, edit->info->layer, next_frame);
}


////////////////////////////////
// other plugin callbacks.
////////////////////////////////
static void on_load_project(PROJECT_FILE* project)
{
	// clear the queue and record the initial frame.
	plugin_window.post_callback([](uintptr_t) static
	{
		cursor_undo_queue.clear(get_edit_info().frame);
		logger->verbose(logger, L"Cursor undo buffer cleared.");
	}, {});
}


////////////////////////////////
// define menu items.
////////////////////////////////
#define NAME(name)	PLUGIN_NAME "\\" name
constexpr struct {
	wchar_t const* name;
	void (*callback)(EDIT_SECTION* edit);
} edit_menu_items[] = {
	{ NAME(L"左の中間点(レイヤー)"), [](EDIT_SECTION* edit)
	{
		move_layer_core(edit, false, true);
	}
	},
	{ NAME(L"右の中間点(レイヤー)"), [](EDIT_SECTION* edit)
	{
		move_layer_core(edit, true, true);
	}
	},
	{ NAME(L"左の境界(レイヤー)"), [](EDIT_SECTION* edit)
	{
		move_layer_core(edit, false, false);
	}
	},
	{ NAME(L"右の境界(レイヤー)"), [](EDIT_SECTION* edit)
	{
		move_layer_core(edit, true, false);
	}
	},
	{ NAME(L"左の中間点(シーン)"), [](EDIT_SECTION* edit)
	{
		move_scene_core(edit, false, true);
	}
	},
	{ NAME(L"右の中間点(シーン)"), [](EDIT_SECTION* edit)
	{
		move_scene_core(edit, true, true);
	}
	},
	{ NAME(L"左の境界(シーン)"), [](EDIT_SECTION* edit)
	{
		move_scene_core(edit, false, false);
	}
	},
	{ NAME(L"右の境界(シーン)"), [](EDIT_SECTION* edit)
	{
		move_scene_core(edit, true, false);
	}
	},
	{ NAME(L"左へ1ページ移動"), [](EDIT_SECTION* edit)
	{
		move_per_page(edit, -1.0);
	}
	},
	{ NAME(L"右へ1ページ移動"), [](EDIT_SECTION* edit)
	{
		move_per_page(edit, +1.0);
	}
	},
	{ NAME(L"左へ一定量移動"), [](EDIT_SECTION* edit)
	{
		move_per_page(edit, -settings.search.page_rate);
	}
	},
	{ NAME(L"右へ一定量移動"), [](EDIT_SECTION* edit)
	{
		move_per_page(edit, +settings.search.page_rate);
	}
	},
	{ NAME(L"選択オブジェクトへ移動"), &move_to_focused },
	{ NAME(L"タイムラインの中央へ移動"), &move_to_timeline_center },

	{ NAME(L"左へ1ページスクロール"), [](EDIT_SECTION* edit)
	{
		scroll_horiz_per_page(edit, -1.0);
	}
	},
	{ NAME(L"右へ1ページスクロール"), [](EDIT_SECTION* edit)
	{
		scroll_horiz_per_page(edit, +1.0);
	}
	},
	{ NAME(L"左へ一定量スクロール"), [](EDIT_SECTION* edit)
	{
		scroll_horiz_per_page(edit, -settings.search.page_rate);
	}
	},
	{ NAME(L"右へ一定量スクロール"), [](EDIT_SECTION* edit)
	{
		scroll_horiz_per_page(edit, +settings.search.page_rate);
	}
	},
	{ NAME(L"先頭へスクロール"), [](EDIT_SECTION* edit)
	{
		scroll_horiz_to_start_end(edit, true);
	}
	},
	{ NAME(L"最後へスクロール"), [](EDIT_SECTION* edit)
	{
		scroll_horiz_to_start_end(edit, false);
	}
	},
	{ NAME(L"カーソル位置へスクロール"), &scroll_horiz_to_cursor },
	{ NAME(L"選択オブジェクトへスクロール"), &scroll_to_focused },

	{ NAME(L"上へスクロール"), [](EDIT_SECTION* edit)
	{
		edit->set_display_layer_frame(
			edit->info->display_layer_start - 1,
			edit->info->display_frame_start);
	}
	},
	{ NAME(L"下へスクロール"), [](EDIT_SECTION* edit)
	{
		edit->set_display_layer_frame(
			edit->info->display_layer_start + 1,
			edit->info->display_frame_start);
	}
	},
	{ NAME(L"上へ1ページスクロール"), [](EDIT_SECTION* edit)
	{
		edit->set_display_layer_frame(
			edit->info->display_layer_start - edit->info->display_layer_num,
			edit->info->display_frame_start);
	}
	},
	{ NAME(L"下へ1ページスクロール"), [](EDIT_SECTION* edit)
	{
		edit->set_display_layer_frame(
			edit->info->display_layer_start + edit->info->display_layer_num,
			edit->info->display_frame_start);
	}
	},
	{ NAME(L"最上端へスクロール"), [](EDIT_SECTION* edit)
	{
		scroll_vert_to_top_bottom(edit, true);
	}
	},
	{ NAME(L"最下端へスクロール"), [](EDIT_SECTION* edit)
	{
		scroll_vert_to_top_bottom(edit, false);
	}
	},
	{ NAME(L"選択レイヤーへスクロール"), &scroll_vert_to_selected },

	{ NAME(L"現在フレームのオブジェクトを選択(逆順)"), &focus_cursor_object_rev },
	{ NAME(L"左のオブジェクトを選択"), [](EDIT_SECTION* edit)
	{
		focus_left_right_object(edit, false);
	}
	},
	{ NAME(L"右のオブジェクトを選択"), [](EDIT_SECTION* edit)
	{
		focus_left_right_object(edit, true);
	}
	},
	{ NAME(L"上のオブジェクトを選択"), [](EDIT_SECTION* edit)
	{
		focus_above_below_layer_object(edit, false);
	}
	},
	{ NAME(L"下のオブジェクトを選択"), [](EDIT_SECTION* edit)
	{
		focus_above_below_layer_object(edit, true);
	}
	},
	{ NAME(L"上のレイヤーを選択"), [](EDIT_SECTION* edit)
	{
		edit->set_cursor_layer_frame(edit->info->layer - 1, edit->info->frame);
	}
	},
	{ NAME(L"下のレイヤーを選択"), [](EDIT_SECTION* edit)
	{
		edit->set_cursor_layer_frame(edit->info->layer + 1, edit->info->frame);
	}
	},

	{ NAME(L"左の小節線へ移動(BPM)"), [](EDIT_SECTION* edit)
	{
		move_to_bpm_grid(edit, 1, edit->info->grid_bpm_beat, false);
	}
	},
	{ NAME(L"右の小節線へ移動(BPM)"), [](EDIT_SECTION* edit)
	{
		move_to_bpm_grid(edit, 1, edit->info->grid_bpm_beat, true);
	}
	},
	{ NAME(L"左の拍数線へ移動(BPM)"), [](EDIT_SECTION* edit)
	{
		move_to_bpm_grid(edit, 1, 1, false);
	}
	},
	{ NAME(L"右の拍数線へ移動(BPM)"), [](EDIT_SECTION* edit)
	{
		move_to_bpm_grid(edit, 1, 1, true);
	}
	},
	{ NAME(L"左に1/N拍移動(BPM)"), [](EDIT_SECTION* edit)
	{
		move_to_bpm_grid(edit, settings.search.bpm_grid_div, 1, false);
	}
	},
	{ NAME(L"右に1/N拍移動(BPM)"), [](EDIT_SECTION* edit)
	{
		move_to_bpm_grid(edit, settings.search.bpm_grid_div, 1, true);
	}
	},
	{ NAME(L"左にグリッド基準線を移動(BPM)"), [](EDIT_SECTION* edit)
	{
		shift_bpm_grid_offset(edit, -1);
	}
	},
	{ NAME(L"右にグリッド基準線を移動(BPM)"), [](EDIT_SECTION* edit)
	{
		shift_bpm_grid_offset(edit, +1);
	}
	},
	{ NAME(L"グリッド基準線を現在フレームに(BPM)"), [](EDIT_SECTION* edit)
	{
		Timeline_calc const tl_calc{
			edit->info->rate,
			edit->info->scale
		};
		edit->set_grid_bpm(
			edit->info->grid_bpm_tempo,
			edit->info->grid_bpm_beat,
			static_cast<float>(tl_calc.frame_to_second(edit->info->frame))
		);
	}
	},
	{ NAME(L"最寄りの小節線を現在フレームに(BPM)"), &shift_bpm_grid_nearest_measure_to_cursor },

	// object moving menu items.
	{ NAME(L"左に選択オブジェクトを詰める"), [](EDIT_SECTION* edit)
	{
		move_selected_objects(edit, Direction::Left);
	}
	},
	{ NAME(L"右に選択オブジェクトを詰める"), [](EDIT_SECTION* edit)
	{
		move_selected_objects(edit, Direction::Right);
	}
	},
	{ NAME(L"上に選択オブジェクトを移動"), [](EDIT_SECTION* edit)
	{
		move_selected_objects(edit, Direction::Up);
	}
	},
	{ NAME(L"下に選択オブジェクトを移動"), [](EDIT_SECTION* edit)
	{
		move_selected_objects(edit, Direction::Down);
	}
	},

	// cursor undo menu items.
	{ NAME(L"カーソル位置を元に戻す"), &cursor_undo },
	{ NAME(L"カーソル位置をやり直す"), &cursor_redo },
},
obj_menu_items[] = {
	{ L"左に選択オブジェクトを詰める", [](EDIT_SECTION* edit)
	{
		move_selected_objects(edit, Direction::Left);
	}
	},
	{ L"右に選択オブジェクトを詰める", [](EDIT_SECTION* edit)
	{
		move_selected_objects(edit, Direction::Right);
	}
	},
};
#undef NAME


////////////////////////////////
// DLL main.
////////////////////////////////
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
	{
		dll_hinst = hModule;
		::DisableThreadLibraryCalls(hModule);
		break;
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


////////////////////////////////
// exported functions.
////////////////////////////////

// logging facilities.
extern "C" __declspec(dllexport) void InitializeLogger(LOG_HANDLE* handle)
{
	logger = handle;
}

// init (check version).
extern "C" __declspec(dllexport) bool InitializePlugin(DWORD version)
{
	if (version >= least_ver_num) return true;

	logger->error(logger, L"Requires AviUtl ExEdit2 " LEAST_VER_STR L" or newer!");
	::MessageBoxW(nullptr,
		PLUGIN_NAME L" が動作するには AviUtl ExEdit2 " LEAST_VER_STR L" 以上が必要です！",
		PLUGIN_NAME, MB_OK | MB_ICONERROR);
	return false;
}

// register.
#define PLUGIN_INFO_FMT(name, ver, author)	(name " " ver " by " author)
#define PLUGIN_INFO		PLUGIN_INFO_FMT(PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR)
extern "C" __declspec(dllexport) void RegisterPlugin(HOST_APP_TABLE* host)
{
	// load settings from .ini.
	settings.load();
	cursor_undo_queue = { static_cast<size_t>(settings.cursor_undo.queue_size) };
	cursor_undo_queue.clear(0);

	// プラグインの情報を設定
	host->set_plugin_information(PLUGIN_INFO);

	// 編集ハンドルを作成
	edit_handle = host->create_edit_handle();

	// create and register plugin window.
	plugin_window.create_register_window(host, PLUGIN_NAME);

	// register menu items.
	for (auto const& item : edit_menu_items)
		host->register_edit_menu(item.name, item.callback);
	for (auto const& item : obj_menu_items)
		host->register_object_menu(item.name, item.callback);

	// register event callbacks.
	host->register_project_load_handler(&on_load_project);
}
