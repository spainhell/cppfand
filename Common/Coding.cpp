#include "Coding.h"
#include "FileD.h"
#include "textfunc.h"

// Coding::SetPassword and Coding::HasPassword work with FileD, the rest of Coding is in fandbase

void Coding::SetPassword(FileD* file_d, uint16_t nr, std::string passwd)
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

bool Coding::HasPassword(FileD* file_d, uint16_t nr, const std::string& passwd)
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
