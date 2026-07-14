#include <windows.h>
#include <string>
#include <iostream>
#include <tchar.h>
using namespace std;
static LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	switch (Message) {
	/* 在销毁时，告诉主线程停止 */
	case WM_DESTROY: {
		PostQuitMessage(0);
		return 0;
	}
	default:
		return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	WNDCLASSEX wc{};
	HWND hwnd,tmph;
	MSG msg;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(0,IDC_ARROW);
	wc.hbrBackground = HBRUSH(5);
	wc.lpszClassName = L"WindowClass";
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION); /* 当您想使用项目图标时，请使用 "A" 作为图标名称 */
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	if (!RegisterClassEx(&wc)) {
		MessageBox(0,L"窗口注册失败！",L"错误",327696);
		return 0;
	}
	hwnd = CreateWindowExW(WS_EX_DLGMODALFRAME, L"WindowClass", L"Form", 281673728,
		CW_USEDEFAULT, /* x */
		CW_USEDEFAULT, /* y */
		617, /* 宽度 */
		440, /* 高度 */
		0,0,hInstance,0);
	/*  这是我们的程序核心，所有输入都在这里处理并发送
	 	到WndProc。请注意，GetMessage在接收到消息之前会
	 	阻止代码流，这个循环不会导致CPU使用率过高。  */
	

		tmph = CreateWindowExW(0, L"Static",L"Minecraft Launcher",WS_CHILD | WS_VISIBLE | SS_CENTER,137,3,327,40,hwnd,HMENU(0),GetModuleHandle(0),0);
		SendMessage(tmph,WM_SETFONT,WPARAM(CreateFontA(29,0,0,0,FW_NORMAL,0,0,0,1,0,0,0,0,LPCSTR("微软雅黑"))),1);

		tmph = CreateWindowExW(0, L"Static",L"Made by Yeskunkun!",WS_CHILD | WS_VISIBLE | SS_CENTER,137,43,327,32,hwnd,HMENU(1),GetModuleHandle(0),0);
		SendMessage(tmph,WM_SETFONT,WPARAM(CreateFontA(24,0,0,0,FW_NORMAL,0,0,0,1,0,0,0,0,LPCSTR("微软雅黑"))),1);

		tmph = CreateWindowExW(0, L"Static",L"用户名：",WS_CHILD | WS_VISIBLE | SS_CENTER,10,178,113,39,hwnd,HMENU(2),GetModuleHandle(0),0);
		SendMessage(tmph,WM_SETFONT,WPARAM(CreateFontA(28,0,0,0,FW_NORMAL,0,0,0,1,0,0,0,0,LPCSTR("微软雅黑"))),1);

		tmph = CreateWindowExW(0, L"Button",L"退出程序",WS_CHILD | WS_VISIBLE | BS_CENTER,360,321,200,48,hwnd,HMENU(4),GetModuleHandle(0),0);
		SendMessage(tmph,WM_SETFONT,WPARAM(CreateFontA(28,0,0,0,FW_NORMAL,0,0,0,1,0,0,0,0,LPCSTR("微软雅黑"))),1);

		tmph = CreateWindowExW(0, L"Button",L"启动游戏",WS_CHILD | WS_VISIBLE | BS_CENTER,40,321,300,48,hwnd,HMENU(6),GetModuleHandle(0),0);
		SendMessage(tmph,WM_SETFONT,WPARAM(CreateFontA(28,0,0,0,FW_NORMAL,0,0,0,1,0,0,0,0,LPCSTR("微软雅黑"))),1);

		tmph = CreateWindowExW(0, L"Button",L"1.12.2                 V  ",WS_CHILD | WS_VISIBLE | BS_CENTER,20,131,223,42,hwnd,HMENU(8),GetModuleHandle(0),0);
		SendMessage(tmph,WM_SETFONT,WPARAM(CreateFontA(27,0,0,0,FW_NORMAL,0,0,0,1,0,0,0,0,LPCSTR("微软雅黑"))),1);

		tmph = CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit",L"",WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,255,130,318,36,hwnd,HMENU(11),GetModuleHandle(0),0);
		SendMessage(tmph,WM_SETFONT,WPARAM(CreateFontA(25,0,0,0,FW_NORMAL,0,0,0,1,0,0,0,0,LPCSTR("微软雅黑"))),1);

		tmph = CreateWindowExW(0, L"Static",L"当前游戏版本",WS_CHILD | WS_VISIBLE | SS_CENTER,7,80,166,39,hwnd,HMENU(12),GetModuleHandle(0),0);
		SendMessage(tmph,WM_SETFONT,WPARAM(CreateFontA(28,0,0,0,FW_NORMAL,0,0,0,1,0,0,0,0,LPCSTR("微软雅黑"))),1);

		tmph = CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit",L"",WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,128,177,360,36,hwnd,HMENU(14),GetModuleHandle(0),0);
		SendMessage(tmph,WM_SETFONT,WPARAM(CreateFontA(25,0,0,0,FW_NORMAL,0,0,0,1,0,0,0,0,LPCSTR("微软雅黑"))),1);

		tmph = CreateWindowExW(0, L"Button",L"随机",WS_CHILD | WS_VISIBLE | BS_CENTER,502,177,75,42,hwnd,HMENU(17),GetModuleHandle(0),0);
		SendMessage(tmph,WM_SETFONT,WPARAM(CreateFontA(27,0,0,0,FW_NORMAL,0,0,0,1,0,0,0,0,LPCSTR("微软雅黑"))),1);


	while (GetMessage(&msg,0,0,0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}