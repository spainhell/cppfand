#include "TFile.h"


#include "FieldDescr.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "obaseww.h"
#include "../Logging/Logging.h"
#include "../pascal/random.h"
#include "../textfunc/textfunc.h"

struct TT1Page
{
	WORD Signum = 0;
	WORD OldMaxPage = 0;
	longint FreePart = 0;
	bool Rsrvd1 = false, CompileProc = false, CompileAll = false;
	WORD IRec = 0;
	// potud se to nekoduje (13B)
	// odtud jsou polozky prohnany XORem
	longint FreeRoot = 0, MaxPage = 0;   /*eldest version=>array Pw[1..40] of char;*/
	double TimeStmp = 0.0;
	bool HasCoproc = false;
	char Rsrvd2[25]{ '\0' };
	char Version[4]{ '\0' };
	BYTE LicText[105]{ 0 };
	BYTE Sum = 0;
	char X1[295]{ '\0' };
	WORD LicNr = 0;
	char X2[11]{ '\0' };
	char PwNew[40]{ '\0' };
	BYTE Time = 0;

	void Load(BYTE* input512);
	void Save(BYTE* output512);
};

// nahraje nactenych 512B ze souboru do struktury
void TT1Page::Load(BYTE* input512)
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
	memcpy(&TimeStmp, &input512[index], 6); index += 6;
	memset(&TimeStmp + 6, 0, 2); // v pascalu to bylo 6B, tady je to 8B
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

// nahraje nactenych 512B ze souboru do struktury
void TT1Page::Save(BYTE* output512)
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
	memcpy(&output512[index], &TimeStmp, 6); index += 6;
	//memset(&TimeStmp + 6, 0, 2); // v pascalu to bylo 6B, tady je to 8B
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

void RandIntByBytes(longint& nr)
{
	BYTE* byte = (BYTE*)&nr;
	for (size_t i = 0; i < 4; i++)
	{
		byte[i] = byte[i] ^ Random(255); //randomice6s4173[randIndex++];
	}
}

void RandWordByBytes(WORD& nr)
{
	BYTE* byte = (BYTE*)&nr;
	for (size_t i = 0; i < 2; i++)
	{
		byte[i] = byte[i] ^ Random(255); //randomice6s4173[randIndex++];
	}
}

void RandReal48ByBytes(double& nr)
{
	BYTE* byte = (BYTE*)&nr;
	for (size_t i = 0; i < 6; i++)
	{
		byte[i] = byte[i] ^ Random(255); //randomice6s4173[randIndex++];
	}
}

void RandBooleanByBytes(bool& nr)
{
	BYTE* byte = (BYTE*)&nr;
	for (size_t i = 0; i < sizeof(nr); i++)
	{
		byte[i] = byte[i] ^ Random(255); //randomice6s4173[randIndex++];
	}
}

void RandArrayByBytes(void* arr, size_t len)
{
	BYTE* byte = (BYTE*)arr;
	for (size_t i = 0; i < len; i++)
	{
		byte[i] = byte[i] ^ Random(255); //randomice6s4173[randIndex++];
	}
}

TFile::TFile(const TFile& orig)
{
	Format = orig.Format;
}

void TFile::Err(WORD n, bool ex)
{
	if (IsWork) {
		SetMsgPar(FandWorkTName);
		WrLLF10Msg(n);
		if (ex) GoExit();
	}
	else {
		CFileMsg(n, 'T');
		if (ex) CloseGoExit();
	}
}

void TFile::TestErr()
{
	if (HandleError != 0) Err(700 + HandleError, true);
}

longint TFile::UsedFileSize()
{
	if (Format == FptFormat) return FreePart * BlockSize;
	else return longint(MaxPage + 1) << MPageShft;
}

bool TFile::NotCached()
{
	return !IsWork && CFile->NotCached();
}

bool TFile::Cached()
{
	return !NotCached();
}

