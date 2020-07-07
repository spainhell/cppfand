#include "exprcmp.h"
#include <regex>

bool CmpStringWithMask(std::string Value, std::string Mask)
{
	Mask = RegexFromString(Mask);
	std::regex self_regex(Mask);
	if (std::regex_search(Value, self_regex)) {
		return true;
	}
	return false;
}

std::string RegexFromString(std::string Mask)
{
	if (Mask.length() == 0) return "";

	// puvodni "." je v reg. vyrazu r"\."
	size_t index = Mask.find('.'); // . -> \.
	while (index != std::string::npos) {
		Mask = Mask.replace(index, 1, "\\.");
		index = Mask.find('.', index + 2);
	}

	// puvodni otaznik je nahrazen r"." - jakokoliv 1 znak
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
	
	return Mask;
}
