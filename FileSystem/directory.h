#pragma once
#include <string>
#include <vector>

using namespace std;

bool directoryExists(const string& path);
string parentDirectory(string path);
vector<string> directoryItems(const string& path, string mask);
