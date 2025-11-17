#include "Coding.h"

#include "access.h"
#include "../Common/textfunc.h"

std::string Coding::Code(const std::string& data)
{
	std::string result;
	for (const char i : data) {
		result += static_cast<char>(i ^ 0xAA);
	}
	return result;
}

void Coding::Code(void* data, size_t length)
{
	uint8_t* pb = static_cast<uint8_t*>(data);
	for (size_t i = 0; i < length; i++) {
		pb[i] = pb[i] ^ 0xAA;
	}
}

std::string Coding::CodingString(FileD* file_d, const std::string& S)
{
	if (file_d->FF->TF->LicenseNr == 0) {
		return Code(S);
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
		file_d->FF->TF->PwCode = Code(file_d->FF->TF->PwCode);
	}
	else {
		file_d->FF->TF->Pw2Code = passwd;
		file_d->FF->TF->PwCode = AddTrailChars(file_d->FF->TF->Pw2Code, '@', 20);
		file_d->FF->TF->PwCode = Code(file_d->FF->TF->Pw2Code);
	}
}

bool Coding::HasPassword(FileD* file_d, WORD nr, const std::string& passwd)
{
	std::string filePwd;
	if (nr == 1) {
		filePwd = file_d->FF->TF->PwCode;
		filePwd = Code(filePwd);
	}
	else {
		filePwd = file_d->FF->TF->Pw2Code;
		filePwd = Code(filePwd);
	}
	return passwd == TrailChar(filePwd, '@');
}

std::string Coding::XDecode(const std::string& coded_input)
{
	std::string output;
	output.reserve(coded_input.length());

	// get last two characters (first byte in the string is lower!):
	WORD offset = static_cast<WORD>(static_cast<uint8_t>(coded_input[coded_input.length() - 1]) << 8);
	offset += static_cast<uint8_t>(coded_input[coded_input.length() - 2]);
	offset = offset ^ 0xCCCC;

	// rotate count is on 3rd position from the end:
	uint8_t rotate_count = coded_input[coded_input.length() - 3];
	rotate_count = rotate_count & 0x03;

	// rotate mask is 0x9C << rotate_count
	uint8_t mask = 0x9C;
	rotateByteLeft(mask, rotate_count);

	size_t index = offset;
	WORD salt = static_cast<WORD>(0xFF00) + static_cast<uint8_t>(coded_input[index++]);

	while (index < coded_input.length() - 3) { // there is 2B offset on the end (0xCCCC)
		if ((salt & 0b0000000100000000) == 0) {
			// there is a new value for DX in the next position
			salt = static_cast<WORD>(0xFF00) + static_cast<uint8_t>(coded_input[index++]);
		}
		else if ((salt & 0b0000000000000001) != 0) {
			// a compression - repeat already decoded characters
			const uint8_t repeats = coded_input[index++];
			offset = static_cast<uint8_t>(coded_input[index++]);
			offset += static_cast<WORD>(static_cast<uint8_t>(coded_input[index++]) << 8);
			for (uint8_t i = 0; i < repeats; i++) {
				output += output[offset - 2];
				offset++;
			}			
			salt = salt >> 1;
		}
		else {
			uint8_t next = coded_input[index++];
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
	uint8_t timer = 0xA3;
	uint8_t mask = 0x9C;
	const uint8_t rotate_count = timer & 0x03;

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
				const uint8_t diff = static_cast<uint8_t>(output.length() - 1 - last_salt_index);
				output[last_salt_index] = static_cast<uint8_t>(0x01 << diff);

				if (len > 0xFF) {
					len = 0xFF; // max. length is 255
				}

				output += static_cast<uint8_t>(len);
				output += static_cast<uint8_t>((start + 2) & 0xFF); // in original PC FAND it points behind array length (1st 2 Bytes)
				output += static_cast<uint8_t>((start + 2) >> 8);
				index += len;

				salt = salt >> 1;
			}
			else {
				uint8_t next = input[index++];
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
	output += static_cast<uint8_t>(offset & 0xFF);
	output += static_cast<uint8_t>(offset >> 8);

	return output;
}

void Coding::rotateByteLeft(uint8_t& input, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		input = static_cast<uint8_t>(input << 1) | static_cast<uint8_t>(input >> 7);
	}
}

void Coding::rotateByteRight(uint8_t& input, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		input = static_cast<uint8_t>(input >> 1) | static_cast<uint8_t>(input << 7);
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
