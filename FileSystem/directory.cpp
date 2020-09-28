#include "directory.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

vector<string> directoryItems(const string& path)
{
    string d = ".";
    vector<string> result;
    for (const auto& entry : std::filesystem::directory_iterator(d))
        if (entry.is_directory()) {
            result.push_back(entry.path().string());
        }
        else {
        	
            result.push_back(entry.path().filename().string());
        }
    return result;
}
