#pragma once
#include <string>
#include <vector>

using namespace std;

int fileExists(const string& path);
bool directoryExists(const string& path);
string parentDirectory(string path);
string getDirectory(string fullPath);
vector<string> directoryItems(const string& path, string mask);
