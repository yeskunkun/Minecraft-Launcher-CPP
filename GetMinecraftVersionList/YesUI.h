#include <Windows.h>
#include <dwmapi.h>
#include <CommCtrl.h>
#include <uxtheme.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <thread>
//#include "Downloader.h"
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "UxTheme.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
using namespace std;
class _WindowClass {
public:
	HWND _hwnd = 0;
	WNDPROC _proc;
	HINSTANCE _inst = 0;
	RECT _rc{};
	WNDCLASSEXW _wc{};
	bool registered = false;
	DWORD _create(LPCWSTR title, int w, int h) {
		_inst = GetModuleHandleW(0);
		ZeroMemory(&_wc, sizeof(WNDCLASSEXW));
		_wc.cbSize = sizeof(WNDCLASSEXW);
		_wc.hInstance = _inst;
		_wc.lpfnWndProc = DefWindowProcW;
		_wc.lpszClassName = L"WindowClass";
		_wc.hCursor = LoadCursorW(0, IDC_ARROW);
		if (!registered) {
			if (!RegisterClassExW(&_wc)) {
				return GetLastError();
			}
			registered = true;
		}
		_rc = { 0, 0, w, h };
		DWORD dwExStyle = WS_EX_DLGMODALFRAME;
		DWORD dwStyle = WS_OVERLAPPEDWINDOW;
		AdjustWindowRectEx(&_rc, dwStyle, FALSE, dwExStyle);
		_hwnd = CreateWindowExW(dwExStyle, _wc.lpszClassName, title, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT,
			_rc.right - _rc.left, _rc.bottom - _rc.top, 0, 0, _inst, 0);
		return GetLastError();
	}
	void Destroy() {
		if (_hwnd) {
			DestroyWindow(_hwnd);
			_hwnd = 0;
		}
		if (registered) {
			UnregisterClassW(_wc.lpszClassName, _inst);
			registered = false;
		}
	}
};

