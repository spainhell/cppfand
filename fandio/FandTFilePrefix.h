#pragma once
class FandTFilePrefix
{
public:
	unsigned short Signum = 0;
	unsigned short OldMaxPage = 0;
	int FreePart = 0;
	bool Rsrvd1 = false, CompileProc = false, CompileAll = false;
	unsigned short IRec = 0;
	// potud se to nekoduje (13B)
	// odtud jsou polozky prohnany XORem
	int FreeRoot = 0, MaxPage = 0;   /*eldest version=>array Pw[1..40] of char;*/
	double TimeStmp = 0.0;
	bool HasCoproc = false;
	char Rsrvd2[25]{ '\0' };
	char Version[4]{ '\0' };
	unsigned char LicText[105]{ 0 };
	unsigned char Sum = 0;
	char X1[295]{ '\0' };
	unsigned short LicNr = 0;
	char X2[11]{ '\0' };
	char PwNew[40]{ '\0' };
	unsigned char Time = 0;

	void Load(unsigned char* input512);
	void Save(unsigned char* output512);
};

