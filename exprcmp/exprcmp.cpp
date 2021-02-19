#include "exprcmp.h"
#include <regex>

bool CmpStringWithMask(std::string Value, std::string Mask)
{
	Mask = RegexFromString(Mask);
	std::regex self_regex(Mask, std::regex_constants::icase);
	if (std::regex_search(Value, self_regex)) {
		return true;
	}
	return false;
}

std::string RegexFromString(std::string Mask)
{
	if (Mask.length() == 0) return "";

	// puvodni "\" je v reg. vyrazu r"\\"
	size_t index = Mask.find('\\'); /* \ -> \\ */
	while (index != std::string::npos) {
		Mask = Mask.replace(index, 1, "\\\\");
		index = Mask.find('\\', index + 2);
	}

	// puvodni "[" je v reg. vyrazu r"\["
	index = Mask.find('['); // { -> \[
	while (index != std::string::npos) {
		Mask = Mask.replace(index, 1, "\\[");
		index = Mask.find('[', index + 2);
	}

	// puvodni "]" je v reg. vyrazu r"\]"
	index = Mask.find(']'); // ] -> \]
	while (index != std::string::npos) {
		Mask = Mask.replace(index, 1, "\\]");
		index = Mask.find(']', index + 2);
	}

	// puvodni "{" je v reg. vyrazu r"\{"
	index = Mask.find('{'); // { -> \{
	while (index != std::string::npos) {
		Mask = Mask.replace(index, 1, "\\{");
		index = Mask.find('{', index + 2);
	}

	// puvodni "}" je v reg. vyrazu r"\}"
	index = Mask.find('}'); // } -> \}
	while (index != std::string::npos) {
		Mask = Mask.replace(index, 1, "\\}");
		index = Mask.find('}', index + 2);
	}

	// puvodni "." je v reg. vyrazu r"\."
	index = Mask.find('.'); // . -> \.
	while (index != std::string::npos) {
		Mask = Mask.replace(index, 1, "\\.");
		index = Mask.find('.', index + 2);
	}

	// puvodni otaznik je nahrazen r"." - jakykoliv 1 znak
	index = Mask.find('?'); // ? -> .
	while (index != std::string::npos) {
		Mask[index] = '.';
		index = Mask.find('?', index);
	}

	// puvodni hvezdicka je nahrazena r".*" - jakekoliv znaky
	index = Mask.find('*'); // * -> .*
	while (index != std::string::npos) {
		Mask = Mask.replace(index, 1, ".*");
		index = Mask.find('*', index + 2);
	}
	
	return "^" + Mask + "$";
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
