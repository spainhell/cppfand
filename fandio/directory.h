#pragma once
#include <string>
#include <vector>

using namespace std;

int fileExists(const string& path);
bool directoryExists(const string& path);
string parentDirectory(string path);
string getDirectory(string fullPath, char pathDelim = '\\');
vector<string> directoryItems(const string& path_with_mask);
vector<string> directoryItems(const string& path, string mask, bool add_parent_dir);
bool deleteFile(const string& path);
time_t lastWriteTime(const string& path);
