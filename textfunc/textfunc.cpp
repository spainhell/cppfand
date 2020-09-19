#include "pch.h"
#include "textfunc.h"

std::vector<std::string> GetAllRows(std::string input)
{
	std::vector<std::string> vStr;
	size_t nextStart = 0;
	for (size_t i = 0; i < input.length(); i++)
	{
		if (input[i] == '\r' || input[i] == '\n') {
			size_t nl = 1; // kolik znaku ma ukonceni?
			// narazili jsme na konec radku
			if (i < input.length() - 1 && input[i] == '\r' && input[i + 1] == '\n') {
				// jedna se o CR+LF
				nl++;
				i++;
			}
			std::string nStr = input.substr(nextStart, i - nextStart + 1 - nl);
			vStr.push_back(nStr);
			nextStart = i + 1;
		}
	}
	std::string nStr = input.substr(nextStart, input.length() - nextStart);
	vStr.push_back(nStr);
	return vStr;
}

/// odstrani znaky na konci retezce
std::string TrailChar(std::string& input, char c)
{
	size_t Count = 0;
	for (int i = (int)input.length() - 1; i >= 0; i--) {
		if (input[i] == c) Count++;
		else break;
	}
	return input.substr(0, input.length() - Count);
}

/// odstrani znaky na zacatku retezce
std::string LeadChar(std::string& input, char c)
{
	size_t startIndex = 0;
	for (size_t i = 0; i < input.length(); i++) {
		if (input[i] == c) startIndex++;
		else break;
	}
	return input.substr(startIndex);
}
