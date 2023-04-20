#include "Coding.h"
#include "access.h"
#include "../Common/textfunc.h"

void Coding::Code(std::string& data)
{
	for (char& i : data) {
		i = (char)(i ^ 0xAA);
	}
}

void Coding::Code(void* A, WORD L)
{
	BYTE* pb = (BYTE*)A;
	for (int i = 0; i < L; i++) {
		pb[i] = pb[i] ^ 0xAA;
	}
}

void Coding::CodingLongStr(FileD* file_d, LongStr* S)
{
	if (file_d->FF->TF->LicenseNr == 0) Code(S->A, S->LL);
	else Coding::XDecode(S); // musi mit o 2B vic - saha si tam XDecode!!!
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

void Coding::XDecode(LongStr* origS)
{
	BYTE RMask = 0;
	WORD* AX = new WORD();
	BYTE* AL = (BYTE*)AX;
	BYTE* AH = AL + 1;

	WORD* BX = new WORD();
	BYTE* BL = (BYTE*)BX;
	BYTE* BH = BL + 1;

	WORD* CX = new WORD();
	BYTE* CL = (BYTE*)CX;
	BYTE* CH = CL + 1;

	WORD* DX = new WORD();
	BYTE* DL = (BYTE*)DX;
	BYTE* DH = DL + 1;

	if (origS->LL == 0) return;
	BYTE* S = new BYTE[origS->LL + 2];
	WORD* len = (WORD*)&S[0];
	*len = origS->LL;
	memcpy(&S[2], &origS->A[0], origS->LL);

	WORD offset = 0;

	BYTE* DS = S;     // ukazuje na delku pred retezcem
	BYTE* ES = &S[2]; // ukazuje na zacatek retezce
	WORD SI = 0;
	*BX = SI;
	*AX = *(WORD*)&DS[SI];
	if (*AX == 0) return;
	SI = 2;
	WORD DI = 0;
	*BX += *AX;
	*AX = *(WORD*)&S[*BX];
	*AX = *AX ^ 0xCCCC;
	SI += *AX;
	(*BX)--;
	*CL = S[*BX];
	*CL = *CL & 3;
	*AL = 0x9C;
	rotateByteLeft(*AL, *CL);
	RMask = *AL;
	*CH = 0;

label1:
	*AL = DS[SI]; SI++; // lodsb
	*DH = 0xFF;
	*DL = *AL;

label2:
	if (SI >= *BX) goto label4;
	if ((*DH & 1) == 0) goto label1;
	if ((*DL & 1) != 0) goto label3;
	*AL = DS[SI]; SI++; // lodsb
	rotateByteLeft(RMask, 1);
	*AL = *AL ^ RMask;
	ES[DI] = *AL; DI++; // stosb
	*DX = *DX >> 1;
	goto label2;

label3:
	*AL = DS[SI]; SI++; // lodsb
	*CL = *AL;
	*AX = *(WORD*)&DS[SI]; SI++; SI++; // lodsw
	offset = *AX;
	for (size_t i = 0; i < *CX; i++) { // rep
		ES[DI] = DS[offset]; offset++; DI++; // movsb
	}
	*DX = *DX >> 1;
	goto label2;

label4:
	origS->LL = DI;
	memcpy(origS->A, &S[2], DI);

	delete AX; delete BX; delete CX; delete DX;
	delete[] S;
}

void Coding::rotateByteLeft(BYTE& input, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		input = input << 1 | input >> 7;
	}
}
