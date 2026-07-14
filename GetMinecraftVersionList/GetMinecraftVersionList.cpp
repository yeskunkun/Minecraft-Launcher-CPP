#include <Windows.h>
#include <WinHttp.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cctype>
#include <cstdio>
#include <Shlwapi.h>
#include <iomanip>
#include <thread>
#include <atomic>
#include <mutex>
#include <fstream>
#include <chrono>
#include <shlobj.h>
#include <map>
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
using namespace std;
char VERSION[MAX_PATH] = "26.2";
LPCSTR GAME_DIR_REL = ".minecraft";
LPCSTR MANIFEST_URL = "https://launchermeta.mojang.com/mc/game/version_manifest.json";
char JAVA_PATH[MAX_PATH] = "";
string JAVA_VERSION = "";
map<string, string> JAVA_USERSELECTED;
HWND hwnd, hListView, hProgress,hStart, hListView2;
#include "Extract.h"
#include "GetScript.h"
#include "Downloader.h"
#include "Java.h"
#include "YesUI.h"
/*
	DownloadNew(VERSION);
	StartGame(VERSION);
*/
struct Version { string id, type;};
bool IsItemExistsIgnoreCase(HWND hCombo, const string& text) {
	// 获取所有项目数量
	int count = SendMessageW(hCombo, CB_GETCOUNT, 0, 0);

	for (int i = 0; i < count; i++) {
		// 获取每个项目的文本
		int len = SendMessageA(hCombo, CB_GETLBTEXTLEN, i, 0);
		if (len == CB_ERR) continue;

		string item(len + 1, L'\0');
		SendMessageA(hCombo, CB_GETLBTEXT, i, (LPARAM)&item[0]);
		item.resize(len);

		// 不区分大小写比较
		if (_stricmp(item.c_str(), text.c_str()) == 0) {
			return true;
		}
	}
	return false;
}
int main() {
	thread([&]() {
		string raw = HttpGet(MANIFEST_URL);
		json data = json::parse(raw);
		while (!IsWindow(hListView)) {
			Sleep(1);
		}
		if (raw.empty()) {
			SetWindowTextW(hListView, L"无法获取版本。");
			EnableWindow(hListView, false);
		} else {
			SetWindowTextA(hListView, string(data["latest"]["release"]).c_str());
			auto& versions_array = data["versions"];
			vector<Version> versions;
			for (auto it = versions_array.begin(); it != versions_array.end(); ++it) {
				Version v;
				v.id = (*it)["id"];
				v.type = (*it)["type"];
				//printf("[版本加载] 导入%s. . . \n", v);
				if (v.type == string("release")) {
					versions.push_back(v);
					//printf("[版本加载] 导入%s. . . \n", v);
					SendMessageA(hListView, CB_ADDSTRING, 0, LPARAM(string(v.id).c_str()));
				}
			}
			EnableWindow(hListView, true);
		}
		}).detach();
	thread([]() {
		auto a = GetJavaVersions();
		printf("[Java] 共有%d个\n", a.size());
		if (a.size() <= 0) {
			EnableWindow(hListView2, true);
			return;
		}
		bool j = true;
		for (auto& i : a) {
			printf("[Java] 检测版本：%s\n", i.second);
			SendMessageA(hListView2, CB_ADDSTRING, 0, LPARAM(("Java "+i.second).c_str()));
			if (j) {
				SendMessageA(hListView2, CB_SELECTSTRING, 0, LPARAM(("Java " + i.second).c_str()));
			}
			j = false;
		}
		string buf = replace_all(GetComboBoxText(hListView2), "Java ", "");
		printf("[组合框] 选中Java版本：%s\n", buf);
		string path = FindKeyByValue(GetJavaVersions(), buf);
		if (path.empty()) {
			printf("[组合框] Java选择失败因为找不到路径！");
		}
		else {
			strcpy_s(JAVA_PATH, MAX_PATH, path.c_str());
			printf("[组合框] path=%s\n", JAVA_PATH);
			JAVA_VERSION = buf;
		}
		EnableWindow(hListView2, true);
		}
	).detach();
	auto window = Window();
	hwnd = window._cls._hwnd;
	window.title(L"Minecraft Launcher by Yeskunkun");
	window.size(600, 400);
	window.enable_size(false);

	HWND tmph = CreateWindowExW(0, L"Static", L"Minecraft Launcher", WS_CHILD | WS_VISIBLE | SS_CENTER, 137, 3, 327, 40, hwnd, 0, 0, 0);
	SendMessageW(tmph, WM_SETFONT, WPARAM(CreateFontA(40, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft Yahei UI")), 1);
	tmph = CreateWindowExW(0, L"Static", L"Made by Yeskunkun!", WS_CHILD | WS_VISIBLE | SS_CENTER, 137, 43, 327, 32, hwnd, 0, 0, 0);
	SendMessageW(tmph, WM_SETFONT, WPARAM(CreateFontA(32, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft Yahei UI")), 1);
	hListView2 = CreateWindowExW(0, L"Combobox", 0, WS_DISABLED | WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL, 156, 84, 260, 34, hwnd, HMENU(16), 0, 0);
	SendMessageW(hListView2, WM_SETFONT, WPARAM(CreateFontA(28, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft Yahei UI")), 1);

	tmph = CreateWindowExW(0, L"Static", L"Java路径：", WS_CHILD | WS_VISIBLE | SS_LEFT, 16, 82, 130, 32, hwnd, 0,0, 0);
	SendMessageW(tmph, WM_SETFONT, WPARAM(CreateFontA(32, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, LPCSTR("微软雅黑"))), 1);
	Button bbtn;
	bbtn = Button(window, L"浏览", Font(L"Microsoft Yahei UI", 27), [&]() {
		OPENFILENAMEA ofn{};
		char fileName[MAX_PATH]{};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = hwnd;
		ofn.lpstrFilter = "Java.exe 可执行文件\0java.exe\0";
		ofn.nFilterIndex = 1;
		ofn.lpstrFile = fileName;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		if (GetOpenFileNameA(&ofn)) {
			SetWindowTextA(GetDlgItem(hwnd, 16), fileName);
			string ver = GetJavaMajorVersionStr(fileName);
			if (ver.empty()) {
				if (window.msgbox(L"版本错误，请重新选择", L"警告", 49) == IDOK) {
					bbtn.invoke();
				}
			}
			else {
				int index = SendMessageW(hListView2, CB_FINDSTRINGEXACT, -1, LPARAM(("Java " + ver).c_str()));
				if (!IsItemExistsIgnoreCase(hListView2, "Java " + ver)) {
					SendMessageA(hListView2, CB_ADDSTRING, 0, LPARAM(("Java " + ver).c_str()));
					SendMessageA(hListView2, CB_SELECTSTRING, 0, LPARAM(("Java " + ver).c_str()));
					map<string, string> a = { { fileName, ver } };
					JAVA_USERSELECTED.insert(a.begin(), a.end());
					printf("[JVM] Java 选择完成，版本号是：%s，index=%d\n", ver, index);
				}
			}
		}
		}, false);
	bbtn.place(502, 83, 75, 38);
	Button dbtn;
	dbtn = Button(window, L"下载", Font(L"Microsoft Yahei UI", 27), [dbtn]() {
		HMENU hMenu = CreatePopupMenu();
		AppendMenuW(hMenu, MF_STRING, 4001, L"Java 8");
		AppendMenuW(hMenu, MF_SEPARATOR, 0, 0);
		AppendMenuW(hMenu, MF_STRING, 4002, L"Java 17");
		AppendMenuW(hMenu, MF_STRING, 4003, L"Java 25");
		AppendMenuW(hMenu, MF_SEPARATOR, 0, 0);
		AppendMenuW(hMenu, MF_STRING, 4004, L"访问 Java 8 官方网站");
		AppendMenuW(hMenu, MF_STRING, 4004, L"访问 JDK 官方网站");
		// 获取鼠标当前位置
		POINT pt;
		GetCursorPos(&pt);
		printf("[菜单] X=%d, Y=%d\n", pt.x, pt.y);
		TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, pt.x+4,pt.y+4,0,hwnd,0);
		}, false);
	dbtn.place(420, 83, 75, 38);
	tmph = CreateWindowExW(0, L"Static", L"游戏版本：", WS_CHILD | WS_VISIBLE | SS_LEFT, 16, 126, 130, 39, hwnd, 0, 0, 0);
	SendMessageW(tmph, WM_SETFONT, WPARAM(CreateFontA(32, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft Yahei UI")), 1);

	tmph = CreateWindowExW(0, L"Static", L"用户名：", WS_CHILD | WS_VISIBLE | SS_LEFT, 16, 172, 117, 32, hwnd, HMENU(2), 0, 0);
	SendMessageW(tmph, WM_SETFONT, WPARAM(CreateFontA(32, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft Yahei UI")), 1);
	tmph = CreateWindowExW(0, L"Edit", L"LocalPlayer", WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | WS_BORDER, 128, 173, 360, 36, hwnd, HMENU(0xEEEB), 0, 0);
	SendMessageW(tmph, WM_SETFONT, WPARAM(CreateFontA(28, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft Yahei UI")), 1);
	Button(window, L"随机", Font(L"Microsoft Yahei UI", 27), [&]() {
		LARGE_INTEGER tick;
		QueryPerformanceCounter(&tick);
		unsigned int seed = static_cast<unsigned int>(tick.LowPart ^ (tick.HighPart << 16));
		srand(seed);
		ostringstream oss;
		oss << "Player"<< setw(3) << setfill('0') <<(rand() % 999) + 1;
		SetWindowTextA(GetDlgItem(hwnd, 0xEEEB), oss.str().c_str());
		}).place(502, 172, 75, 38);
	Button sbtn = Button(window, L"开始游戏", Font(L"Microsoft Yahei UI", 31), [&]() {
		if (hProgress) {
			SendMessageW(hProgress, WM_SYSCOMMAND, SC_CLOSE, 0);
		}
		hProgress = CreateWindowExW(0, PROGRESS_CLASS, 0, WS_VISIBLE | WS_CHILD | PBS_SMOOTH, 25, 281, 550, 30, hwnd, (HMENU)17, 0, 0);
		EnableWindow(sbtn._hwnd, false);
		thread([sbtn]() {
			char path[MAX_PATH]{};
			GetModuleFileNameA(0, path, MAX_PATH);
			string fullPath(path);
			size_t pos = fullPath.find_last_of("\\");
			if (pos != -1) {
				string directory = fullPath.substr(0, pos);
				SetCurrentDirectoryA(directory.c_str()) != 0;
			}
			int len = GetWindowTextLengthA(GetDlgItem(hwnd, 16));
			char* buffer = new char[len + 1];
			GetWindowTextA(GetDlgItem(hwnd, 16), buffer, len + 1);
			string ver = GetJavaMajorVersionStr(buffer);
			len = GetWindowTextLengthA(GetDlgItem(hwnd, 0xEEEE));
			string buffer2 = GetComboBoxText(GetDlgItem(hwnd, 0xEEEE));
			strcpy_s(VERSION, MAX_PATH, buffer2.c_str());
			printf("[游戏] Java版本为%s，游戏版本为%s，正在启动游戏. . . \n", ver, buffer2);
			SendMessageW(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 120));
			SendMessageW(hProgress, PBM_SETSTEP, 1, 0);
			SendMessageW(hProgress, PBM_SETBARCOLOR, 0, RGB(0, 120, 215));
			SendMessageW(hProgress, PBM_SETPOS, 0, 0);
			DownloadNew(buffer2, hProgress);
			/*
atomic<int> g_completed(0);
atomic<int> g_failed(0);
mutex g_print_mutex;
vector<pair<int, string>> tokens;
size_t tkptr = 0;*/
			g_completed = g_failed = 0;
			tokens.clear();
			tkptr = 0;
			g_versionInfo = {};
			printf("[游戏] Java路径：\n", JAVA_PATH);
			StartGame(buffer2);
			EnableWindow(sbtn._hwnd, true);
			PostMessageW(hProgress, WM_SYSCOMMAND, SC_CLOSE, 0);
			InvalidateRect(hwnd, 0, true); }
		).detach();
		}, true);
	sbtn.place(40, 321, 300, 48);
	hStart = sbtn._hwnd;
	Button(window, L"退出程序", Font(L"Microsoft Yahei UI", 31), [&]() {
		ExitProcess(0);
		}).place(360, 321, 200, 48);
	hListView = CreateWindowExW(0, L"Combobox", 0, WS_DISABLED | WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | CBS_HASSTRINGS | WS_VSCROLL, 152, 127, 180, 42, hwnd, HMENU(0xEEEE), 0, 0);
	SetWindowTextW(hListView, L"正在加载游戏版本. . . ");

	SendMessageW(hListView, WM_SETFONT, WPARAM(CreateFontA(28, 0, 0, 0, FW_NORMAL, 0, 0, 0, 1, 0, 0, 0, 0, "Microsoft Yahei UI")), 1);
	Checkbox btn = Checkbox(window, L"包括快照版本", Font(L"Microsoft Yahei UI", 28), [&]() {
		thread([&]() {
			string raw = HttpGet(MANIFEST_URL);
			json data = json::parse(raw);
			while (!IsWindow(hListView)) {
				Sleep(1);
			}
			if (raw.empty()) {
				SetWindowTextW(hListView, L"无法获取版本。");
				EnableWindow(hListView, false);
			}
			else {
				SendMessageW(hListView, CB_RESETCONTENT, 0, 0);
				EnableWindow(hListView, false);
				SetWindowTextA(hListView, string(data["latest"]["release"]).c_str());
				auto& versions_array = data["versions"];
				vector<Version> versions;
				for (auto it = versions_array.begin(); it != versions_array.end(); ++it) {
					Version v;
					v.id = (*it)["id"];
					v.type = (*it)["type"];
					EnableWindow(hListView, false);
					if (!btn.get()) {
						if (v.type == string("release")) {
							versions.push_back(v);
							//printf("[版本加载] 导入%s. . . \n", v);
							SendMessageA(hListView, CB_ADDSTRING, 0, LPARAM(string(v.id).c_str()));
						}
					}
					else {
						versions.push_back(v);
						//printf("[版本加载] 导入%s. . . \n", v);
						SendMessageA(hListView, CB_ADDSTRING, 0, LPARAM(string(v.id).c_str()));
					}
				}
				EnableWindow(hListView, true);
			}
			}).detach();
		});
	btn.place(350, 124, 180, 42);


	window.mainloop();
	return 0;
}