void TFile::RdPrefix(bool Chk)
{
	TT1Page T;
	longint* TNxtAvailPage = (longint*)&T; /* .DBT */
	struct stFptHd { longint FreePart = 0; WORD X = 0, BlockSize = 0; }; /* .FPT */
	stFptHd* FptHd = (stFptHd*)&T;
	BYTE sum = 0;
	longint FS = 0, ML = 0, RS = 0;
	WORD i = 0, n = 0;
	if (Chk) {
		FS = FileSizeH(Handle);
		if (FS <= 512) {
			//FillChar(PwCode, 40, '@');
			PwCode = "";
			PwCode = AddTrailChars(PwCode, '@', 40);
			Code(PwCode);
			SetEmpty();
			return;
		}
	}
	BYTE header512[512];
	RdWrCache(true, Handle, NotCached(), 0, 512, header512);
	srand(RS);
	LicenseNr = 0;
	if (Format == DbtFormat) {
		MaxPage = *TNxtAvailPage - 1;
		GetMLen();
		return;
	}
	if (Format == FptFormat) {
		FreePart = SwapLong((*FptHd).FreePart);
		BlockSize = Swap((*FptHd).BlockSize);
		return;
	}

	// nactena data jsou porad v poli, je nutne je nahrat do T
	T.Load(header512);

	// Move(&T.FreePart, &FreePart, 23);
	FreePart = T.FreePart; // 4B
	Reserved = T.Rsrvd1; // 1B
	CompileProc = T.CompileProc; // 1B
	CompileAll = T.CompileAll; // 1B
	IRec = T.IRec; // 2B
	FreeRoot = T.FreeRoot; // 4B
	MaxPage = T.MaxPage; // 4B
	TimeStmp = T.TimeStmp; // 6B v Pascalu, 8B v C++ 

	if (!IsWork && (CFile == Chpt) && ((T.HasCoproc != HasCoproc) ||
		(CompArea(Version, T.Version, 4) != _equ))) CompileAll = true;
	if (T.OldMaxPage == 0xffff) goto label1;
	else {
		FreeRoot = 0;
		if (FreePart > 0) {
			if (!Chk) FS = FileSizeH(Handle); ML = FS;
			MaxPage = (FS - 1) >> MPageShft; GetMLen();
		}
		else {
			FreePart = -FreePart; MaxPage = T.OldMaxPage;
		label1:
			GetMLen();
			ML = MLen;
			if (!Chk) FS = ML;
		}
	}
	if (IRec >= 0x6000) {
		IRec = IRec - 0x2000;
		if (!IsWork && (CFile->Typ == '0')) LicenseNr = T.LicNr;
	}
	if (IRec >= 0x4000) {
		IRec = IRec - 0x4000;
		RandSeed = ML + T.Time;
		RandIntByBytes(T.FreeRoot);
		RandIntByBytes(T.MaxPage);
		RandReal48ByBytes(T.TimeStmp);
		RandBooleanByBytes(T.HasCoproc);
		RandArrayByBytes(T.Rsrvd2, 25);
		RandArrayByBytes(T.Version, 4);
		RandArrayByBytes(T.LicText, 105);
		RandArrayByBytes(&T.Sum, 1);
		RandArrayByBytes(T.X1, 295);
		RandWordByBytes(T.LicNr);
		RandArrayByBytes(T.X2, 11);
		RandArrayByBytes(T.PwNew, 40);
		PwCode = std::string(&T.PwNew[0], 20);
		Pw2Code = std::string(&T.PwNew[20], 20);
	}
	else {
		RandSeed = ML + T.Time;
		RandIntByBytes(T.FreeRoot);
		RandIntByBytes(T.MaxPage);
		RandReal48ByBytes(T.TimeStmp);
		RandBooleanByBytes(T.HasCoproc);
		RandArrayByBytes(T.Rsrvd2, 25);
		PwCode = std::string(&T.PwNew[0], 20);
		Pw2Code = std::string(&T.PwNew[20], 20);
	}
	Code(PwCode);
	Code(Pw2Code);
	if ((FreePart < MPageSize) || (FreePart > ML) || (FS < ML) ||
		(FreeRoot > MaxPage) || (MaxPage == 0)) {
		Err(893, false);
		MaxPage = (FS - 1) >> MPageShft;
		FreeRoot = 0;
		GetMLen();
		FreePart = NewPage(true);
		SetUpdHandle(Handle);
	}
	//FillChar(&T, 512, 0);
	eofPos = (MaxPage + 1) * MPageSize;
	srand(RS);
}

