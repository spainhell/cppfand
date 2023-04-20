#pragma once
#include "FileD.h"

class Coding
{
public:
	void static Code(std::string& data);

	void static Code(void* A, WORD L);

	void static CodingLongStr(FileD* file_d, LongStr* S);

	/// <summary>
	/// Set password in FandTFile (set field PwCode or Pw2Code)
	/// </summary>
	/// <param name="file_d">FileD pointer</param>
	/// <param name="nr">Password type</param>
	/// <param name="passwd">Password</param>
	void static SetPassword(FileD* file_d, WORD nr, std::string passwd);

	bool static HasPassword(FileD* file_d, WORD nr, std::string passwd);

	void static XDecode(LongStr* origS);

private:
	void static rotateByteLeft(BYTE& input, size_t count);
};

