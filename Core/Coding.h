#pragma once
#include "FileD.h"

class Coding
{
public:
	void static Code(std::string& data);

	void static Code(void* data, size_t length);

	void static CodingLongStr(FileD* file_d, LongStr* S);
	std::string static CodingString(FileD* file_d, std::string& S);

	/// <summary>
	/// Set password in FandTFile (set field PwCode or Pw2Code)
	/// </summary>
	/// <param name="file_d">FileD pointer</param>
	/// <param name="nr">Password type</param>
	/// <param name="passwd">Password</param>
	void static SetPassword(FileD* file_d, WORD nr, std::string passwd);

	bool static HasPassword(FileD* file_d, WORD nr, std::string passwd);

	std::string static XDecode(const std::string& coded_input);
	std::string static XEncode(const std::string& input);

private:
	void static rotateByteLeft(BYTE& input, size_t count);
	void static rotateByteRight(BYTE& input, size_t count);
	static bool findCommonSubstr(const std::string& input, size_t index, size_t& offset, size_t& length);
};