void TFile::WrPrefix()
{
	TT1Page T;

	switch (Format)	{
	case DbtFormat: {
		longint* TNxtAvailPage = (longint*)&T;		/* .DBT */
		memset(&T, ' ', sizeof(T));
		*TNxtAvailPage = MaxPage + 1;
		break;
	}
	case FptFormat: {
		struct stFptHd { longint FreePart = 0; WORD X = 0, BlockSize = 0; }; /* .FPT */
		stFptHd* FptHd = (stFptHd*)&T;
		memset(&T, 0, sizeof(T));
		(*FptHd).FreePart = SwapLong(FreePart);
		(*FptHd).BlockSize = Swap(BlockSize);
		break;
	}
	case T00Format: {
		longint RS = 0;
		WORD n = 0;
		WORD i = 0;
		memset(&T, '@', sizeof(T));
		std::string Pw = PwCode + Pw2Code;
		Code(Pw);
		RandSeed = RS;
		if (LicenseNr != 0) {
			for (i = 0; i < 20; i++) {
				Pw[i] = static_cast<char>(Random(255));
			}
		}
		n = 0x4000;
		// TODO: T.Time = Time;
		if (Pw.length() != 40) {
			throw std::exception("Bad PwCode + Pw2Code length! Must be 40.");
		}
		memcpy(T.PwNew, Pw.c_str(), Pw.length());
		RandSeed = MLen + T.Time; // srand(MLen + T.Time);
		// for (i = 14; i < 511; i++) TX[i] = TX[i] ^ Random(255);
		RandIntByBytes(T.FreeRoot);
		RandIntByBytes(T.MaxPage);
		RandReal48ByBytes(T.TimeStmp);
		RandBooleanByBytes(T.HasCoproc);
		RandArrayByBytes(T.Rsrvd2, 25);
		RandArrayByBytes(T.Version, 4);
		RandArrayByBytes(T.LicText, 105);
		RandArrayByBytes(&T.Sum, 1);
		RandArrayByBytes(T.X1, 295);
		RandWordByBytes(T.LicNr);
		RandArrayByBytes(T.X2, 11);
		RandArrayByBytes(T.PwNew, 40);

		T.LicNr = LicenseNr;
		if (LicenseNr != 0) {
			BYTE sum = 0;
			n = 0x6000;
			sum = T.LicNr;
			for (i = 0; i < 105; i++) sum += T.LicText[i];
			T.Sum = sum;
		}
		T.FreePart = FreePart; // 8B
		T.Rsrvd1 = Reserved; // 1B
		T.CompileProc = CompileProc; // 1B
		T.CompileAll = CompileAll; // 1B
		T.IRec = IRec; // 2B
		T.FreeRoot = FreeRoot; // 4B
		T.MaxPage = MaxPage; // 4B
		T.TimeStmp = TimeStmp; // 6B Pascal

		T.OldMaxPage = 0xffff;
		T.Signum = 1;
		T.IRec += n;
		memcpy(T.Version, Version, 4);
		T.HasCoproc = HasCoproc;
		RandSeed = RS;
		break;
	}
	default: ;
	}

	BYTE header512[512]{ 0 };
	T.Save(header512);
	RdWrCache(false, Handle, NotCached(), 0, 512, header512);
}

void TFile::SetEmpty()
{
	BYTE X[MPageSize];
	integer* XL = (integer*)&X;
	if (Format == DbtFormat) { MaxPage = 0; WrPrefix(); return; }
	if (Format == FptFormat) { FreePart = 8; BlockSize = 64; WrPrefix(); return; }
	FreeRoot = 0; MaxPage = 1; FreePart = MPageSize; MLen = 2 * MPageSize;
	WrPrefix();
	memset(X, 0, MPageSize); //FillChar(X, MPageSize, 0); 
	*XL = -510;
	RdWrCache(false, Handle, NotCached(), MPageSize, MPageSize, X);
}

void TFile::Create()
{
	Handle = OpenH(_isoverwritefile, Exclusive);
	TestErr();
	IRec = 1; LicenseNr = 0;

	PwCode = "";
	PwCode = AddTrailChars(PwCode, '@', 20);
	Code(PwCode);

	Pw2Code = "";
	Pw2Code = AddTrailChars(Pw2Code, '@', 20);
	Code(Pw2Code);

	eofPos = 2 * MPageSize;

	SetEmpty();
}

longint TFile::NewPage(bool NegL)
{
	longint PosPg;
	BYTE X[MPageSize]{ 0 };
	longint* L = (longint*)&X;
	if (FreeRoot != 0) {
		PosPg = FreeRoot << MPageShft;
		RdWrCache(true, Handle, NotCached(), PosPg, 4, &FreeRoot);
		if (FreeRoot > MaxPage) {
			Err(888, false);
			FreeRoot = 0;
			goto label1;
		}
	}
	else {
	label1:
		MaxPage++; MLen += MPageSize; PosPg = MaxPage << MPageShft;
		//pos = eofPos;			// NE
		eofPos += MPageSize;		// prodlouzim soubor o logickou stranku
		//TruncH(this->Handle, eofPos);			// kvuli FANDu i o fyzickou
	}
	//FillChar(X, MPageSize, 0); 
	if (NegL) *L = -510;
	RdWrCache(false, Handle, NotCached(), PosPg, MPageSize, X);
	return PosPg;
}

void TFile::ReleasePage(longint PosPg)
{
	BYTE X[MPageSize];
	longint* Next = (longint*)&X;
	//FillChar(X, MPageSize, 0);
	memset(X, 0, MPageSize);
	*Next = FreeRoot;
	RdWrCache(false, Handle, NotCached(), PosPg, MPageSize, X);
	FreeRoot = PosPg >> MPageShft;
}

