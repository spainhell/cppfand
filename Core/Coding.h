#pragma once
#include "FileD.h"

class Coding
{
public:
	[[nodiscard]] std::string static Code(const std::string& data);

	void static Code(void* data, size_t length);

	[[nodiscard]] std::string static CodingString(FileD* file_d, const std::string& S);

	/// <summary>
	/// Set password in FandTFile (set field PwCode or Pw2Code)
	/// </summary>
	/// <param name="file_d">FileD pointer</param>
	/// <param name="nr">Password type</param>
	/// <param name="passwd">Password</param>
	void static SetPassword(FileD* file_d, uint16_t nr, std::string passwd);

	bool static HasPassword(FileD* file_d, uint16_t nr, const std::string& passwd);

	std::string static XDecode(const std::string& coded_input);
	std::string static XEncode(const std::string& input);

private:
	void static rotateByteLeft(uint8_t& input, size_t count);
	void static rotateByteRight(uint8_t& input, size_t count);
	static bool findCommonSubstr(const std::string& input, size_t index, size_t& offset, size_t& length);
};

