#pragma once
#include <fstream>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <vector>
#include <regex>
#include <algorithm>
#include <windows.h>
#include "json.hpp"
using namespace std;
using json = nlohmann::json;
namespace fs = filesystem;
// 安全字符串替换
BOOL RunProgramInCurrentConsole(const string& _Command) {
	SECURITY_ATTRIBUTES sa{};
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = true;
	sa.lpSecurityDescriptor = 0;
	HANDLE hStdOutRead, hStdOutWrite, hStdErrRead, hStdErrWrite;
	CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0);
	CreatePipe(&hStdErrRead, &hStdErrWrite, &sa, 0);
	SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(hStdErrRead, HANDLE_FLAG_INHERIT, 0);
	STARTUPINFOA si{};
	PROCESS_INFORMATION pi{};
	si.cb = sizeof(STARTUPINFOA);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	si.hStdOutput = hStdOutWrite;
	si.hStdError = hStdOutWrite;
	BOOL bCreateSuccess = CreateProcessA(0, LPSTR(_Command.c_str()), 0, 0, true, 0, 0, 0, &si, &pi);
	CloseHandle(hStdOutWrite);
	CloseHandle(hStdErrWrite);
	char buffer[4096]{};
	DWORD dw = 0;
	while (ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &dw, 0) && dw > 0) {
		buffer[dw] = '\0';
		WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), buffer, dw, 0, 0);
	}
	WaitForSingleObject(pi.hProcess, INFINITE);
	GetExitCodeProcess(pi.hProcess, &dw);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(hStdOutRead);
	CloseHandle(hStdErrRead);
	return dw;
}
void string_replace(string& str, const string& old_str, const string& new_str) {
	if (old_str.empty() || str.empty()) return;
	size_t pos = 0;
	while ((pos = str.find(old_str, pos)) != string::npos)
	{
		str.replace(pos, old_str.length(), new_str);
		pos += new_str.length();
	}
}
string json_get_str(const json& j, const string& key, const string& def = "") {
	if (!j.contains(key) || !j[key].is_string()) return def;
	return j[key].get<string>();
}
string build_classpath(const string& mcdir, const string& version, const json& dic) {
	string classpath;
	vector<string> lib_paths;
	if (dic.contains("libraries") && dic["libraries"].is_array()) {
		for (const auto& lib : dic["libraries"]) {
			if (!lib.is_object()) continue;
			if (lib.contains("natives") && lib["natives"].is_object())
				continue;

			if (!lib.contains("downloads") || !lib["downloads"].is_object()) continue;
			const auto& downloads = lib["downloads"];
			if (!downloads.contains("artifact") || !downloads["artifact"].is_object()) continue;
			const auto& artifact = downloads["artifact"];
			string lib_path = json_get_str(artifact, "path");
			if (lib_path.empty()) continue;

			string full_lib = mcdir + "\\libraries\\" + lib_path;
			replace(full_lib.begin(), full_lib.end(), '/', '\\');

			if (fs::exists(full_lib))
			{
				lib_paths.push_back(full_lib);
			}
		}
	}

	// 添加版本主jar
	string main_jar = mcdir + "\\versions\\" + version + "\\" + version + ".jar";
	if (fs::exists(main_jar))
		lib_paths.push_back(main_jar);
	for (size_t i = 0; i < lib_paths.size(); ++i) {
		classpath += "\"" + lib_paths[i] + "\"";
		if (i != lib_paths.size() - 1)
			classpath += ";";
	}

	return classpath;
}
#define ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
std::string GenerateFixedUUID(const std::string& name, const std::string& namespaceUUID = "6ba7b810-9dad-11d1-80b4-00c04fd430c8") {
	// ============ 1. 解析命名空间 UUID ============
	std::string cleanNS;
	for (char c : namespaceUUID) {
		if (c != '-' && c != '{' && c != '}') cleanNS += c;
	}
	if (cleanNS.length() != 32) return "";

	uint8_t nsBytes[16];
	for (int i = 0; i < 16; i++) {
		nsBytes[i] = (uint8_t)std::stoi(cleanNS.substr(i * 2, 2), nullptr, 16);
	}

	// ============ 2. 准备 SHA-1 输入数据 ============
	std::vector<uint8_t> data;
	data.insert(data.end(), nsBytes, nsBytes + 16);
	data.insert(data.end(), name.begin(), name.end());

	// ============ 3. SHA-1 计算 ============
	uint32_t h0 = 0x67452301, h1 = 0xEFCDAB89, h2 = 0x98BADCFE, h3 = 0x10325476, h4 = 0xC3D2E1F0;
	uint64_t messageLen = data.size() * 8;

	// 填充
	std::vector<uint8_t> buffer = data;
	buffer.push_back(0x80);
	while ((buffer.size() * 8) % 512 != 448) buffer.push_back(0);

	// 添加长度
	for (int i = 7; i >= 0; i--) buffer.push_back((messageLen >> (i * 8)) & 0xFF);

	// SHA-1 主循环
	for (size_t offset = 0; offset < buffer.size(); offset += 64) {
		uint32_t w[80];
		for (int i = 0; i < 16; i++) {
			w[i] = (buffer[offset + i * 4] << 24) | (buffer[offset + i * 4 + 1] << 16) |
				(buffer[offset + i * 4 + 2] << 8) | buffer[offset + i * 4 + 3];
		}
		for (int i = 16; i < 80; i++) {
			w[i] = ROTL32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
		}

		uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
		for (int i = 0; i < 80; i++) {
			uint32_t f, k;
			if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999; }
			else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
			else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
			else { f = b ^ c ^ d; k = 0xCA62C1D6; }
			uint32_t temp = ROTL32(a, 5) + f + e + k + w[i];
			e = d; d = c; c = ROTL32(b, 30); b = a; a = temp;
		}
		h0 += a; h1 += b; h2 += c; h3 += d; h4 += e;
	}

	// ============ 4. 提取哈希前 16 字节 ============
	uint8_t hash[20];
	for (int i = 0; i < 4; i++) hash[i] = (h0 >> (24 - i * 8)) & 0xFF;
	for (int i = 0; i < 4; i++) hash[4 + i] = (h1 >> (24 - i * 8)) & 0xFF;
	for (int i = 0; i < 4; i++) hash[8 + i] = (h2 >> (24 - i * 8)) & 0xFF;
	for (int i = 0; i < 4; i++) hash[12 + i] = (h3 >> (24 - i * 8)) & 0xFF;
	for (int i = 0; i < 4; i++) hash[16 + i] = (h4 >> (24 - i * 8)) & 0xFF;

	// ============ 5. 设置版本和变体 ============
	hash[6] = (hash[6] & 0x0F) | 0x50;  // v5
	hash[8] = (hash[8] & 0x3F) | 0x80;  // RFC 4122

	// ============ 6. 格式化为 UUID 字符串 ============
	std::ostringstream oss;
	oss << std::hex << std::setfill('0');
	for (int i = 0; i < 16; i++) {
		oss << std::setw(2) << (int)hash[i];
		if (i == 3 || i == 5 || i == 7 || i == 9) oss << '-';
	}
	return oss.str();
}
string getscript(const string& mcdir,const string& version,const string& maxMen,const string& username, const string& accessToken = "${accessToken}") {
	const string version_json_path = mcdir + "\\versions\\" + version + "\\" + version + ".json";
	json dic;
	try {
		if (!fs::exists(version_json_path))
			throw runtime_error("文件不存在: " + version_json_path);
		ifstream file(version_json_path);
		if (!file.is_open())
			throw runtime_error("无法打开文件: " + version_json_path);
		string json_text((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
		file.close();
		dic = json::parse(json_text);
	} catch (const exception& e){
		cout << "解析版本json失败: " << string(e.what()) << '\n';
		return "";
	}
	// 获取natives路径
	vector<string> possible_paths = {
		mcdir + "\\versions\\" + version + "\\natives-windows-x86_64",
		mcdir + "\\versions\\" + version + "\\natives-windows",
		mcdir + "\\versions\\" + version + "\\natives",
		mcdir + "\\versions\\" + version + "\\" + version + "-natives"};
	string natives_dir = possible_paths[0];
	for (const auto& path : possible_paths) {
		if (fs::exists(path)) {
			// 检查是否包含dll文件
			for (const auto& entry : fs::directory_iterator(path)) {
				if (entry.path().extension() == ".dll") {
					natives_dir = path;
				}
			}
		}
	}
	// 构建classpath
	string classpath = build_classpath(mcdir, version, dic);

	// 构建JVM参数
	string JVM;

	// 内存参数
	JVM += "-Xmx" + maxMen + " ";
	JVM += "-Xmn256m ";

	// 编码设置
	JVM += "-Dfile.encoding=GB18030 ";
	JVM += "-Dsun.stdout.encoding=GB18030 ";
	JVM += "-Dsun.stderr.encoding=GB18030 ";

	// JVM性能参数
	JVM += "-XX:+UnlockExperimentalVMOptions ";
	JVM += "-XX:+UnlockDiagnosticVMOptions ";
	JVM += "-XX:+UseG1GC ";
	JVM += "-XX:G1MixedGCCountTarget=5 ";
	JVM += "-XX:G1NewSizePercent=20 ";
	JVM += "-XX:G1ReservePercent=20 ";
	JVM += "-XX:MaxGCPauseMillis=50 ";
	JVM += "-XX:G1HeapRegionSize=32m ";
	JVM += "-XX:-OmitStackTraceInFastThrow ";
	JVM += "-XX:MaxInlineLevel=15 ";
	JVM += "-XX:-DontCompileHugeMethods ";
	JVM += "-XX:MaxNodeLimit=240000 ";
	JVM += "-XX:NodeLimitFudgeFactor=8000 ";
	JVM += "-XX:TieredCompileTaskTimeout=10000 ";
	JVM += "-XX:ReservedCodeCacheSize=400M ";
	JVM += "-XX:NmethodSweepActivity=1 ";

	// 安全参数
	JVM += "-Djava.rmi.server.useCodebaseOnly=true ";
	JVM += "-Dcom.sun.jndi.rmi.object.trustURLCodebase=false ";
	JVM += "-Dcom.sun.jndi.cosnaming.object.trustURLCodebase=false ";
	JVM += "-Dlog4j2.formatMsgNoLookups=true ";

	// Minecraft特定参数
	JVM += "-Dfml.ignoreInvalidMinecraftCertificates=true ";
	JVM += "-Dfml.ignorePatchDiscrepancies=true ";
	JVM += "-Dminecraft.client.jar=" + mcdir + "\\versions\\" + version + "\\" + version + ".jar ";
	JVM += "-Djava.net.useSystemProxies=true ";
	JVM += "-XX:HeapDumpPath=MojangTricksIntelDriversForPerformance_javaw.exe_minecraft.exe.heapdump ";

	// 关键：设置原生库路径
	JVM += "-Djava.library.path=" + natives_dir + " ";

	// 启动器信息
	JVM += "-Dminecraft.launcher.brand=HMCL ";
	JVM += "-Dminecraft.launcher.version=3.13.2 ";

	// classpath
	JVM += "-cp " + classpath + " ";
	printf("[JVM] JVM 参数获取完成\n");
	// 主类
	string main_class = json_get_str(dic, "mainClass");
	if (main_class.empty()) main_class = "net.minecraft.client.main.Main";
	printf("[JVM] 主类: %s\n", main_class.c_str());
	// 游戏参数
	string game_args = main_class + " ";
	game_args += "--version " + version + " ";
	game_args += "--gameDir " + mcdir + " ";
	game_args += "--assetsDir " + mcdir + "\\assets ";

	// 资源索引
	string assetIndexId;
	if (dic.contains("assetIndex") && dic["assetIndex"].is_object())
		assetIndexId = json_get_str(dic["assetIndex"], "id");
	if (assetIndexId.empty()) assetIndexId = version;
	game_args += "--assetIndex " + assetIndexId + " ";

	// 用户认证
	string uuid = GenerateFixedUUID(username);
	game_args += "--uuid " + uuid + " ";
	game_args += "--accessToken " + accessToken + " ";
	game_args += "--userType msa ";
	game_args += "--username " + username + " ";
	// 窗口分辨率
	game_args += "--width 854 ";
	game_args += "--height 480 ";

	printf("[JVM] 游戏参数获取完成\n");
	// 拼接完整命令
	string commandLine = JVM + " " + game_args;

	// 清理多余空格
	regex multiple_spaces("\\s+");
	commandLine = regex_replace(commandLine, multiple_spaces, " ");

	return commandLine;
}