void TFile::Delete(longint pos)
{
	// funkce smaze cast T00 nebo TTT souboru
	// s ohledem na to, ze v TTT souboru jsou predkompilovana data
	// a tato data pak nejsou nicim nahrazena (novou kompilaci neumime ulozit)
	// dojde ke ztrate dat

	// proto budeme kontrolovat, zda se jedna o TTT soubor a pokud ano,
	// tak koncime a nic mazat nebudeme

	std::string name = upperCaseString(CFile->FullName);
	if (name.find("ttt") != std::string::npos) return;

	if (pos <= 0) return;
	if ((Format != T00Format) || NotCached()) return;
	if ((pos < MPageSize) || (pos >= MLen)) {
		Err(889, false);
		return;
	}

	SetUpdHandle(Handle);

	long pospg;
	short l;
	unsigned short u;
	char pg[MPageSize];
	char* p, * p1;

	if (pos < MPageSize || pos >= eofPos)
		return;								// mimo datovou oblast souboru
	//SeekH(pos); Read(&l, 2);					// delka stringu
	RdWrCache(true, Handle, NotCached(), pos, 2, &l);
	if ((unsigned short)l < MPageSize - 2)					// short text?
	{										// ANO, short na sdilene strance
		u = (unsigned short)pos % MPageSize;// offset ve strance
		pospg = pos - u;					// pozice stranky v souboru
		//ReadPg(pospg, pg);					// nactu stranku
		RdWrCache(true, Handle, NotCached(), pospg, MPageSize, pg);
		*(short*)(pg + u) = -l;				// zaporne delku
		p = pg;
		while (!0)							// spojuji volne fragmenty (posledni je vzdy volny!)
		{
			// POZOR, fand v aktualni strance neudrzuje zbyvajici delku!!!, proto nasl. 2 radky
			if (pospg + p - pg == FreePart)
				break;						// pred freePart platny string n. prazdny aktualni
			l = *(short*)p;
			if (l <= 0)						// je-li fragment volny
			{
				p1 = p - l + 2;				// adresa nasledujiciho fragmentu
				if (pospg + p1 - pg == FreePart)			// volna cast aktualniho segmentu
				{
					FreePart = pospg + p - pg;	// pripojim to k volne casti
					break;
				}
				if (p1 >= pg + MPageSize - 2)	// byl posledni (fragment je alespon 3)
					break;
				if ((l = *(short*)p1) <= 0)	// je-li volny
					*(short*)p += l - 2;    // pripojim ho k predchozimu
				else
					p = p1;					// jinak vezmu nasledujici (nebo dalsi? + l + 2)
			}
			else
				p += l + 2;					// preskocim obsazeny fragment
		}
		//		if (freePart >= pospg && freePart < pospg + XPAGESIZE) // actual shared page?
		//		{
		//			WritePg(pospg, pg);			// zapis stranku
		//			pospg += p - pg;			// pozice posledniho volneho segmentu ve strance
		//			if (freePart > pospg)		// tohle je kvuli FANDu
		//				freePart = pospg;		// zvetsim volny prostor pro FAND
												// (lepsi by bylo mit jen adresu sdileneho segmentu)
												// (a vkladat do libovolneho fragmentu ve strance)	
		//		}
		//		else
		if (*(short*)pg <= -(MPageSize - 2)	// jediny volny fragment ve strance
			&& pospg != FreePart)					// a neni to aktualni segment
			ReleasePage(pospg);               // uvolnim stranku
		else
			//WritePg(pospg, pg);				// zapis stranku (celou, mohlo se slucovat)
			RdWrCache(false, Handle, NotCached(), pospg, MPageSize, pg);
	}
	else                          	// NE, long text
		while (!0) 						// cyklus uvolnovani segmentu
		{
			u = (unsigned short)l;
			if (u == MaxLStrLen + 1)	// posledni segment?
				--u;                    // ano, smazu priznak
			u += 2;						// do 1. zapocitam delku
			while (!0)					// cyklus uvolnovani stranek v segmentu
			{
				//SeekH(pos + MPageSize - 4); Read(&pospg, 4);	    // nactu adresu dalsi stranky
				RdWrCache(true, Handle, NotCached(), pos + MPageSize - 4, 4, &pospg);
				ReleasePage(pos);
				pos = pospg;
				if (u <= MPageSize)		// posledni stranka v segmentu
					break;
				u -= MPageSize - 4;
			}
			if ((unsigned short)l != MaxLStrLen + 1)	// dalsi segment?
				break;									// ne
			//SeekH(pos); Read(&l, 2);		// delka segmentu
			RdWrCache(true, Handle, NotCached(), pos, 2, &l);
		}
}

