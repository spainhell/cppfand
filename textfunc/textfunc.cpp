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

/// cislo radku zacina 1 .. N, pocet 1 .. N
std::string GetNthLine(std::string& input, size_t from, size_t count)
{
	if (from < 1 || count < 1) return "";
	size_t startIndex = 0;
	size_t stopIndex = input.find('\n');
	for (size_t i = 1; i < from; i++) {
		startIndex = stopIndex + 1;
		stopIndex = input.find('\n', stopIndex + 1);
	}
	for (size_t i = 1; i < count; i++) {
		// je pozadovano vice radku, budeme navysovat stop index
		stopIndex = input.find('\n', stopIndex + 1);
	}

	return input.substr(startIndex, stopIndex - startIndex - 1);
}

/// vytvori ze vstupu formatovany retez o maximalni delce znaku
std::string GetStyledStringOfLength(std::string& input, size_t length)
{
	if (length == 0) return "";
	if (input.length() <= length) return input;
	std::string result;
	size_t charsInserted = 0;
	for (size_t i = 0; i < input.length(); i++)
	{
		char c = input[i];
		result += c;
		if (!(c == 0x13 || c == 0x17 || c == 0x11 || c == 0x04
			|| c == 0x02 || c == 0x05 || c == 0x01)) charsInserted++;
		if (charsInserted >= result.length()) break;
	}
	return result;
}
