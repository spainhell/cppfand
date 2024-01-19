#include "FandTFilePrefix.h"

#include <cstring>
#include <exception>
#include "../pascal/real48.h"

// nahraje nactenych 512B ze souboru do struktury
void FandTFilePrefix::Load(unsigned char* input512)
{
	size_t index = 0;
	memcpy(&Signum, &input512[index], 2); index += 2;
	memcpy(&OldMaxPage, &input512[index], 2); index += 2;
	memcpy(&FreePart, &input512[index], 4); index += 4;
	memcpy(&Rsrvd1, &input512[index], 1); index++;
	memcpy(&CompileProc, &input512[index], 1); index++;
	memcpy(&CompileAll, &input512[index], 1); index++;
	memcpy(&IRec, &input512[index], 2); index += 2;
	memcpy(&FreeRoot, &input512[index], 4); index += 4;
	memcpy(&MaxPage, &input512[index], 4); index += 4;
	TimeStmp = Real48ToDouble(&input512[index]); index += 6;
	memcpy(&HasCoproc, &input512[index], 1); index++;
	memcpy(&Rsrvd2, &input512[index], 25); index += 25;
	memcpy(&Version, &input512[index], 4); index += 4;
	memcpy(&LicText, &input512[index], 105); index += 105;
	memcpy(&Sum, &input512[index], 1); index++;
	memcpy(&X1, &input512[index], 295); index += 295;
	memcpy(&LicNr, &input512[index], 2); index += 2;
	memcpy(&X2, &input512[index], 11); index += 11;
	memcpy(&PwNew, &input512[index], 40); index += 40;
	memcpy(&Time, &input512[index], 1); index++;
	// index by ted mel mit hodnotu 512
	if (index != 512) throw std::exception("Error in TT1Page::Load");
}

// ulozi strukturu 512B do souboru
void FandTFilePrefix::Save(unsigned char* output512)
{
	size_t index = 0;
	memcpy(&output512[index], &Signum, 2); index += 2;
	memcpy(&output512[index], &OldMaxPage, 2); index += 2;
	memcpy(&output512[index], &FreePart, 4); index += 4;
	memcpy(&output512[index], &Rsrvd1, 1); index++;
	memcpy(&output512[index], &CompileProc, 1); index++;
	memcpy(&output512[index], &CompileAll, 1); index++;
	memcpy(&output512[index], &IRec, 2); index += 2;
	memcpy(&output512[index], &FreeRoot, 4); index += 4;
	memcpy(&output512[index], &MaxPage, 4); index += 4;

	const std::array<unsigned char, 6> time_stamp = DoubleToReal48(TimeStmp);
	for (size_t i = 0; i < 6; i++, index++) {
		output512[index] = time_stamp[i];
	}

	memcpy(&output512[index], &HasCoproc, 1); index++;
	memcpy(&output512[index], &Rsrvd2, 25); index += 25;
	memcpy(&output512[index], &Version, 4); index += 4;
	memcpy(&output512[index], &LicText, 105); index += 105;
	memcpy(&output512[index], &Sum, 1); index++;
	memcpy(&output512[index], &X1, 295); index += 295;
	memcpy(&output512[index], &LicNr, 2); index += 2;
	memcpy(&output512[index], &X2, 11); index += 11;
	memcpy(&output512[index], &PwNew, 40); index += 40;
	memcpy(&output512[index], &Time, 1); index++;
	// index by ted mel mit hodnotu 512
	if (index != 512) throw std::exception("Error in TT1Page::Load");
}
