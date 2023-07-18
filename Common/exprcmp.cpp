#include "exprcmp.h"
#include <regex>

bool CmpStringWithMask(const std::string& value, std::string mask)
{
	mask = RegexFromString(mask);
	std::regex self_regex(mask, std::regex_constants::icase);
	if (std::regex_search(value, self_regex)) {
		return true;
	}
	return false;
}

std::string RegexFromString(std::string mask)
{
	if (mask.length() == 0) return "";

	// puvodni "\" je v reg. vyrazu r"\\"
	size_t index = mask.find('\\'); /* \ -> \\ */
	while (index != std::string::npos) {
		mask = mask.replace(index, 1, "\\\\");
		index = mask.find('\\', index + 2);
	}

	// puvodni "[" je v reg. vyrazu r"\["
	index = mask.find('['); // { -> \[
	while (index != std::string::npos) {
		mask = mask.replace(index, 1, "\\[");
		index = mask.find('[', index + 2);
	}

	// puvodni "]" je v reg. vyrazu r"\]"
	index = mask.find(']'); // ] -> \]
	while (index != std::string::npos) {
		mask = mask.replace(index, 1, "\\]");
		index = mask.find(']', index + 2);
	}

	// puvodni "{" je v reg. vyrazu r"\{"
	index = mask.find('{'); // { -> \{
	while (index != std::string::npos) {
		mask = mask.replace(index, 1, "\\{");
		index = mask.find('{', index + 2);
	}

	// puvodni "}" je v reg. vyrazu r"\}"
	index = mask.find('}'); // } -> \}
	while (index != std::string::npos) {
		mask = mask.replace(index, 1, "\\}");
		index = mask.find('}', index + 2);
	}

	// puvodni "(" je v reg. vyrazu r"\("
	index = mask.find('('); // ( -> \(
	while (index != std::string::npos) {
		mask = mask.replace(index, 1, "\\(");
		index = mask.find('(', index + 2);
	}

	// puvodni ")" je v reg. vyrazu r"\)"
	index = mask.find(')'); // ) -> \)
	while (index != std::string::npos) {
		mask = mask.replace(index, 1, "\\)");
		index = mask.find(')', index + 2);
	}

	// puvodni "." je v reg. vyrazu r"\."
	index = mask.find('.'); // . -> \.
	while (index != std::string::npos) {
		mask = mask.replace(index, 1, "\\.");
		index = mask.find('.', index + 2);
	}

	// puvodni otaznik je nahrazen r"." - jakykoliv 1 znak
	index = mask.find('?'); // ? -> .
	while (index != std::string::npos) {
		mask[index] = '.';
		index = mask.find('?', index);
	}

	// puvodni hvezdicka je nahrazena r".*" - jakekoliv znaky
	index = mask.find('*'); // * -> .*
	while (index != std::string::npos) {
		mask = mask.replace(index, 1, ".*");
		index = mask.find('*', index + 2);
	}
	
	return "^" + mask + "$";
}

bool FindShiftCtrlAltFxx(std::string input, std::string& key, unsigned char& fnKeyNr)
{
	std::regex self_regex("^(shift|ctrl|alt|)f(\\d{1,2})$", std::regex_constants::icase);
	std::smatch sm;
	if (std::regex_search(input, sm, self_regex)) {
		key = sm[1];
		for (size_t i = 0; i < key.length(); i++) key[i] = (char)tolower(key[i]);
		fnKeyNr = (unsigned char)std::stoi(sm[2]);
		return true;
	}
	return false;
}
