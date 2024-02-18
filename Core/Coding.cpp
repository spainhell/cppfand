#include "Coding.h"

#include "access.h"
#include "../Common/textfunc.h"

void Coding::Code(std::string& data)
{
	for (char& i : data) {
		i = static_cast<char>(i ^ 0xAA);
	}
}

void Coding::Code(void* data, size_t length)
{
	BYTE* pb = static_cast<BYTE*>(data);
	for (size_t i = 0; i < length; i++) {
		pb[i] = pb[i] ^ 0xAA;
	}
}

void Coding::CodingLongStr(FileD* file_d, LongStr* S)
{
	if (file_d->FF->TF->LicenseNr == 0) {
		Code(S->A, S->LL);
	}
	else {
		std::string coded_input = std::string(S->A, S->LL);
		std::string decoded_input = Coding::XDecode(coded_input);
		S->LL = decoded_input.size();
		memcpy(S->A, decoded_input.c_str(), decoded_input.length());
	}
}

std::string Coding::CodingString(FileD* file_d, std::string& S)
{
	if (file_d->FF->TF->LicenseNr == 0) {
		Code(S);
		return "";
	}
	else {
		return Coding::XDecode(S);
	}
}

void Coding::SetPassword(FileD* file_d, WORD nr, std::string passwd)
{
	if (nr == 1) {
		file_d->FF->TF->PwCode = passwd;
		file_d->FF->TF->PwCode = AddTrailChars(file_d->FF->TF->PwCode, '@', 20);
		Code(file_d->FF->TF->PwCode);
	}
	else {
		file_d->FF->TF->Pw2Code = passwd;
		file_d->FF->TF->PwCode = AddTrailChars(file_d->FF->TF->Pw2Code, '@', 20);
		Code(file_d->FF->TF->Pw2Code);
	}
}

bool Coding::HasPassword(FileD* file_d, WORD nr, std::string passwd)
{
	std::string filePwd;
	if (nr == 1) {
		filePwd = file_d->FF->TF->PwCode;
		Code(filePwd);
	}
	else {
		filePwd = file_d->FF->TF->Pw2Code;
		Code(filePwd);
	}
	return passwd == TrailChar(filePwd, '@');
}

std::string Coding::XDecode(const std::string& coded_input)
{
	std::string output;
	output.reserve(coded_input.length());

	// get last two characters (first byte in the string is lower!):
	WORD offset = static_cast<WORD>(static_cast<BYTE>(coded_input[coded_input.length() - 1]) << 8);
	offset += static_cast<BYTE>(coded_input[coded_input.length() - 2]);
	offset = offset ^ 0xCCCC;

	// rotate count is on 3rd position from the end:
	BYTE rotate_count = coded_input[coded_input.length() - 3];
	rotate_count = rotate_count & 0x03;

	// rotate mask is 0x9C << rotate_count
	BYTE mask = 0x9C;
	rotateByteLeft(mask, rotate_count);

	size_t index = offset;
	WORD salt = static_cast<WORD>(0xFF00) + static_cast<BYTE>(coded_input[index++]);

	while (index < coded_input.length() - 3) { // there is 2B offset on the end (0xCCCC)
		if ((salt & 0b0000000100000000) == 0) {
			// there is a new value for DX in the next position
			salt = static_cast<WORD>(0xFF00) + static_cast<BYTE>(coded_input[index++]);
		}
		else if ((salt & 0b0000000000000001) != 0) {
			// a compression - repeat already decoded characters
			const BYTE repeats = coded_input[index++];
			offset = static_cast<BYTE>(coded_input[index++]);
			offset += static_cast<WORD>(static_cast<BYTE>(coded_input[index++]) << 8);
			for (BYTE i = 0; i < repeats; i++) {
				output += output[offset - 2];
				offset++;
			}			
			salt = salt >> 1;
		}
		else {
			BYTE next = coded_input[index++];
			rotateByteLeft(mask, 1);
			next = next ^ mask;
			output += static_cast<char>(next);
			salt = salt >> 1;
		}
	}

	return output;
}

std::string Coding::XEncode(const std::string& input)
{
	std::string output;

	WORD offset = 0;
	BYTE timer = 0xA3;
	BYTE mask = 0x9C;
	const BYTE rotate_count = timer & 0x03;

	rotateByteLeft(mask, rotate_count);

	// Initialize variables for encoding loop
	WORD salt = 0xFF00;
	output += static_cast<char>(salt & 0xFF);
	size_t last_salt_index = output.size() - 1;

	size_t index = 0;
	while (index < input.length()) {
		if ((salt & 0b0000000100000000) == 0) {
			salt = 0xFF00;
			output += static_cast<char>(salt & 0xFF);
			last_salt_index = output.size() - 1;
		}
		else {
			size_t start = std::string::npos;
			size_t len = std::string::npos;

			if ((input.length() > index + 2) && findCommonSubstr(input, index, start, len)) {
				// update last salt
				const BYTE diff = static_cast<BYTE>(output.length() - 1 - last_salt_index);
				output[last_salt_index] = static_cast<BYTE>(0x01 << diff);

				if (len > 0xFF) {
					len = 0xFF; // max. length is 255
				}

				output += static_cast<BYTE>(len);
				output += static_cast<BYTE>((start + 2) & 0xFF); // in original PC FAND it points behind array length (1st 2 Bytes)
				output += static_cast<BYTE>((start + 2) >> 8);
				index += len;

				salt = salt >> 1;
			}
			else {
				BYTE next = input[index++];
				rotateByteLeft(mask, 1);
				next = next ^ mask;
				output += static_cast<char>(next);
				salt = salt >> 1;
			}
		}
	}

	// write timer value
	output += timer;

	// write offset
	offset = offset ^ 0xCCCC;
	output += static_cast<BYTE>(offset & 0xFF);
	output += static_cast<BYTE>(offset >> 8);

	return output;
}

void Coding::rotateByteLeft(BYTE& input, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		input = static_cast<BYTE>(input << 1) | static_cast<BYTE>(input >> 7);
	}
}

void Coding::rotateByteRight(BYTE& input, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		input = static_cast<BYTE>(input >> 1) | static_cast<BYTE>(input << 7);
	}
}

bool Coding::findCommonSubstr(const std::string& input, size_t index, size_t& offset, size_t& length) {
	// Get the lengths of the input strings
	const std::string processed = input.substr(0, index);
	const std::string unprocessed = input.substr(index);
	const size_t proc_len = processed.length();
	const size_t unproc_len = unprocessed.length();

	if (proc_len == 0 || unproc_len == 0) return false;

	size_t common = min(proc_len, unproc_len);
	size_t local_index = std::string::npos;

	while (common > 2) {
		local_index = processed.find(unprocessed.substr(0, common));
		if (local_index != std::string::npos) {
			break;
		}
		else {
			common--;
		}
	}

	if (common > 2) {
		offset = local_index;
		length = common;
		return true;
	}
	else {
		return false;
	}
}
