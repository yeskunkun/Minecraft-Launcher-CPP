#pragma once
#include <windows.h>
#include <cstdio>
#include <string>
#include <iostream>
using namespace std;
struct ExtractContext {
	HANDLE hFile = INVALID_HANDLE_VALUE;
	size_t current_file_size = 0, current_extracted = 0, total_extracted = 0, total_size = 0;
	string display_name;
	bool ok = true;
};
bool CreateDirectoryRecursive(const string& path);
bool Extract(const string& zip_path, const string& dest_dir);