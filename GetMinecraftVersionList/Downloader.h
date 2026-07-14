#pragma once
#include "Extract.h"
#include <WinHttp.h>
#include <vector>
#include <mutex>
#include <Shlwapi.h>
#include <string>
#include <cstdio>
using namespace std;
atomic<int> g_completed(0);
atomic<int> g_failed(0);
mutex g_print_mutex;
vector<pair<int, string>> tokens;
size_t tkptr = 0;
wstring S2W(const string& utf8) {
	int req = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, 0, 0);
	wstring buf(req, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &buf[0], req);
	return buf;
}
string W2S(const wstring& wstr) {
	int req = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, 0, 0, 0, 0);
	string buf(req, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &buf[0], req, 0, 0);
	return buf;
}

// 单一主函数：输入MC版本字符串，返回需要的Java版本字符串 "8"/"17"/"21"/"25"
// 快照：26wxx → 25；24w14~25wxx →21；21w19~24w13→17；更早快照→8
// 正式版：1.x 旧格式 / 26.x 新格式
std::string GetMinecraftRequireJava(const std::string& mcVer) {
	// 判断快照格式 XXwXXx
	if (mcVer.size() >= 5
		&& isdigit(static_cast<unsigned char>(mcVer[0]))
		&& isdigit(static_cast<unsigned char>(mcVer[1]))
		&& mcVer[2] == 'w'
		&& isdigit(static_cast<unsigned char>(mcVer[3]))
		&& isdigit(static_cast<unsigned char>(mcVer[4])))
	{
		int year = (mcVer[0] - '0') * 10 + (mcVer[1] - '0');
		int week = (mcVer[3] - '0') * 10 + (mcVer[4] - '0');

		if (year > 26 || (year == 26 && week >= 1))
			return "25";
		if (year > 24 || (year == 24 && week >= 14))
			return "21";
		if (year > 21 || (year == 21 && week >= 19))
			return "17";
		return "8";
	}

	// 正式版本分支
	size_t firstDot = mcVer.find('.');
	if (firstDot == std::string::npos)
		return "17"; // 非法版本兜底

	int major = std::stoi(mcVer.substr(0, firstDot));
	if (major == 1)
	{
		// 传统 1.x.y 格式
		int minor = std::stoi(mcVer.substr(firstDot + 1, 2));
		if (minor <= 16)
			return "8";
		if (minor >= 17 && minor <= 20)
		{
			if (minor == 20)
			{
				size_t secondDot = mcVer.find('.', firstDot + 1);
				if (secondDot != std::string::npos)
				{
					int patch = std::stoi(mcVer.substr(secondDot + 1));
					if (patch >= 5) return "21";
				}
				return "17";
			}
			return "17";
		}
		// 1.21 ~ 1.25
		return "21";
	}
	else if (major >= 26)
	{
		// 新版号 26.1 / 26.1.2 / 26.2 全部 Java25
		return "25";
	}
	// 21~25 新版号
	return "21";
}
struct JsonVal {
	int type;
	string str;
	vector<JsonVal> arr;
	vector<pair<string, JsonVal>> obj;
	JsonVal() : type(5) {}
	bool HasKey(const string& k) const {
		for (auto& p : obj) {
			if (p.first == k) return true;
		}
		return false;
	}
	const JsonVal& Get(const string& k) const {
		for (auto& p : obj) {
			if (p.first == k) return p.second;
		}
		static JsonVal empty;
		return empty;
	}
	const JsonVal& operator[](size_t idx) const {
		return arr[idx];
	}
};
JsonVal g_versionInfo;
string PathCat(const string& a, const string& b) {
	char tmp[MAX_PATH]{};
	PathCombineA(tmp, a.c_str(), b.c_str());
	return tmp;
}
string HttpGet(const string& url, DWORD timeout = 10000) {
	HINTERNET hSession = 0, hConnect = 0, hRequest = 0;
	vector<char> rawBytes;
	URL_COMPONENTS urlComp = { sizeof(URL_COMPONENTS) };
	wchar_t host[MAX_PATH]{}, path[1024]{};
	urlComp.lpszHostName = host;
	urlComp.dwHostNameLength = sizeof(host) - 1;
	urlComp.lpszUrlPath = path;
	urlComp.dwUrlPathLength = sizeof(path) - 1;
	wstring wideUrl = S2W(url);
	WinHttpCrackUrl(wideUrl.c_str(), (DWORD)wideUrl.length(), 0, &urlComp);
	hSession = WinHttpOpen(L"", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	WinHttpSetTimeouts(hSession, timeout, timeout, timeout, timeout);
	hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, urlComp.nPort, 0);
	DWORD requestFlags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
	hRequest = WinHttpOpenRequest(hConnect, L"GET", urlComp.lpszUrlPath, 0,
		WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, requestFlags);
	WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
	WinHttpReceiveResponse(hRequest, 0);

	DWORD size = 0;
	do {
		if (!WinHttpQueryDataAvailable(hRequest, &size) || size == 0) {
			break;
		}
		vector<char> buffer(size);
		DWORD downloaded = 0;
		if (WinHttpReadData(hRequest, buffer.data(), size, &downloaded) && downloaded > 0) {
			rawBytes.insert(rawBytes.end(), buffer.data(), buffer.data() + downloaded);
		}
	} while (size > 0);
	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	if (hSession) WinHttpCloseHandle(hSession);
	return string(rawBytes.begin(), rawBytes.end());
}
void JsonTokenize(const string& src) {
	tokens.clear(); tkptr = 0;
	size_t i = 0, len = src.size();
	while (i < len) {
		char c = src[i];
		if (isspace((unsigned char)c)) {
			i++;
			continue;
		}
		switch (c) {
		case '{': { tokens.push_back({ 0, "{" }); i++; break; }
		case '}': { tokens.push_back({ 1, "}" }); i++; break; }
		case '[': { tokens.push_back({ 2, "[" }); i++; break; }
		case ']': { tokens.push_back({ 3, "]" }); i++; break; }
		case ':': { tokens.push_back({ 4, ":" }); i++; break; }
		case ',': { tokens.push_back({ 5, "," }); i++; break; }
		case '"': {
			i++; string s;
			while (i < len && src[i] != '"') {
				if (src[i] == '\\') i++;
				s += src[i];
				i++;
			}
			tokens.push_back({ 6, s });
			i++;
			break;
		}
		default: {
			string word;
			while (i < len && !isspace((unsigned char)src[i])
				&& src[i] != '{' && src[i] != '}' && src[i] != '[' && src[i] != ']'
				&& src[i] != ':' && src[i] != ',') {
				word += src[i];
				i++;
			}
			if (word == "true" || word == "false") {
				tokens.push_back({ 8, word });
			}
			else if (word == "null") {
				tokens.push_back({ 9, word });
			}
			else {
				tokens.push_back({ 7, word });
			}
			break;
		}
		}
	}
}
JsonVal ParseJson() {
	JsonVal res;
	auto& t = tokens[tkptr++];
	int tokenType = t.first;
	switch (tokenType) {
	case 6: { res.type = 0; res.str = t.second; return res; }
	case 7: { res.type = 3; res.str = t.second; return res; }
	case 8: { res.type = 4; res.str = t.second; return res; }
	case 9: { res.type = 5; return res; }
	case 0: {
		res.type = 2;
		while (tokens[tkptr].first != 1) {
			auto k = tokens[tkptr++];
			tkptr++;
			res.obj.emplace_back(k.second, ParseJson());
			if (tokens[tkptr].first == 5) tkptr++;
		}
		tkptr++;
		return res;
	}
	case 2: {
		res.type = 1;
		while (tokens[tkptr].first != 3) {
			res.arr.push_back(ParseJson());
			if (tokens[tkptr].first == 5) tkptr++;
		}
		tkptr++;
		return res;
	}
	default: break;
	}
	return res;
}
bool DownloadFile(const string& url, const string& savePath) {
	DWORD attr = GetFileAttributesA(savePath.c_str());
	if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
		HANDLE hFile = CreateFileA(savePath.c_str(), GENERIC_READ, FILE_SHARE_READ,
			0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hFile != INVALID_HANDLE_VALUE) {
			DWORD size = GetFileSize(hFile, 0);
			CloseHandle(hFile);
			if (size > 0) {
				g_completed++;
				return true;
			}
		}
	}
	string dir = savePath;
	size_t pos = dir.find_last_of("/\\");
	if (pos != string::npos) {
		dir = dir.substr(0, pos);
		for (size_t i = 0; i < dir.length(); i++) {
			if (dir[i] == '/' || dir[i] == '\\') {
				dir[i] = '\\';
				CreateDirectoryA(dir.substr(0, i + 1).c_str(), 0);
			}
		}
		CreateDirectoryA(dir.c_str(), 0);
	}
	for (int attempt = 0; attempt < 3; attempt++) {
		string tempPath = savePath + ".tmp";
		string data = HttpGet(url, 30000);
		if (data.empty()) {
			Sleep(500);
			continue;
		}
		HANDLE hFile = CreateFileA(tempPath.c_str(), GENERIC_WRITE, 0, 0,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (hFile == INVALID_HANDLE_VALUE) {
			Sleep(500);
			continue;
		}
		DWORD wr;
		if (!WriteFile(hFile, data.c_str(), (DWORD)data.size(), &wr, 0) || wr != data.size()) {
			CloseHandle(hFile);
			DeleteFileA(tempPath.c_str());
			Sleep(500);
			continue;
		}
		CloseHandle(hFile);
		HANDLE hCheck = CreateFileA(tempPath.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hCheck != INVALID_HANDLE_VALUE) {
			DWORD size = GetFileSize(hCheck, 0);
			CloseHandle(hCheck);
			if (size > 0) {
				if (attr != INVALID_FILE_ATTRIBUTES) {
					DeleteFileA(savePath.c_str());
				}
				MoveFileA(tempPath.c_str(), savePath.c_str());
				g_completed++;
				return true;
			}
		}
		DeleteFileA(tempPath.c_str());
		Sleep(1000);
	}
	g_failed++;
	return false;
}
void DownloadAllTasks(const vector<pair<string, string>>& tasks, HWND hProgress) {
	int total = (int)tasks.size();
	printf("\n开始下载所有文件，共 %d 个\n", total);
	printf("%s\n", string(50, '=').c_str());
	int workerCount = min(64, total);
	vector<thread> threads;
	atomic<int> current(0);
	for (int i = 0; i < workerCount; i++) {
		threads.emplace_back([&]() {
			while (true) {
				int idx = current++;
				if (idx >= total) break;

				const auto& task = tasks[idx];
				DownloadFile(task.first, task.second);

				lock_guard<mutex> lock(g_print_mutex);
				int done = g_completed + g_failed;
				printf("\r下载中 %.2f%% (%d/%d)，成功:%d 失败:%d", double(done) / total * 100.0, done, total,
					g_completed.load(), g_failed.load());
				PostMessageW(GetDlgItem(hwnd,17), PBM_SETPOS, done, 0);
			}});
	}
	for (auto& t : threads) {
		if (t.joinable()) t.join();
	}
	printf("\n%s\n", string(50, '=').c_str());
}
string GetNativeDir() {
	SYSTEM_INFO si;
	GetNativeSystemInfo(&si);
	string archName = (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) ? "x86_64" : "x86";
	return "natives-windows-" + archName;
}
void ExtractNatives(const JsonVal& versionInfo) {
	char bufA[MAX_PATH]{};
	GetCurrentDirectoryA(MAX_PATH, bufA);
	string work(bufA);
	string gameDir = PathCat(work, GAME_DIR_REL);
	string verDir = PathCat(PathCat(gameDir, "versions"), VERSION);
	string nativesDir = PathCat(verDir, GetNativeDir());
	if (PathFileExistsA(nativesDir.c_str())) {
		string searchPath = nativesDir + "\\*.dll";
		WIN32_FIND_DATAA findData;
		HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
		if (hFind != INVALID_HANDLE_VALUE) {
			FindClose(hFind);
			printf("[阶段:解压] Native库已解压: %s\n", nativesDir.c_str());
			return;
		}
		FindClose(hFind);
	}
	CreateDirectoryA(nativesDir.c_str(), 0);
	if (!versionInfo.HasKey("libraries")) {
		return;
	}
	string nativeKey = "windows";
	printf("[阶段:解压] 开始解压native库...\n");
	int count = 0;
	int failed = 0;
	const JsonVal& libs = versionInfo.Get("libraries");
	for (size_t i = 0; i < libs.arr.size(); i++) {
		const JsonVal& lib = libs[i];
		bool allow = true;
		if (lib.HasKey("rules")) {
			allow = false;
			const JsonVal& rules = lib.Get("rules");
			for (size_t r = 0; r < rules.arr.size(); r++) {
				const JsonVal& rule = rules[r];
				string act = rule.Get("action").str;
				if (!rule.HasKey("os")) {
					if (act == "allow") allow = true;
					continue;
				}
				string osName = rule.Get("os").Get("name").str;
				if (act == "allow" && osName == "windows") allow = true;
				if (act == "disallow" && osName == "windows") allow = false;
			}
		}
		if (!allow) continue;
		const JsonVal& dl = lib.Get("downloads");
		if (!dl.HasKey("classifiers")) continue;
		const JsonVal& cls = dl.Get("classifiers");
		for (auto& p : cls.obj) {
			if (p.first.find(nativeKey) != string::npos) {
				string libPath = PathCat(PathCat(gameDir, "libraries"),
					p.second.Get("path").str);
				if (PathFileExistsA(libPath.c_str())) {
					if (Extract(libPath, nativesDir)) {
						count++;
						printf("\r[阶段:解压] 已解压 %d 个native库", count);
					}
				}
				break;
			}
		}
	}
	printf("\n[阶段:解压] 已解压 %d 个native库", count);
	if (failed > 0) {
		printf("，失败 %d 个", failed);
	}
	printf(" 到: %s\n", nativesDir.c_str());
}
string GetFileNameFromURL(const std::string& url) {
	if (url.empty())
		return "";
	size_t lastSlash = url.find_last_of("/\\");
	string filename;
	if (lastSlash != -1) {
		filename = url.substr(lastSlash + 1);
	}
	else {
		filename = url;
	}
	size_t questionMark = filename.find('?');
	if (questionMark != -1) {
		filename = filename.substr(0, questionMark);
	}
	size_t hashMark = filename.find('#');
	if (hashMark != -1) {
		filename = filename.substr(0, hashMark);
	}
	filename.erase(0, filename.find_first_not_of(" \t\r\n"));
	filename.erase(filename.find_last_not_of(" \t\r\n") + 1);
	if (filename.empty())
		return "download";
	return filename;
}
vector<pair<string, string>> BuildTaskList() {
	vector<pair<string, string>> alltsk;
	alltsk.reserve(512);

	char bufA[MAX_PATH]{};
	GetCurrentDirectoryA(MAX_PATH, bufA);
	string work(bufA);
	string gameDir = PathCat(work, GAME_DIR_REL);
	string verDir = PathCat(PathCat(gameDir, "versions"), VERSION);
	string libDir = PathCat(gameDir, "libraries");
	string assetDir = PathCat(gameDir, "assets");
	string sysOs = "windows";
	string nativeKey = "windows";

	string raw = HttpGet(MANIFEST_URL);
	if (raw.empty()) {
		printf("[下载] 无法获取清单\n");
		return {};
	}

	JsonTokenize(raw);
	tkptr = 0;
	JsonVal root = ParseJson();
	const JsonVal& versions = root.Get("versions");

	string verMetaUrl;
	for (size_t i = 0; i < versions.arr.size(); i++) {
		if (versions[i].Get("id").str == VERSION) {
			verMetaUrl = versions[i].Get("url").str;
			break;
		}
	}

	if (verMetaUrl.empty()) {
		printf("[下载] 无法找到版本 %s\n", VERSION);
		return {};
	}

	raw = HttpGet(verMetaUrl.c_str());
	if (raw.empty()) {
		printf("[下载] 无法获取版本详情\n");
		return {};
	}

	JsonTokenize(raw);
	tkptr = 0;
	g_versionInfo = ParseJson();

	// 客户端jar和json
	if (g_versionInfo.HasKey("downloads")) {
		const JsonVal& dl = g_versionInfo.Get("downloads");
		if (dl.HasKey("client")) {
			alltsk.push_back({ dl.Get("client").Get("url").str, PathCat(verDir, string(VERSION) + ".jar") });
			alltsk.push_back({ verMetaUrl ,PathCat(verDir, string(VERSION) + ".json") });
		}
	}

	// 库文件
	if (g_versionInfo.HasKey("libraries")) {
		const JsonVal& libs = g_versionInfo.Get("libraries");
		for (size_t i = 0; i < libs.arr.size(); i++) {
			const JsonVal& lib = libs[i];
			bool allow = true;

			if (lib.HasKey("rules")) {
				allow = false;
				const JsonVal& rules = lib.Get("rules");
				for (size_t r = 0; r < rules.arr.size(); r++) {
					const JsonVal& rule = rules[r];
					string act = rule.Get("action").str;
					if (!rule.HasKey("os")) {
						if (act == "allow") allow = true;
						continue;
					}
					string osName = rule.Get("os").Get("name").str;
					if (act == "allow" && osName == sysOs) allow = true;
					if (act == "disallow" && osName == sysOs) allow = false;
				}
			}

			if (!allow) continue;

			const JsonVal& dl = lib.Get("downloads");
			if (dl.HasKey("artifact")) {
				const JsonVal& art = dl.Get("artifact");
				alltsk.push_back({ art.Get("url").str,
								  PathCat(libDir, art.Get("path").str) });
			}

			if (dl.HasKey("classifiers")) {
				const JsonVal& cls = dl.Get("classifiers");
				for (auto& p : cls.obj) {
					if (p.first.find(nativeKey) != string::npos) {
						alltsk.push_back({ p.second.Get("url").str, PathCat(libDir, p.second.Get("path").str) });
						break;
					}
				}
			}
		}
	}

	// 资源文件
	if (g_versionInfo.HasKey("assetIndex")) {
		const JsonVal& idxInfo = g_versionInfo.Get("assetIndex");
		string idxUrl = idxInfo.Get("url").str;
		string aid = idxInfo.Get("id").str;
		string idxRaw = HttpGet(idxUrl.c_str());

		alltsk.push_back({ idxUrl, PathCat(assetDir, "indexes\\" + GetFileNameFromURL(idxUrl)) });
		if (!idxRaw.empty()) {
			JsonTokenize(idxRaw);
			tkptr = 0;
			JsonVal assetJson = ParseJson();
			if (assetJson.HasKey("objects")) {
				const JsonVal& objs = assetJson.Get("objects");
				for (auto& entry : objs.obj) {
					string hash = entry.second.Get("hash").str;
					string url = "https://resources.download.minecraft.net/" +
						hash.substr(0, 2) + "/" + hash;
					string path = PathCat(PathCat(PathCat(assetDir, "objects"),
						hash.substr(0, 2)), hash);
					alltsk.push_back({ url, path });
				}
			}
		}
		else {
			printf("[下载] 获取资源索引失败\n");
		}
	}

	return alltsk;
}
////////////////////////////////////////////////
bool DownloadNew(string VERSION,HWND hProgress) {
	printf("Minecraft %s 下载器\nMade by Yeskunkun!\n", VERSION);
	printf("[下载] 正在解析版本信息...\n");
	vector<pair<string, string>> tasks = BuildTaskList();
	if (tasks.empty()) {
		printf("[下载] 没有找到任何下载任务\n");
		return 1;
	}
	printf("[下载] 共需下载 %zu 个文件\n", tasks.size());
	SendMessageW(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, tasks.size()));
	DownloadAllTasks(tasks,hProgress);
	ExtractNatives(g_versionInfo);
	printf("[下载] 下载完成，成功: %d 失败: %d\n", g_completed.load(), g_failed.load());
	SendMessageW(hProgress,WM_SYSCOMMAND,SC_CLOSE,0);
	if (GetLastError()) {
		return false;
	}
	return true;
}
bool StartGame(string VERSION) {
	//此处做了修改，如果要调用API，移除下面的这些。
	int len = GetWindowTextLengthA(GetDlgItem(hwnd, 0xEEEB));
	char* buffer = new char[len + 1];
	GetWindowTextA(GetDlgItem(hwnd, 0xEEEB), buffer, len + 1);
	///////////////////////////////////////////
	string commandline = string(JAVA_PATH) + " " + getscript(GAME_DIR_REL, VERSION, "2048M", buffer);
	//ofstream file("test.bat", ios::binary);
	//file << "@" << commandline << "\r\n@set /p = \"Press any key. . . \"< nul & pause > nul";
	//file.close();
	printf("[JVM] 脚本生成完成，正在执行命令. . . \n==================================================\n%s\n==================================================\n[JVM] 游戏日志：\n",commandline.c_str());
	RunProgramInCurrentConsole(commandline.c_str());
	printf("[JVM] 游戏进程结束\n");
	if (GetLastError()) {
		return false;
	}
	return true;
}