LongStr* TFile::Read(WORD StackNr, longint Pos)
{
	LongStr* s = nullptr;
	WORD i = 0, l = 0;
	char* p = nullptr;
	//WORD* pofs = (WORD*)p;
	int offset = 0;
	struct stFptD { longint Typ = 0, Len = 0; } FptD;
	Pos -= LicenseNr;
	if (Pos <= 0 /*OldTxt=-1 in RDB!*/) goto label11;
	else switch (Format) {
	case DbtFormat: {
		s = new LongStr(32768); //(LongStr*)GetStore(32770);
		Pos = Pos << MPageShft;
		p = s->A;
		l = 0;
		while (l <= 32768 - MPageSize) {
			RdWrCache(true, Handle, NotCached(), Pos, MPageSize, &p[offset]);
			for (i = 1; i < MPageSize; i++) { if (p[offset + i] == 0x1A) goto label0; l++; }
			offset += MPageSize;
			Pos += MPageSize;
		}
		l--;
	label0:
		s->LL = l; ReleaseStore(&s->A[l + 1]);
		break;
	}
	case FptFormat: {
		Pos = Pos * BlockSize;
		RdWrCache(true, Handle, NotCached(), Pos, sizeof(FptD), &FptD);
		if (SwapLong(FptD.Typ) != 1/*text*/) goto label11;
		else {
			l = SwapLong(FptD.Len) & 0x7FFF;
			s = new LongStr(l); //(LongStr*)GetStore(l + 2);

			s->LL = l;
			RdWrCache(true, Handle, NotCached(), Pos + sizeof(FptD), l, s->A);
		}
		break;
	}
	default:
		if ((Pos < MPageSize) || (Pos >= MLen)) goto label1;
		RdWrCache(true, Handle, NotCached(), Pos, 2, &l);
		if (l > MaxLStrLen + 1) {
		label1:
			Err(891, false);
		label11:
			if (StackNr == 1) s = new LongStr(l); //(LongStr*)GetStore(2);
			else s = new LongStr(l); //(LongStr*)GetStore2(2);
			s->LL = 0;
			goto label2;
		}
		if (l == MaxLStrLen + 1) { l--; }
		if (StackNr == 1) s = new LongStr(l); //(LongStr*)GetStore(l + 2);
		else s = new LongStr(l + 2); //(LongStr*)GetStore2(l + 2);
		s->LL = l;
		RdWr(true, Pos + 2, l, s->A);
		break;
	}
label2:
	return s;
}

longint TFile::Store(char* s, size_t l)
{
	integer rest; longint N; void* p; longint pos;
	char X[MPageSize + 1]{ 0 };
	struct stFptD { longint Typ = 0, Len = 0; } FptD;
	longint result = 0;
	if (l == 0) { return result; }

	switch (Format)
	{
	case DbtFormat: {
		pos = MaxPage + 1; N = pos << MPageShft; if (l > 0x7fff) l = 0x7fff;
		RdWrCache(false, Handle, NotCached(), N, l, s);
		FillChar(X, MPageSize, ' '); X[0] = 0x1A; X[1] = 0x1A;
		rest = MPageSize - (l + 2) % MPageSize;
		RdWrCache(false, Handle, NotCached(), N + l, rest + 2, X);
		MaxPage += (l + 2 + rest) / MPageSize;
		break;
	}
	case FptFormat: {
		pos = FreePart; N = FreePart * BlockSize;
		if (l > 0x7fff) l = 0x7fff;
		FreePart = FreePart + (sizeof(FptD) + l - 1) / BlockSize + 1;
		FptD.Typ = SwapLong(1); FptD.Len = SwapLong(l);
		RdWrCache(false, Handle, NotCached(), N, sizeof(FptD), &FptD);
		N += sizeof(FptD);
		RdWrCache(false, Handle, NotCached(), N, l, s);
		N += l;
		l = FreePart * BlockSize - N;
		if (l > 0) {
			p = GetStore(l); FillChar(p, l, ' ');
			RdWrCache(false, Handle, NotCached(), N, l, p); ReleaseStore(p);
		}
		break;

	}
	default: {
		SetUpdHandle(Handle);

		long l1;
		short i;
		char buf[MPageSize];

		if (startPos != 0)			// rozpracovano?
		{       				// ANO
			if (segPos != 0) {		// long string?
				//Seek(segPos);  // ano
				RdWrCache(true, Handle, NotCached(), segPos, 2, &i);
			}
			else {
				//Seek(startPos);     // ne
				RdWrCache(true, Handle, NotCached(), startPos, 2, &i);
			}
			//Read(&i, 2);			// dosud zapsana delka v aktualnim segmentu
			l1 = (unsigned short)i + l; 			// nova delka
			if (segPos == 0 && ((unsigned short)i < MPageSize - 2))		// short data?
			{
				//Read(buf, i);
				RdWrCache(true, Handle, NotCached(), startPos, i, buf);
				Delete(startPos);
				if (l1 < MPageSize - 2)	// zustane short string
				{   	// pripad zapisu kratkych stringu po castech je blbost, neoptimalizuju!
					memmove(buf + i, s, l);		// v bufferu vytvorim novy short string
					StoreShortStr(buf, l + i);
				}
				else						// vytvorim long string
				{
					segPos = startPos = NewPage(false);
					//Seek(startPos + 2);
					//Write(buf, i);				// zapisu puvodni data
					RdWrCache(false, Handle, NotCached(), startPos + 2, i, buf);
					workPos = startPos + i + 2;
					AddLongStr(s, l, i);
				}
			}
			else						// pridavat k long stringu
				AddLongStr(s, l, i);
		}
		else						// NE     -- odpovida soucasnemu reseni
		{
			if (l < MPageSize - 2)		// short string?
			{
				StoreShortStr(s, l);
			}
			else						// long string
			{
				segPos = startPos = NewPage(false);
				workPos = startPos + 2;
				AddLongStr(s, l, 0);
			}
		}
		pos = startPos;
		break;
		}
	}
	return pos;

//	if (Format == DbtFormat) {
//
//		goto label1;
//	}
//	if (Format == FptFormat) {
//
//		goto label1;
//	}
//	if (l > MaxLStrLen) l = MaxLStrLen;
//	if (l > MPageSize - 2) pos = NewPage(false);  /* long text */
//	else {                                  /* short text */
//		rest = MPageSize - FreePart % MPageSize;
//		if (l + 2 <= rest) pos = FreePart;
//		else { pos = NewPage(false); FreePart = pos; rest = MPageSize; }
//		if (l + 4 >= rest) FreePart = NewPage(false);
//		else {
//			FreePart += l + 2; rest = l + 4 - rest;
//			RdWrCache(false, Handle, NotCached(), FreePart, 2, &rest);
//		}
//	}
//	RdWrCache(false, Handle, NotCached(), pos, 2, &l);
//	RdWr(false, pos + 2, l, S->A);
//label1:
	// return pos;
}

