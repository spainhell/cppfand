#include "directory.h"
#include <filesystem>
#include <iostream>
#include <chrono>

#include "../exprcmp/exprcmp.h"
#include "../textfunc/textfunc.h"

namespace fs = std::filesystem;
using namespace std::chrono_literals;

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

template <typename TP>
std::time_t to_time_t(TP tp)
{
	using namespace std::chrono;
	auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
		+ system_clock::now());
	return system_clock::to_time_t(sctp);
}

time_t lastWriteTime(const string& path)
{
	if (fileExists(path) == 0) {
		auto ftime = fs::last_write_time(path);
		return to_time_t(ftime);
	}
	else {
		return 0;
	}
}