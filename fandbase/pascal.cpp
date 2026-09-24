#include "pascal.h"

#include <cstring>
#include <filesystem>
#include <memory>
#include <stdexcept>

#include "textfunc.h"

void val(pstring s, uint8_t& b, WORD& err)
{
	size_t sz;
	b = std::stoul(s.c_str(), &sz, 10);
	// prelozil se cely retezec?
	if (sz == s.length() - 1) {	err = 0; }
	else { err = static_cast<WORD>(sz); }
}

void val(pstring s, WORD& b, WORD& err)
{
	if (s.empty()) return;
	size_t idx;
	b = std::stoul(s.c_str(), &idx, 10);
	// prelozil se cely retezec?
	if (idx == s.length()) { err = 0; }
	else { err = static_cast<WORD>(idx); }
}

void val(pstring s, short& b, short& err)
{
	if (s.empty()) return;
	size_t idx;
	try {
		b = std::stoi(s.c_str(), &idx, 10);
		// prelozil se cely retezec?
		if (idx == s.length()) { err = 0; }
		else { err = static_cast<short>(idx); }
	}
	catch (std::invalid_argument& e) {
		b = 0;
		err = 1;
	}
}

void val(pstring s, double& b, short& err)
{
	size_t idx = 0;
	try {
		b = std::stod(s.c_str(), &idx);
		// prelozil se cely retezec?
		if (idx == s.length()) { err = 0; }
		else { err = static_cast<short>(idx); }
	}
	catch (std::invalid_argument& e) {
		b = 0;
		err = 1;
	}
}

void val(pstring s, double& b, WORD& err)
{
	if (s.length() == 0) {
		b = 0; err = 0;
		return;
	}
	size_t idx = 0;
	try {
		b = std::stod(s.c_str(), &idx);
		// prelozil se cely retezec?
		if (idx == s.length()) { err = 0; }
		else { err = static_cast<WORD>(idx); }
	}
	catch (std::invalid_argument& e) {
		b = 0;
		err = 1;
	}
}

void val(pstring s, int& b, WORD& err)
{
	if (s.length() == 0) { err = 1;	return;	}
	size_t idx = 0;
	try {
		b = std::stoi(s.c_str(), &idx);
		// prelozil se cely retezec?
		if (idx == s.length()) { err = 0; }
		else { err = static_cast<WORD>(idx); }
	}
	catch (std::invalid_argument& e) {
		b = 0;
		err = 1;
	}
}

void val(pstring s, int& b, short& err)
{
	if (s.length() == 0) { err = 1;	return; }
	size_t idx = 0;
	try {
		b = std::stoi(s.c_str(), &idx);
		// prelozil se cely retezec?
		if (idx == s.length()) { err = 0; }
		else { err = static_cast<WORD>(idx); }
	}
	catch (std::invalid_argument& e) {
		b = 0;
		err = 1;
	}
}

double valDouble(std::string& s, short& err)
{
	if (s.length() == 0) {
		err = 0;
		return 0;
	}
	double result = 0;
	size_t idx = 0;
	try {
		result = std::stod(s.c_str(), &idx);
		// prelozil se cely retezec?
		if (idx == s.length()) { err = 0; }
		else { err = static_cast<WORD>(idx); }
	}
	catch (std::invalid_argument& e) {
		result = 0;
		err = 1;
	}
	return result;
}

pstring copy(pstring source, size_t index, size_t count)
{
	return source.substr(index - 1, count);
}

void str(int input, pstring& output)
{
	std::string a = std::to_string(input);
	output.replace(a.c_str());
}

void str(double input, pstring& output)
{
	char buffer[255];
	snprintf(buffer, sizeof(buffer), "%f", input);
	std::string temp = buffer;
	output = temp;
}

void str(double input, std::string& output)
{
	char buffer[255];
	snprintf(buffer, sizeof(buffer), "%f", input);
	output = buffer;
}

void str(double input, int total, int right, pstring& output)
{
	char buffer[255];
	snprintf(buffer, sizeof(buffer), "%*.*f", total, right, input);
	output = buffer;
}

void str(double input, int total, int right, std::string& output)
{
	char buffer[255];
	snprintf(buffer, sizeof(buffer), "%*.*f", total, right, input);
	output = buffer;
}

void str(double input, int right, pstring& output)
{
	char buffer[255];
	snprintf(buffer, sizeof(buffer), "%.*f", right, input);
	output = buffer;
}

void str(double input, int right, std::string& output)
{
	char buffer[255];
	snprintf(buffer, sizeof(buffer), "%.*f", right, input);
	output = buffer;
}

WORD pred(WORD input)
{
	if (input <= 0) return 0;
	return input - 1;
}

WORD succ(WORD input)
{
	if (input == 0xFFFF) return 0xFFFF;
	return input + 1;
}

void FSplit(const std::string& fullname, std::string& dir, std::string& name, std::string& ext, char pathDelim)
{
	std::filesystem::path pth = fullname;
	bool isDir = is_directory(pth);

	// !!! This causes a crash if there is a directory with the same name as a file !!!
	//if (isDir) {
	//	dir = pth.generic_string();
	//	name = "";
	//	ext = "";
	//}
	//else
	//{
		name = pth.stem().string();
		ext = pth.extension().string();
		dir = pth.generic_string().substr(0, pth.generic_string().length() - name.length() - ext.length());
	//}
	
	if (pathDelim == '/') ReplaceChar(dir, '\\', '/');
	if (pathDelim == '\\') ReplaceChar(dir, '/', '\\');
}

void FillChar(void* cil, int delka, size_t vypln)
{
	memset(cil, vypln, delka);
}

uint8_t Hi(WORD cislo)
{
	return cislo >> 8;
}

uint8_t Lo(WORD cislo)
{
	return cislo & 0x00FF;
}

WORD Swap(WORD cislo)
{
	return ((cislo & 0x00FF) << 4) + (cislo >> 4);
}

std::string GetEnv(const char* name)
{
	std::string result;
	size_t requiredSize = 0;
	getenv_s(&requiredSize, NULL, 0, name);
	if (requiredSize == 0) {
		result = "";
	}
	else {
		std::unique_ptr<char[]> buffer = std::make_unique<char[]>(requiredSize * sizeof(char));
		getenv_s(&requiredSize, buffer.get(), requiredSize, name);
		result = std::string(buffer.get());
	}
	return result;
}
