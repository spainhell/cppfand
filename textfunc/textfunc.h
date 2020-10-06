#pragma once
#include <vector>
#include <string>

std::vector<std::string> GetAllRows(std::string input);
std::string TrailChar(std::string& input, char c);
std::string LeadChar(std::string& input, char c);
std::string GetNthLine(std::string& input, size_t from, size_t count);
std::string GetStyledStringOfLength(std::string& input, size_t length);