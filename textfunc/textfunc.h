#pragma once
#include "pch.h"
#include <vector>
#include <string>

std::vector<std::string> GetAllRows(std::string input);
std::string TrailChar(std::string& input, char c);
std::string LeadChar(std::string& input, char c);