// Define static member
class Control {
public:
	HWND _hwnd, _hwndParent;
	DWORD place(int x, int y, int w, int h) {
		return SetWindowPos(_hwnd, HWND_TOP, x, y, w, h, SWP_SHOWWINDOW);
	}
};
HBRUSH hBrush = CreateSolidBrush(RGB(240, 240, 240));
COLORREF bgColor, textColor, separatorColor, hoverColor, disabledColor, borderColor, darkBorderColor, DarkModeBGR;
int isDarkThemeEnabled = false, DarkColor;
typedef int(__stdcall* SetPreferredAppModeProc)(int);
SetPreferredAppModeProc SetPreferredAppMode = (SetPreferredAppModeProc)GetProcAddress(LoadLibraryExW(L"uxtheme.dll", 0, 2048), MAKEINTRESOURCEA(135));
typedef NTSTATUS(WINAPI* PRtlGetVersion)(PRTL_OSVERSIONINFOEXW);
PRtlGetVersion RtlGetVersion = (PRtlGetVersion)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");
RTL_OSVERSIONINFOEXW os = { sizeof(RTL_OSVERSIONINFOEXW) };
int style_ = 0, havehandle = 0, lastanswer = 0;
wstring message_, title_;
WNDPROC originalProc;
HHOOK hhook;
HWND hwnd_;
extern "C" {
	int WINAPI MessageBoxTimeoutA(IN HWND hWnd, IN LPCSTR lpText, IN LPCSTR lpCaption, IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);
	int WINAPI MessageBoxTimeoutW(IN HWND hWnd, IN LPCWSTR lpText, IN LPCWSTR lpCaption, IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);
};
bool _allowdark, _allowdrag;
MSG msg;
WNDPROC oldproc;
LRESULT HandleWindow(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, WNDPROC oldproc = NULL) {
	switch (msg) {
	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORMSGBOX:
	case WM_CTLCOLORSCROLLBAR:
	case WM_CTLCOLORBTN: {
		SetTextColor((HDC)wparam, isDarkThemeEnabled ? RGB(240, 240, 240) : RGB(0, 0, 0));
		SetBkColor((HDC)wparam, isDarkThemeEnabled ? DarkColor : RGB(240, 240, 240));
		return (INT_PTR)CreateSolidBrush(isDarkThemeEnabled ? DarkColor : RGB(240, 240, 240));
	}
	case WM_LBUTTONDOWN: {
		ReleaseCapture();
		SendMessageW(hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);
		break;
	}
	case WM_ERASEBKGND: {
		RECT rect;
		GetClientRect(hwnd, &rect);
		FillRect(HDC(wparam), &rect, CreateSolidBrush(isDarkThemeEnabled ? DarkColor : RGB(240, 240, 240)));
		return 1;
	}
	case WM_SETTINGCHANGE: {
		HKEY hKey;
		DWORD valueSize = sizeof(DWORD);
		RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey);
		LONG result = RegGetValueW(hKey, 0, L"AppsUseLightTheme", RRF_RT_REG_DWORD, 0, &isDarkThemeEnabled, &valueSize);
		RegCloseKey(hKey);
		if (result == ERROR_SUCCESS) {
			isDarkThemeEnabled = (!isDarkThemeEnabled);
		}
		EnumChildWindows(hwnd, WNDENUMPROC([](HWND hWnd, LPARAM lparam) -> BOOL {
			if (isDarkThemeEnabled) {
				wchar_t className[520];
				GetClassNameW(hWnd, className, sizeof(className));
				if (wcscmp(className, L"ComboBox") == 0) {
					SetWindowTheme(hWnd, L"DarkMode_CFD", NULL);
				}
				else {
					SetWindowTheme(hWnd, L"DarkMode_Explorer", NULL);
				}
			}
			else {
				SetWindowTheme(hWnd, L"Explorer", NULL);
			}
			return true; }), 0);
		RECT rect;
		GetClientRect(hwnd, &rect);
		MARGINS margins = { -1 };
		DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &isDarkThemeEnabled, sizeof(int));
		if (isDarkThemeEnabled) {
			hBrush = CreateSolidBrush(RGB(32, 32, 32));
			DarkModeBGR = DarkColor;
			//DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &DarkColor, sizeof(int));
			bgColor = DarkModeBGR;
			textColor = RGB(222, 222, 222);
			separatorColor = RGB(70, 70, 70);
			hoverColor = RGB(62, 62, 64);
			disabledColor = RGB(100, 100, 100);
			borderColor = RGB(80, 80, 80);
			darkBorderColor = RGB(40, 40, 40);
		}
		else {
			hBrush = CreateSolidBrush(RGB(240, 240, 240));
			DarkModeBGR = 0xF0F0F0;
			//DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &DarkModeBGR, sizeof(int));
			bgColor = RGB(240, 240, 240);
			textColor = RGB(0, 0, 0);
			separatorColor = RGB(222, 222, 222);
			hoverColor = RGB(200, 222, 255);
			disabledColor = RGB(150, 150, 150);
			borderColor = RGB(255, 255, 255);
			darkBorderColor = RGB(225, 225, 225);
		}
		if (isDarkThemeEnabled) {
			if ((os.dwBuildNumber < 18362) && (os.dwBuildNumber >= 17763)) {
				SetPropW(hwnd, L"UseImmersiveDarkModeColors", HANDLE(isDarkThemeEnabled));
			}
			else if ((os.dwBuildNumber >= 18362) && (os.dwBuildNumber < 19041)) {
				DwmSetWindowAttribute(hwnd, 19, &isDarkThemeEnabled, sizeof(isDarkThemeEnabled));
			}
			else if (os.dwBuildNumber >= 19041) {
				DwmSetWindowAttribute(hwnd, 20, &isDarkThemeEnabled, sizeof(isDarkThemeEnabled));
			}
			FillRect(GetDC(hwnd), &rect, CreateSolidBrush(isDarkThemeEnabled ? DarkColor : RGB(240, 240, 240)));
		}
		else {
			margins = { 0 };
			DwmSetWindowAttribute(hwnd, 20, &isDarkThemeEnabled, sizeof(isDarkThemeEnabled));
			FillRect(GetDC(hwnd), &rect, CreateSolidBrush(isDarkThemeEnabled ? DarkColor : RGB(240, 240, 240)));
		}
		DwmExtendFrameIntoClientArea(hwnd, &margins);
		DwmSetWindowAttribute(hwnd, 1029, &isDarkThemeEnabled, sizeof(isDarkThemeEnabled));
		enum class SystemBackdropType { Auto, None, Mica, Acrylic, Tabbed } value = SystemBackdropType::Mica;
		DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &value, sizeof(value));
		break;
	}
	}
	if (oldproc) {
		return CallWindowProcW(oldproc, hwnd, msg, wparam, lparam);
	}
	return DefWindowProcW(hwnd, msg, wparam, lparam);
}
class Font {
public:
	HFONT _hFont;
	Font(LPCWSTR family, int size, bool bold = false, bool italic = false) {
		_hFont = CreateFontW(size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, italic, 0, 0, 1, 0, 0, 0, 0, family);
	}
	Font() {}
};
string GetComboBoxText(HWND hCombo) {
	int len = GetWindowTextLengthA(hCombo);
	string text(len + 1, L'\0');
	GetWindowTextA(hCombo, &text[0], len + 1);
	text.resize(len);
	return text;
}
string replace_all(string str, const string& old_sub, const string& new_sub) {
	size_t pos = 0;
	while ((pos = str.find(old_sub, pos)) != -1) {
		str.replace(pos, old_sub.length(), new_sub);
		pos += new_sub.length();
	}
	return str;
}
string FindKeyByValue(const map<string, string>& m, const string& value) {
	printf("[FindKeyByValue] 准备查找%s\n", value);
	for (const auto& pair : m) {
		printf("[FindKeyByValue] |-first=%s,second=%s\n", pair.first.c_str(), pair.second.c_str());
		if (pair.second == value) {
			return pair.first;  // 返回对应的 key
		}
	}
	printf("[Java] 未找到Java通过%s\n", value);
	return "java.exe";  // 未找到
}
class Window {
private:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		switch (msg) {
		case WM_COMMAND: {
			ULONG cmdId = LOWORD(wparam);
			auto it = _buttonCallbacks.find(cmdId);
			if (it != _buttonCallbacks.end() && it->second) {
				try {
					it->second();
				}
				catch (...) {
					throw;
				}
			}
            if (LOWORD(wparam) == 0xEEEE && HIWORD(wparam) == CBN_DROPDOWN) {
				COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };
				if (GetComboBoxInfo(GetDlgItem(hwnd,0xEEEE), &info)) {
					HWND hComboLBox = info.hwndList; // 这就是你的目标窗口句柄[citation:2]
					printf("[组合框] 深色模式初始化，ComboLBox窗口句柄：%d\n",int(hComboLBox));
					if (hComboLBox) {
						thread([hComboLBox]() {
							Sleep(100);
							HWND hLBox = FindWindowExW(0, 0, L"ComboLBox", 0);
							EnumChildWindows(hLBox, WNDENUMPROC([](HWND hWnd, LPARAM lparam) -> BOOL {
								if (isDarkThemeEnabled) {
									wchar_t className[520];
									GetClassNameW(hWnd, className, sizeof(className));
									printf("[组合框] 遍历子窗口，类名：%s\n", className);
									SetWindowTheme(hWnd, L"DarkMode_Explorer", NULL);
								}
								else {
									SetWindowTheme(hWnd, L"Explorer", NULL);
								}
								return true; }), 0); }).detach();
					}
				}
			}
			else if (LOWORD(wparam) == 0xEEEE && HIWORD(wparam) == CBN_SELCHANGE) {
				thread([hwnd]() {
					Sleep(1);
					string buf = GetComboBoxText(GetDlgItem(hwnd, 0xEEEE));
					printf("[组合框] 选中版本：%s，需要Java：%s\n", buf, GetMinecraftRequireJava(buf));
					}).detach();
			}
			else if (LOWORD(wparam) == 16 && HIWORD(wparam) == CBN_SELCHANGE) {
				thread([hwnd]() {
					Sleep(1);
					string buf = replace_all(GetComboBoxText(hListView2),"Java ","");
					printf("[组合框] 选中Java版本：%s\n", buf);
					string path = FindKeyByValue(GetJavaVersions(),buf);
					if (path.empty()) {
						printf("[组合框] Java选择失败因为找不到路径！");
					}
					else {
						strcpy_s(JAVA_PATH, MAX_PATH, path.c_str());
						printf("[组合框] path=%s\n", JAVA_PATH);
						JAVA_VERSION = buf;
					}
					}).detach();
			}
			if (wparam >= 4001 && wparam <= 4003) {
				thread([hwnd, msg, wparam, lparam]() {
					if (IsWindow(hProgress)) {
						DestroyWindow(hProgress);
					}
					hProgress = CreateWindowExW(0, PROGRESS_CLASS, 0, WS_VISIBLE | WS_CHILD, 25, 281, 550, 30, hwnd, (HMENU)17, 0, 0);
					HWND hbtn = HWND(lparam);
					EnableWindow(hbtn, false);
					EnableWindow(hStart, false);
					InvalidateRect(hwnd, 0, true);
					char path[MAX_PATH]{};
					GetModuleFileNameA(0, path, MAX_PATH);
					string fullPath(path);
					size_t pos = fullPath.find_last_of("\\");
					if (pos != -1) {
						string directory = fullPath.substr(0, pos);
						SetCurrentDirectoryA(directory.c_str()) != 0;
					}
					SendMessageW(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
					SendMessageW(hProgress, PBM_SETSTEP, 1, 0);
					SendMessageW(hProgress, PBM_SETBARCOLOR, 0, RGB(0, 120, 215));
					SendMessageW(hProgress, PBM_SETPOS, 0, 0);
					printf("[Java] 准备下载，msg=%d. . . \n", wparam);
					switch (wparam) {
					case 4001: {
						InstallJava("8", hbtn);
						break;
					}
					case 4002: {
						InstallJava("17", hbtn);
						break;
					}
					case 4003: {
						InstallJava("25", hbtn);
						break;
					}
					}
					EnableWindow(hbtn, true);
					EnableWindow(hStart, true);
					SendMessageW(hProgress, WM_SYSCOMMAND, SC_CLOSE, 0);
					DestroyWindow(hProgress);
					InvalidateRect(hwnd, 0, true);
					///////////////////////////////////////////////////////////
					auto a = GetJavaVersions();
					printf("[Java] 共有%d个\n", a.size());
					if (a.size() <= 0) {
						EnableWindow(hListView2, true);
						return;
					}
					bool j = true;
					for (auto& i : a) {
						printf("[Java] 检测版本：%s\n", i.second);
						SendMessageA(hListView2, CB_ADDSTRING, 0, LPARAM(("Java " + i.second).c_str()));
						if (j) {
							SendMessageA(hListView2, CB_SELECTSTRING, 0, LPARAM(("Java " + i.second).c_str()));
						}
						j = false;
					}
					string buf = replace_all(GetComboBoxText(hListView2), "Java ", "");
					printf("[组合框] 选中Java版本：%s\n", buf);
					string _path = FindKeyByValue(GetJavaVersions(), buf);
					if (_path.empty()) {
						printf("[组合框] Java选择失败因为找不到路径！");
					}
					else {
						strcpy_s(JAVA_PATH, MAX_PATH, _path.c_str());
						printf("[组合框] path=%s\n", JAVA_PATH);
						JAVA_VERSION = buf;
					}
					EnableWindow(hListView2, true);
					}).detach();
			}
			break;
		}
		}
		return HandleWindow(hwnd, msg, wparam, lparam);
	}
	wstring Lower(LPCWSTR src) {
		wstring dst = src;
		LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, src, wcslen(src), dst.data(), dst.size());
		return dst;
	}
	bool Equal(LPCWSTR a, LPCWSTR b) {
		return CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, a, wcslen(a), b, wcslen(b)) == CSTR_EQUAL;
	}
