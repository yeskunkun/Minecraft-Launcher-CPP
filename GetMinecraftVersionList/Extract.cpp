#include "zip_file.hpp"
#include "Extract.h"
bool CreateDirectoryRecursive(const string& path) {
	if (path.empty()) return true;
	string p = path;
	for (char& c : p) if (c == '/') c = '\\';
	while (!p.empty() && (p.back() == '\\' || p.back() == '/')) {
		p.pop_back();
	}
	if (p.empty()) {
		return true;
	}
	DWORD attr = GetFileAttributesA(p.c_str());
	if (attr & FILE_ATTRIBUTE_DIRECTORY) {
		return true;
	}
	size_t pos = p.find_last_of("\\/");
	if (pos != -1) {
		CreateDirectoryRecursive(p.substr(0, pos));
	}
	if (!CreateDirectoryA(p.c_str(), 0)) {
		if (GetLastError() != ERROR_ALREADY_EXISTS) {
			return false;
		}
	}
	return true;
}
bool Extract(const string& zip_path, const string& dest_dir) {
	printf("[下载] zipPath: %s, destDir: %s\n", zip_path.c_str(), dest_dir.c_str());
	mz_zip_archive zip{};
	if (!mz_zip_reader_init_file(&zip, zip_path.c_str(), 0)) {
		printf("[下载] 错误：无法打开文件 '%s'。\n", zip_path.c_str());
		return false;
	}
	size_t num_files = mz_zip_reader_get_num_files(&zip);
	printf("[下载] 正在分析...\n");
	size_t total_size = 0;
	for (size_t i = 0; i < num_files; ++i) {
		mz_zip_archive_file_stat stat;
		if (!mz_zip_reader_file_stat(&zip, i, &stat)) {
			mz_zip_reader_end(&zip);
			printf("[下载] 错误：无法获取文件 #%zu\n", i);
			return false;
		}
		if (stat.m_filename[strlen(stat.m_filename) - 1] != '/') {
			total_size += stat.m_uncomp_size;
		}
	}
	printf("\r%20s\r", "");
	CreateDirectoryRecursive(dest_dir);
	ExtractContext ctx;
	ctx.total_size = total_size;
	ctx.total_extracted = 0;
	for (size_t i = 0; i < num_files; ++i) {
		mz_zip_archive_file_stat stat;
		if (!mz_zip_reader_file_stat(&zip, i, &stat)) {
			mz_zip_reader_end(&zip);
			printf("[下载] 错误：无法获取文件 #%zu\n", i);
			return false;
		}
		string filename(stat.m_filename), out_path = dest_dir + "\\" + filename;
		if (!filename.empty() && filename.back() == '/') {
			CreateDirectoryRecursive(out_path);
			continue;
		}
		CreateDirectoryRecursive(out_path.substr(0, out_path.find_last_of("\\/")));
		HANDLE hFile = CreateFileA(out_path.c_str(), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		ctx.hFile = hFile;
		ctx.current_file_size = stat.m_uncomp_size;
		ctx.current_extracted = 0;
		ctx.display_name = filename;
		if (filename.length() > 16) {
			ctx.display_name = filename.substr(0, 13) + "...";
		}
		ctx.ok = true;
		if (!mz_zip_reader_extract_to_callback(&zip, i, [](void* opaque, size_t ofs, const void* buf, size_t n) -> size_t {
			ExtractContext* ctx = static_cast<ExtractContext*>(opaque);
			if (ctx->hFile != INVALID_HANDLE_VALUE) {
				DWORD wr = 0;
				if (!WriteFile(ctx->hFile, buf, static_cast<DWORD>(n), &wr, 0) || wr != n) {
					ctx->ok = false;
					return 0;  // 返回0终止解压
				}
			}
			ctx->current_extracted += n;
			ctx->total_extracted += n;
			char progress_buf[32]{};
			sprintf_s(progress_buf, sizeof(progress_buf), "%.2f", ctx->total_size ? (100.0 * ctx->total_extracted / ctx->total_size) : 0.0);
			string display = "解压：" + string(progress_buf) + "%\t" + ctx->display_name;
			if (display.length() < 34) {
				display.append(34 - display.length(), ' ');
			}
			printf("\r%s", display.c_str());
			return n; }, &ctx, 0) || !ctx.ok) {
			CloseHandle(hFile);
			mz_zip_reader_end(&zip);
			printf("[下载] 错误：解压文件 '%s' 失败\n", filename.c_str());
			return false;
		}
		CloseHandle(hFile);
	}
	mz_zip_reader_end(&zip);
	printf("[下载] 解压：100.00%%\t解压操作成功完成。\n");
	return true;
}
