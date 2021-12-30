#include "directory.h"
#include <filesystem>
#include <iostream>

#include "../exprcmp/exprcmp.h"
#include "../textfunc/textfunc.h"

namespace fs = std::filesystem;

int fileExists(const string& path)
{
	int result;
	if (fs::exists(path)) result = 0;
	else {
		// directory exists?
		fs::path p = path;
		p.remove_filename();
		if (fs::exists(p)) {
			// file not exists
			result = 1;
		}
		else {
			// dictionary not exists
			result = 2;
		}
	}
	return result;
}

bool directoryExists(const string& path)
{
	return fs::exists(path);
}

string parentDirectory(string path)
{
	if (path[path.length() - 1] == '\\') path.erase(path.length() - 1, 1);
	fs::path p = path;
	fs::path pp = p.parent_path();
	auto result = pp.string();
	if (result[result.length() - 1] == '\\') return result;
	return result + '\\';
}

string getDirectory(string fullPath, char pathDelim)
{
	fs::path p = fullPath;
	p.remove_filename();

	auto dir = p.generic_string();
	if (pathDelim == '/') ReplaceChar(dir, '\\', '/');
	if (pathDelim == '\\') ReplaceChar(dir, '/', '\\');

	return dir;
}

vector<string> directoryItems(const string& path, string mask)
{
	vector<string> result;
	if (path != parentDirectory(path)) result.push_back("\\..");
	for (const auto& entry : fs::directory_iterator(path)) {
		auto fileName = entry.path().filename().string();
		if (entry.is_directory()) {
			result.push_back("\\" + fileName);
		}
		else {
			if (CmpStringWithMask(fileName, mask)) {
				result.push_back(fileName);
			}
		}
	}
	if (result.size() > 1) {
		sort(result.begin() + 1, result.end());
	}
	return result;
}

bool deleteFile(const string& path)
{
	return fs::remove(path);
}