public:
	_WindowClass _cls;
	int _cmdindex = 0x1000;
	static unordered_map<ULONG, function<void()>> _buttonCallbacks;
	Window() {
		init();
	}
	void init(bool allowdark = false, bool allowdrag = false) {
		_allowdark = allowdark, _allowdrag = allowdrag;
		RtlGetVersion(&os);
		if (os.dwBuildNumber >= 17763) {
			SetPreferredAppMode(1);
		}
		DWORD err = _cls._create(L"UI", 200, 200);
		SetWindowLongPtrW(_cls._hwnd, -4, LONG_PTR(WndProc));
		SendMessageW(_cls._hwnd, WM_SETTINGCHANGE, 0, 0);
		GetWindowRect(_cls._hwnd, &_cls._rc);
	}
	void enable_size(bool enable) {
		if (enable) {
			SetWindowLongPtrW(_cls._hwnd, -16, GetWindowLongPtrW(_cls._hwnd, -16) & ~WS_MINIMIZEBOX & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME);
		}
		else {
			SetWindowLongPtrW(_cls._hwnd, -16, GetWindowLongPtrW(_cls._hwnd, -16) |WS_MINIMIZEBOX |WS_MAXIMIZEBOX |WS_THICKFRAME);
		}
	}
	void bindkey(int key, function<void()> callback) {
		_buttonCallbacks[key] = callback;
	}
	int msgbox(LPCWSTR message, LPCWSTR title, DWORD style, DWORD timeout = 0) {
		RtlGetVersion(&os);
		lastanswer = 0;
		message_ = (message == NULL ? L"错误" : message);
		title_ = title; style_ = style; hwnd_ = _cls._hwnd;
		havehandle = 0;
		CreateThread(0, 0, [](LPVOID lpParam) -> unsigned long {
			hhook = SetWindowsHookExW(5, [](int nCode, WPARAM wParam, LPARAM lParam) {
				if (nCode == HCBT_CREATEWND || nCode == HCBT_ACTIVATE) {
					havehandle = (int)wParam;
					ShowWindow(HWND(wParam), SW_HIDE);
					UnhookWindowsHookEx(hhook);
				}
				return CallNextHookEx(hhook, nCode, wParam, lParam); }, 0, GetCurrentThreadId());
			lastanswer = MessageBoxExW(hwnd_, message_.c_str(), title_.c_str(), style_ | MB_SETFOREGROUND, MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED));
			return true; }, 0, 0, 0);
		while (!havehandle) {
			GetMessageW(&msg, 0, 0, 0);
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		HWND hmsg = HWND(havehandle);
		oldproc = (WNDPROC)SetWindowLongPtrW(hmsg, -4, LONG_PTR(WNDPROC([](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
			if (msg == WM_PAINT) {
				return DefWindowProcW(hwnd, msg, wparam, lparam);
			}
			return HandleWindow(hwnd, msg, wparam, lparam, oldproc);
			})));
		InvalidateRect(hmsg, 0, true);
		EnumChildWindows(hmsg, WNDENUMPROC([](HWND hWnd, LPARAM lparam) -> BOOL {
			if (isDarkThemeEnabled) {
				SetWindowTheme(hWnd, L"DarkMode_Explorer", NULL);
			}
			else {
				SetWindowTheme(hWnd, L"Explorer", NULL);
			}
			return true; }), 0);
		SendMessageW(hmsg, WM_SETTINGCHANGE, 0, 0);
		while (!lastanswer) {
			GetMessageW(&msg, 0, 0, 0);
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		cout << "[MSGBOX] lastanswer = " << lastanswer << endl;
		return lastanswer;
	}
	void update() {
		SendMessageW(_cls._hwnd, WM_SETTINGCHANGE, 0, 0);
		UpdateWindow(_cls._hwnd);
		show();
	}
	void destroy() {
		_cls.Destroy();
	}
	void title(LPCWSTR _title) {
		SetWindowTextW(_cls._hwnd, _title);
	}
	void size(int w, int h) {
		RECT rect = { 0, 0, w, h };
		AdjustWindowRectEx(&rect, GetWindowLongPtrW(_cls._hwnd, -16), FALSE, GetWindowLongPtrW(_cls._hwnd, -20));      // 扩展样式
		SetWindowPos(_cls._hwnd, HWND_TOP, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);
		GetWindowRect(_cls._hwnd, &_cls._rc);
	}
	void move(int x, int y) {
		SetWindowPos(_cls._hwnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE);
	}
	void show() {
		ShowWindow(_cls._hwnd, SW_SHOW);
	}
	void hide() {
		ShowWindow(_cls._hwnd, SW_HIDE);
	}
	DWORD mainloop() {
		update();
		while (GetMessageW(&msg, 0, 0, 0)) {
			if (msg.message == WM_KEYDOWN) {
				if (msg.wParam >= 0x01 && msg.wParam <= 0xFE) {
					auto it = _buttonCallbacks.find(msg.wParam);
					if (it != _buttonCallbacks.end() && it->second) {
						try {
							it->second();
						}
						catch (...) {
							throw;
						}
					}
				}
			}
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
			if (!IsWindow(_cls._hwnd)) {
				break;
			}
		}
		return msg.wParam;
	}
	Control create(LPCWSTR lpClassName, LPCWSTR title) {
		Control self;
		self._hwndParent = _cls._hwnd;
		DWORD style, styleex;
		if (Equal(lpClassName, L"Static")) {
			style = WS_BORDER | ES_LEFT;
		}
		self._hwnd = CreateWindowExW(0, lpClassName, title, WS_CHILD | WS_VISIBLE,
			0, 0, 0, 0, self._hwndParent, 0, _cls._inst, 0);
		return self;
	}
};
unordered_map<ULONG, function<void()>> Window::_buttonCallbacks;
template<typename Derived>
class ControlBase {
public:
	int OffsetX = 22;
	int OffsetY = 10;
	HWND _hwnd = nullptr;
	Window _Parent;
	bool _wrap = false;
	LPCWSTR _title = nullptr;
	Font _font;
	void destroy() {
		if (_hwnd) {
			DestroyWindow(_hwnd);
			_hwnd = nullptr;
		}
	}
	void settext(LPCWSTR text) {
		SetWindowTextW(_hwnd, text);
	}
	void disable(bool disabled) {
		EnableWindow(_hwnd, !disabled);
	}
	void show() {
		ShowWindow(_hwnd, SW_SHOW);
	}
	void hide() {
		ShowWindow(_hwnd, SW_HIDE);
	}
	void place(int x, int y, int w, int h) {
		SetWindowPos(_hwnd, HWND_TOP, x, y, w, h, SWP_NOZORDER);
	}
	void place(int x, int y) {
		SIZE size;
		HDC hdc = GetDC(_Parent._cls._hwnd);
		HFONT hOldFont = (HFONT)SelectObject(hdc, _font._hFont);
		if (GetTextExtentPoint32W(hdc, _title, wcslen(_title), &size)) {
			SetWindowPos(_hwnd, HWND_TOP, x, y,
				size.cx + OffsetX,
				size.cy + OffsetY,
				SWP_NOZORDER);
		}
		SelectObject(hdc, hOldFont);
		ReleaseDC(_Parent._cls._hwnd, hdc);
	}
};
class Static : public ControlBase<Static> {
public:
	Static() = default;
	// the anchor argument can be "left", "center", or "right"
	Static(Window& window, LPCWSTR title, Font font, bool wrap = false, LPCSTR anchor = "center") {
		OffsetX = 0;
		OffsetY = 0;
		_Parent = window;
		_title = title;
		_font = font;
		DWORD dwStyle = 0;
		if (anchor == "left") {
			dwStyle = SS_LEFT;
		} else if (anchor == "center") {
			dwStyle = SS_CENTER;
		} else if (anchor = "right") {
			dwStyle = SS_RIGHT;
		}
		_hwnd = CreateWindowExW(0, L"Static", title, WS_CHILD | WS_VISIBLE | dwStyle,
			0, 0, 0, 0, _Parent._cls._hwnd, (HMENU)(INT_PTR)++window._cmdindex, window._cls._inst, 0);
		SendMessageW(_hwnd, WM_SETFONT, WPARAM(font._hFont), true);
	}
};
class Button : public ControlBase<Button> {
public:
	Button() = default;
	Button(Window& window, LPCWSTR title, Font font, function<void()> callback = NULL, bool _default = false) {
		OffsetX = 12;
		OffsetY = 8;
		_Parent = window;
		_title = title;
		_font = font;
		_hwnd = CreateWindowExW(0, L"Button", title, WS_CHILD | WS_VISIBLE | BS_CENTER | (_default?BS_DEFPUSHBUTTON:BS_PUSHBUTTON),
			0, 0, 0, 0, _Parent._cls._hwnd, (HMENU)(INT_PTR)++window._cmdindex, window._cls._inst, 0);
		SendMessageW(_hwnd, WM_SETFONT, WPARAM(font._hFont), true);
		window._buttonCallbacks[window._cmdindex] = callback;
		if (_default) {
			SetFocus(_hwnd);
		}
	}
	void invoke() {
		_Parent._buttonCallbacks[_Parent._cmdindex]();
	}
};
class Checkbox : public ControlBase<Checkbox> {
public:
	int OffsetX = 22;
	int OffsetY = 10;
	Checkbox() = default;
	Checkbox(Window& window, LPCWSTR title, Font font, function<void()> callbackwhencheck = NULL) {
		_Parent = window;
		_title = title;
		_font = font;
		_hwnd = CreateWindowExW(0, L"Button", title, WS_CHILD | WS_VISIBLE | BS_LEFT | BS_AUTOCHECKBOX,
			0, 0, 0, 0, _Parent._cls._hwnd, (HMENU)(INT_PTR)++window._cmdindex, window._cls._inst, 0);
		SendMessageW(_hwnd, WM_SETFONT, WPARAM(font._hFont), true);
		window._buttonCallbacks[window._cmdindex] = callbackwhencheck;
		//cout << window._cmdindex << '\n';
	}
	bool get() {
		return SendMessageW(_hwnd, BM_GETCHECK, 0, 0);
	}
};
class Edit : public ControlBase<Edit> {
public:
	Edit() = default;
	Edit(Window& window, LPCWSTR title, Font font, bool numbersonly = false,
		bool multiline = false, bool autowrap = false) {
		OffsetX = 150;
		OffsetY = 6;
		_Parent = window;
		_title = title;
		_font = font;
		_hwnd = CreateWindowExW(0, L"Edit", title, WS_BORDER | WS_CHILD | WS_VISIBLE | (numbersonly ? ES_NUMBER : 0) |
			(multiline ? (ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL) : 0) | (autowrap ? 0 : (ES_AUTOHSCROLL)),
			0, 0, 0, 0, _Parent._cls._hwnd, (HMENU)(INT_PTR)++window._cmdindex, window._cls._inst, 0);
		SendMessageW(_hwnd, WM_SETFONT, WPARAM(font._hFont), true);
	}
	LPCWSTR get() {
		int len = GetWindowTextLengthW(_hwnd);
		wchar_t* buffer = new wchar_t[len + 1];
		GetWindowTextW(_hwnd, buffer, len + 1);
		return buffer;
	}
};