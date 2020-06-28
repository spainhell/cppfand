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