void TFile::AddLongStr(char* s, size_t l, unsigned short ls)
{
	long l1, pos;
	unsigned short i, u;
	char* p;

	while (!0)				// segment loop
	{
		l1 = ls + l;            	// nova delka
		if (l1 > MaxLStrLen)        // vysledek delsi nez segment?
		{
			u = MaxLStrLen - ls;	// kolik se vejde do stavajiciho segmentu
			l1 = MaxLStrLen + 1;	// ano
		}
		else
			u = l;
		l -= u;					// kolik zbyva na dalsi segment
		//Seek(segPos);
		//Write(&l1, 2);				// INTEL!
		RdWrCache(false, Handle, NotCached(), segPos, 2, &l1);
		if (i = workPos % MPageSize)		// nikdy nepripravuju prazdnou stranku!
			i = MPageSize - i;              // volne misto ve strance
		if (u > i && i < 4)		// nevejde se a neni misto na adresu, musim ho uvolnit
		{
			workPos += (short)i - 4;	// couvnu na 4 slabiku pred koncem stranky
			// Seek(workPos); Read(&l1, 4);	// uschovam 4 posledni slabiky
			RdWrCache(true, Handle, NotCached(), workPos, 4, &l1);
			pos = NewPage(false);
			// Seek(workPos); Write(&pos, 4);
			RdWrCache(false, Handle, NotCached(), workPos, 4, &pos);
			//Seek(pos); Write(&l1, 4);		// na zacatek nove stranky ulozim uschovana data
			RdWrCache(false, Handle, NotCached(), pos, 4, &l1);
			workPos = pos + 4 - i;
			i = MPageSize + (short)i - 4;        // volne misto v nove strance
		}
		for (p = s; u > i; u -= i, p += i, workPos = pos, i = MPageSize)	// dokud se to nevejde  cele
		{
			i -= 4;			// vynecham misto na chain adresu (musi byt zaruceno)
			pos = NewPage(false);
			//Seek(workPos);
			//Write(p, i);
			//Write(&pos, 4);	// zapisu chain addresu
			RdWrCache(false, Handle, NotCached(), workPos, i, p);
			RdWrCache(false, Handle, NotCached(), workPos + i, 4, &pos);
		}
		//Seek(workPos);		// zapisu zbytek segmentu
		//Write(p, u);
		RdWrCache(false, Handle, NotCached(), workPos, u, p);
		workPos += u;
		if (l == 0)			// neni treba dalsi segment
			break;
		s = p + u;
		segPos = NewPage(false);
		workPos = workPos - workPos % MPageSize + MPageSize - 4;  // muze to byt za strankou???
		//Seek(workPos);
		//Write(&segPos, 4);
		RdWrCache(false, Handle, NotCached(), workPos, 4, &segPos);
		workPos = segPos + 2;
		ls = 0;
	}
}

