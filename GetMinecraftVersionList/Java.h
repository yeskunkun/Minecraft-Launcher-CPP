#pragma once
#include <string>
#include <windows.h>
#include <regex>
#include <cstring>
#include <sstream>        // istringstream
#include <map>
#include <iostream>
#include <iomanip>
#include <vector>
#include <winhttp.h>
#include <wininet.h>
#pragma comment(lib,"winhttp.lib")
#pragma comment(lib,"wininet.lib")
using namespace std;
// 传入java.exe/javaw.exe完整路径，返回主版本字符串："8" / "17" / "21" / "25"，失败返回空串
string GetJavaMajorVersionStr(const string& javaBinPath) {
	string cmd = "\"" + javaBinPath + "\" -version 2>&1";
	SECURITY_ATTRIBUTES sa{};
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	HANDLE hRead, hWrite;
	if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
		CloseHandle(hRead);
		CloseHandle(hWrite);
		return "";
	}
	STARTUPINFOA si{};
	si.cb = sizeof(STARTUPINFOA);
	si.hStdOutput = hWrite;
	si.hStdError = hWrite;
	si.dwFlags |= STARTF_USESTDHANDLES;
	PROCESS_INFORMATION pi{};
	BOOL createProc = CreateProcessA(0, const_cast<char*>(cmd.c_str()), 0, 0, TRUE, CREATE_NO_WINDOW, 0, 0, &si, &pi);
	CloseHandle(hWrite);
	char buf[2048]{};
	DWORD readLen;
	string output;
	while (ReadFile(hRead, buf, sizeof(buf) - 1, &readLen, 0) && readLen > 0) {
		output.append(buf, readLen);
		ZeroMemory(buf, sizeof(buf));
	}
	CloseHandle(hRead);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	regex regVer(R"(version \"([^"]+)\")");
	smatch m;
	if (!regex_search(output, m, regVer))
		return "";
	string raw = m[1].str();
	if (raw.rfind("1.8", 0) == 0)
		return "8";
	size_t dotPos = raw.find('.');
	if (dotPos == string::npos)
		return "";
	return raw.substr(0, dotPos);
}

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
CONSOLE_SCREEN_BUFFER_INFO csbi;
void ClearLine(int prev = 0) {
	GetConsoleScreenBufferInfo(GetStdHandle(-11), &csbi);
	csbi.dwCursorPosition.Y -= prev;
	csbi.dwCursorPosition.X = 0;
	DWORD wr;
	FillConsoleOutputCharacterW(GetStdHandle(-11), ' ', csbi.dwSize.X - csbi.dwCursorPosition.X, csbi.dwCursorPosition, &wr);
	SetConsoleCursorPosition(GetStdHandle(-11), csbi.dwCursorPosition);
	SetConsoleTextAttribute(GetStdHandle(-11), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
}
void DisplayInfo(long float total, long float filesize, long float speed, DWORD rs) {
	ClearLine(0);
	cout << "\rDownloading " << fixed << setprecision(2) <<
		//百分比
		total / filesize * 100.0 << "% ("
		//已下载大小
		<< ((total > 1024 * 1024) ? total / (1024.0 * 1024.0) : total / 1024.0) << " MiB / "
		//总大小
		<< ((filesize > 1024 * 1024) ? filesize / (1024.0 * 1024.0) : filesize / 1024.0) << " MiB)  Speed:"
		//速度
		<< ((speed > 1024) ? speed / 1024.0 : speed) << ((speed > 1024) ? " Mi" : " Ki") << "B/s  ETA:"
		//时间
		<< rs / 3600 << ":" << setw(2) << setfill('0') << rs / 60 % 60 << ":" << setw(2) << setfill('0') << rs % 60;
	cout.flush();
	PostMessageW(hProgress, PBM_SETPOS, total / filesize * 100, 0);
}
string GetFileNameFromURL(HINTERNET hConnect, const string& url) {
	char filename[MAX_PATH]{};
	DWORD bs = sizeof(filename);
	if (HttpQueryInfoA(hConnect, HTTP_QUERY_CONTENT_DISPOSITION, filename, &bs, 0)) {
		unsigned long long pos = string(filename).find("filename=");
		if (pos != -1) {
			string name = string(filename).substr(pos + 9);
			if (name.front() == '\"' && name.back() == '\"') {
				name = name.substr(1, name.length() - 2);
			}
			return name;
		}
	}
	string urlstring = url;
	unsigned long long lastSlash = urlstring.find_last_of(L'/');
	if (lastSlash != -1) {
		string name = urlstring.substr(lastSlash + 1);
		return name.substr(0, name.find(L'?'));
	}
	return "Unknown";
}
string DownloadFileWithProgress(LPCSTR url, LPCWSTR user_agent) {
	HINTERNET hInternet = InternetOpenW(user_agent, INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, 0);
	if (!hInternet) {
		return "";
	}
	HINTERNET hConnect = InternetOpenUrlA(hInternet, url, 0, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
	if (!hConnect) {
		return "";
	}
	unsigned long long lastbytes = 0, speed = 0.0, filesize = 0;
	wchar_t contentLengthStr[128]{};
	DWORD starttime = GetTickCount(), lasttime = starttime, readed = 0, lastspeed = starttime, bufferSize = sizeof(contentLengthStr), rs;
	if (HttpQueryInfoW(hConnect, HTTP_QUERY_CONTENT_LENGTH, contentLengthStr, &bufferSize, 0)) {
		filesize = _wtoi64(contentLengthStr);
	}
	else {
		filesize = 0;
	}
	string filename = GetFileNameFromURL(hInternet, url);
	HANDLE hFile = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	BYTE buffer[4096]{};
	long float total = 0;
	while (InternetReadFile(hConnect, buffer, sizeof(buffer), &readed) && readed > 1) {
		DWORD currenttime = GetTickCount(), wr;
		if (!WriteFile(hFile, buffer, readed, &wr, 0) || wr != readed) {
			return "";
		}
		total += readed;
		lasttime = currenttime;
		rs = (filesize - total) / (double(total) / ((currenttime - starttime) / 1000.0));
		if ((currenttime - lastspeed) >= 200) {
			speed = ((total - lastbytes) / 1024.0) / ((currenttime - lastspeed) / 1000.0);
			lastspeed = currenttime;
			lastbytes = total;
			DisplayInfo(total, filesize, speed, rs);
		}
	}
	CloseHandle(hFile);
	return filename;
}
// 主函数：获取所有 Java 路径及其版本号
map<string, string> GetJavaVersions() {
	map<string, string> result;
	vector<string> javaPaths;
	vector<string> extensions;
	char pathextBuf[1024];
	DWORD pathextLen = GetEnvironmentVariableA("PATHEXT", pathextBuf, 1024);
	if (pathextLen > 0) {
		string pathext(pathextBuf, pathextLen);
		size_t start = 0, end;
		while ((end = pathext.find(';', start)) != -1) {
			if (end > start) extensions.push_back(pathext.substr(start, end - start));
			start = end + 1;
		}
		if (start < pathext.length()) extensions.push_back(pathext.substr(start));
	}
	if (extensions.empty()) {
		extensions = { ".exe", ".com", ".bat", ".cmd" };
	}
	bool hasExt = false;
	for (const auto& ext : extensions) {
		if (string("java").length() >= ext.length() &&
			_stricmp(string("java").c_str() + string("java").length() - ext.length(), ext.c_str()) == 0) {
			hasExt = true;
			break;
		}
	}
	char currentDir[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, currentDir);
	char pathBuf[32767]{};
	DWORD pathLen = GetEnvironmentVariableA("PATH", pathBuf, 32767);
	if (pathLen != 0) {
		string pathStr(pathBuf, pathLen);
		vector<string> searchDirs;
		searchDirs.push_back(currentDir);
		size_t start = 0, end;
		while ((end = pathStr.find(';', start)) != -1) {
			if (end > start) {
				string dir = pathStr.substr(start, end - start);
				if (!dir.empty() && dir.back() == '\\')
					dir.pop_back();
				if (!dir.empty())
					searchDirs.push_back(dir);
			}
			start = end + 1;
		}
		if (start < pathStr.length()) {
			string dir = pathStr.substr(start);
			if (!dir.empty() && dir.back() == '\\') dir.pop_back();
			if (!dir.empty())
				searchDirs.push_back(dir);
		}
		for (const auto& dir : searchDirs) {
			string basePath = dir + "\\" + "java";
			if (hasExt) {
				if (GetFileAttributesA(basePath.c_str()) != INVALID_FILE_ATTRIBUTES) {
					javaPaths.push_back(basePath);
				}
			}
			else {
				for (const auto& ext : extensions) {
					string fullPath = basePath + ext;
					if (GetFileAttributesA(fullPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
						javaPaths.push_back(fullPath);
						break;
					}
				}
			}
		}
	}
	for (const auto& path : javaPaths) {
		string command = "\"" + path + "\" -version", output;
		HANDLE hReadPipe, hWritePipe;
		SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), 0, true };
		CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);
		STARTUPINFOA si = { sizeof(STARTUPINFOA) };
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdOutput = hWritePipe;
		si.hStdError = hWritePipe;
		si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		PROCESS_INFORMATION pi{};
		if (!CreateProcessA(0, (LPSTR)command.c_str(), 0, 0, true, CREATE_NO_WINDOW, 0, 0, &si, &pi)) {
			CloseHandle(hReadPipe);
			CloseHandle(hWritePipe);
		}
		CloseHandle(hWritePipe);
		WaitForSingleObject(pi.hProcess, 3000);
		char buffer[4096]{};
		DWORD wr;
		while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &wr, 0) && wr > 0) {
			buffer[wr] = '\0';
			output += buffer;
		}
		CloseHandle(hReadPipe);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		// 解析版本号
		istringstream stream(output);
		string line;
		string versionLine;
		while (getline(stream, line)) {
			if (line.find("version") != -1) {
				versionLine = line;
				break;
			}
		}
		// 提取引号中的版本号
		size_t start = versionLine.find('"');
		start++;
		size_t end = versionLine.find('"', start);
		string version = versionLine.substr(start, end - start);
		if (version.find("1.") == 0) {
			size_t dot1 = version.find('.');
			size_t dot2 = version.find('.', dot1 + 1);
			version = version.substr(dot1 + 1, dot2 - dot1 - 1);
		}
		else {
			size_t dot = version.find('.');
			version = version.substr(0, dot);
		}

		if (!version.empty()) {
			result[path] = version;
		}
		else {
			result[path] = "未知";  // 无法获取版本
		}
	}
	result.insert(JAVA_USERSELECTED.begin(), JAVA_USERSELECTED.end());
	return result;
}
bool InstallJava(string version,HWND hbtn) {
	map<string, string> ads = { {"8","https://javadl.oracle.com/webapps/download/AutoDL?BundleId=253195_f7fe8e644f724108bdb54139381e29a7"},
		{"17","https://download.oracle.com/java/17/archive/jdk-17.0.12_windows-x64_bin.exe"},
		{"25","https://download.oracle.com/java/25/latest/jdk-25_windows-x64_bin.exe" } };
	string url = ads[version];
	if (url.empty()) {
		printf("[下载] 版本错误\n");
		return false;
	}
	printf("[下载] 检索地址：%s\n", url.c_str());
	string name = DownloadFileWithProgress(url.c_str(), L"Mozilla/5.0");
	printf("\n[下载] Java 下载完成，正在安装，你可能需要在另一个窗口执行操作. . . \n");
	SHELLEXECUTEINFOA sei = { sizeof(SHELLEXECUTEINFOA) };
	sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_UNICODE;
	sei.lpVerb = "runas";
	sei.lpFile = name.c_str();
	sei.lpParameters = "/s";
	sei.nShow = SW_HIDE;
	DWORD exitCode = 1;
	if (!ShellExecuteExA(&sei)) {
		printf("ShellExecuteExA失败，错误代码%d\n", GetLastError());
		return false;
	}
	else {
		WaitForSingleObject(sei.hProcess, INFINITE);
		GetExitCodeProcess(sei.hProcess, &exitCode);
		CloseHandle(sei.hProcess);
	}
	if (exitCode != 0) {
		printf("[下载] Java 安装失败，返回代码%d\n", exitCode);
		return false;
	}
	printf("[下载] Java 安装完成！\n");

	thread([]() {
		SendMessageW(hListView2, CB_RESETCONTENT, 0, 0);
		EnableWindow(hListView2, false);
		auto a = GetJavaVersions();
		bool j = true;
		for (auto& i : a) {
			printf("[Java] 检测版本：%s\n", i.second);
			SendMessageA(hListView2, CB_ADDSTRING, 0, LPARAM(("Java " + i.second).c_str()));
			if (j) {
				SendMessageA(hListView2, CB_SELECTSTRING, 0, LPARAM(("Java " + i.second).c_str()));
			}
			j = false;
		}
		EnableWindow(hListView2, true);
		}
	).detach();

	EnableWindow(hbtn, true);
	return true;
}