void TFile::StoreShortStr(char* s, size_t l)
{
	// tohle zobecnit na hledani ve strance?
	short i;
	size_t offset = 0;
	//	Seek(freePart);
	//	Read(&i, 2);			
	//	i = -i; 				// delka volne casti pro data
	i = MPageSize - FreePart % MPageSize - 2;		// delka volne casti - 2 = prostor pro text
	if (l > (unsigned short)i)	// vejde se tam?
	{
		// mohl by prohledavat volne casti
		FreePart = NewPage(false); 	// ne, alokuju novou stranku
		i = MPageSize - 2;		// prostor pro data
	}
	startPos = FreePart;
	//Seek(startPos = FreePart);
	//Write(&l, 2);			// zapisu delku
	//Write(s, l);			// zapisu data
	RdWrCache(false, Handle, NotCached(), startPos, 2, &l);
	RdWrCache(false, Handle, NotCached(), startPos + 2, l, s);
	i -= l;
	// nepripravovat novou stranku (?) - je to predcasne
	// problem - pri uplnem vycerpani ukazuje na nasledujici stranku
	// mohl by mirit o byte nize, stejne se tam zadny fragment nevejde
	// pri ruseni to odchytnu na posledni segment a ne free
	if (i < 2)				// pripravim novou stranku
	{
		FreePart = NewPage(false);
		//Seek(FreePart);
		offset = FreePart;
		i = -MPageSize;
	}
	else
	{
		offset = startPos + 2 + l;
		FreePart += l + 2;
		i = -i;
	}
	i += 2;
	// Write(&i, 2);
	RdWrCache(false, Handle, NotCached(), offset, 2, &i);
}


void TFile::RdWr(bool ReadOp, longint Pos, WORD N, void* X)
{
	Logging* log = Logging::getInstance();
	log->log(loglevel::DEBUG, "TFile::RdWr() 0x%p %s pos: %i, len: %i", Handle, ReadOp ? "read" : "write", Pos, N);
	WORD Rest = 0, L = 0;
	longint NxtPg = 0;
	char* source = (char*)X;
	int offset = 0;
	Rest = MPageSize - (WORD(Pos) & (MPageSize - 1));
	while (N > Rest) {
		L = Rest - 4;
		RdWrCache(ReadOp, Handle, NotCached(), Pos, L, &source[offset]);
		offset += L; N -= L;
		if (!ReadOp) NxtPg = NewPage(false);
		RdWrCache(ReadOp, Handle, NotCached(), Pos + L, 4, &NxtPg);
		Pos = NxtPg;
		if (ReadOp && ((Pos < MPageSize) || (Pos + MPageSize > MLen))) {
			Err(890, false);
			FillChar(&source[offset], N, ' ');
			return;
		}
		Rest = MPageSize;
	}
	RdWrCache(ReadOp, Handle, NotCached(), Pos, N, &source[offset]);
}

void TFile::GetMLen()
{
	MLen = (MaxPage + 1) << MPageShft;
}

WORD RdPrefix()
{
	// NRs - celkovy pocet zaznamu v souboru; RLen - delka 1 zaznamu
	struct x6 { longint NRs = 0; WORD RLen = 0; } X6;
	struct x8 { WORD NRs = 0, RLen = 0; } X8;
	struct xD {
		BYTE Ver = 0; BYTE Date[3] = { 0,0,0 };
		longint NRecs = 0;
		WORD HdLen = 0; WORD RecLen = 0;
	} XD;
	auto result = 0xffff;
	/* !!! with CFile^ do!!! */
	const bool not_cached = CFile->NotCached();
	switch (CFile->Typ) {
	case '8': {
		RdWrCache(true, CFile->Handle, not_cached, 0, 2, &X8.NRs);
		RdWrCache(true, CFile->Handle, not_cached, 2, 2, &X8.RLen);
		CFile->NRecs = X8.NRs;
		if (CFile->RecLen != X8.RLen) { return X8.RLen; }
		break;
	}
	case 'D': {
		RdWrCache(true, CFile->Handle, not_cached, 0, 1, &XD.Ver);
		RdWrCache(true, CFile->Handle, not_cached, 1, 1, &XD.Date[0]);
		RdWrCache(true, CFile->Handle, not_cached, 2, 1, &XD.Date[1]);
		RdWrCache(true, CFile->Handle, not_cached, 3, 1, &XD.Date[2]);
		RdWrCache(true, CFile->Handle, not_cached, 4, 4, &XD.NRecs);
		RdWrCache(true, CFile->Handle, not_cached, 8, 2, &XD.HdLen);
		RdWrCache(true, CFile->Handle, not_cached, 10, 2, &XD.RecLen);
		CFile->NRecs = XD.NRecs;
		if ((CFile->RecLen != XD.RecLen)) { return XD.RecLen; }
		CFile->FrstDispl = XD.HdLen;
		break;
	}
	default: {
		RdWrCache(true, CFile->Handle, not_cached, 0, 4, &X6.NRs);
		RdWrCache(true, CFile->Handle, not_cached, 4, 2, &X6.RLen);
		CFile->NRecs = abs(X6.NRs);
		if ((X6.NRs < 0) && (CFile->Typ != 'X') || (X6.NRs > 0) && (CFile->Typ == 'X')
			|| (CFile->RecLen != X6.RLen)) {
			return X6.RLen;
		}
		break;
	}
	}
	return result;
}

void RdPrefixes()
{
	if (RdPrefix() != 0xffff) CFileError(883);
	if ((CFile->XF != nullptr) && (CFile->XF->Handle != nullptr)) CFile->XF->RdPrefix();
	if ((CFile->TF != nullptr)) CFile->TF->RdPrefix(false);
}

void WrDBaseHd()
{
	FieldDescr* F;
	WORD n, y, m, d, w;
	pstring s;
	const char CtrlZ = '\x1a';

	DBaseHd* P = (DBaseHd*)GetZStore(CFile->FrstDispl);
	char* PA = (char*)&P; // PA:CharArrPtr absolute P;
	F = CFile->FldD.front();
	n = 0;
	while (F != nullptr) {
		if ((F->Flg & f_Stored) != 0) {
			n++;
			{ // with P^.Flds[n]
				auto actual = P->Flds[n];
				switch (F->Typ) {
				case 'F': { actual.Typ = 'N'; actual.Dec = F->M; break; }
				case 'N': { actual.Typ = 'N'; break; }
				case 'A': { actual.Typ = 'C'; break; }
				case 'D': { actual.Typ = 'D'; break; }
				case 'B': { actual.Typ = 'L'; break; }
				case 'T': { actual.Typ = 'M'; break; }
				default:;
				}
				actual.Len = F->NBytes;
				actual.Displ = F->Displ;
				s = F->Name;
				for (size_t i = 1; i < s.length(); i++) s[i] = toupper(s[i]);
				StrLPCopy((char*)&actual.Name[1], s, 11);
			}
		}
		F = (FieldDescr*)F->Chain;
	}

	{ //with P^ do 
		if (CFile->TF != nullptr) {
			if (CFile->TF->Format == TFile::FptFormat) P->Ver = 0xf5;
			else P->Ver = 0x83;
		}
		else P->Ver = 0x03;

		P->RecLen = CFile->RecLen;
		SplitDate(Today(), d, m, y);
		P->Date[1] = BYTE(y - 1900);
		P->Date[2] = (BYTE)m;
		P->Date[3] = (BYTE)d;
		P->NRecs = CFile->NRecs;
		P->HdLen = CFile->FrstDispl;
		PA[(P->HdLen / 32) * 32 + 1] = m;
	}

	// with CFile^
	{
		bool cached = CFile->NotCached();
		RdWrCache(false, CFile->Handle, cached, 0, CFile->FrstDispl, (void*)&P);
		RdWrCache(false, CFile->Handle, cached,
			longint(CFile->NRecs) * CFile->RecLen + CFile->FrstDispl, 1, (void*)&CtrlZ);
	}

	ReleaseStore(P);
}

void WrPrefix()
{
	struct { longint NRs; WORD RLen; } Pfx6 = { 0, 0 };
	struct { WORD NRs; WORD RLen; } Pfx8 = { 0, 0 };

	if (IsUpdHandle(CFile->Handle))
	{
		const bool not_cached = CFile->NotCached();
		switch (CFile->Typ)
		{
		case '8': {
			Pfx8.RLen = CFile->RecLen;
			Pfx8.NRs = static_cast<WORD>(CFile->NRecs);
			RdWrCache(false, CFile->Handle, not_cached, 0, 2, &Pfx8.NRs);
			RdWrCache(false, CFile->Handle, not_cached, 2, 2, &Pfx8.RLen);
			break;
		}
		case 'D': {
			WrDBaseHd();
			break;
		}
		default: {
			Pfx6.RLen = CFile->RecLen;
			if (CFile->Typ == 'X') Pfx6.NRs = -CFile->NRecs;
			else Pfx6.NRs = CFile->NRecs;
			RdWrCache(false, CFile->Handle, not_cached, 0, 4, &Pfx6.NRs);
			RdWrCache(false, CFile->Handle, not_cached, 4, 2, &Pfx6.RLen);
		}
		}
	}
}

void WrPrefixes()
{
	WrPrefix(); /*with CFile^ do begin*/
	if (CFile->TF != nullptr && IsUpdHandle(CFile->TF->Handle))
		CFile->TF->WrPrefix();
	if (CFile->Typ == 'X' && CFile->XF->Handle != nullptr
		&& /*{ call from CopyDuplF }*/ (IsUpdHandle(CFile->XF->Handle) || IsUpdHandle(CFile->Handle)))
		CFile->XF->WrPrefix();
}
