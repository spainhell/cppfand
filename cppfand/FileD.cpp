#include "FileD.h"
#include "access.h"
#include "AddD.h"
#include "GlobalVariables.h"
#include "KeyFldD.h"
#include "XFile.h"
#include "../Indexes/XKey.h"
#include "../Logging/Logging.h"

/// <summary>
/// Vycte zaznam z datoveho souboru (.000)
/// </summary>
/// <param name="file">ukazatel na soubor</param>
/// <param name="N">kolikaty zaznam (1 .. N)</param>
/// <param name="record">ukazatel na buffer</param>
void ReadRec(FileD* file, longint N, void* record)
{
	Logging* log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "ReadRec(), file 0x%p, RecNr %i", file, N);
	RdWrCache(true, file->Handle, file->NotCached(),
		(N - 1) * file->RecLen + file->FrstDispl, file->RecLen, record);
}

void WriteRec(FileD* file, longint N, void* record)
{
	Logging* log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "WriteRec(%i), CFile 0x%p", N, file->Handle);
	RdWrCache(false, file->Handle, file->NotCached(),
		(N - 1) * file->RecLen + file->FrstDispl, file->RecLen, record);
	file->WasWrRec = true;
}

FileD::FileD()
{
}

FileD::FileD(const FileD& orig)
{
	Name = orig.Name;
	RecLen = orig.RecLen;
	Typ = orig.Typ;
	if (orig.TF != nullptr) TF = new TFile(*orig.TF);
	ChptPos = orig.ChptPos;
	//*OrigFD = orig;
	Drive = orig.Drive;
	FldD = orig.FldD;
	IsParFile = orig.IsParFile;
	IsJournal = orig.IsJournal;
	IsHlpFile = orig.IsHlpFile;
	typSQLFile = orig.typSQLFile;
	IsSQLFile = orig.IsSQLFile;
	IsDynFile = orig.IsDynFile;
	if (orig.XF != nullptr) XF = new XFile(*orig.XF);
	if (!orig.Keys.empty()) {
		for (auto& k : orig.Keys) {
			XKey* newKey = new XKey(*k);
			Keys.push_back(newKey);
		}
	}
	Add = orig.Add;
	nLDs = orig.nLDs;
	LiOfs = orig.LiOfs;
}

longint FileD::UsedFileSize()
{
	longint n;
	n = longint(NRecs) * RecLen + FrstDispl;
	if (Typ == 'D') n++;
	return n;
}

bool FileD::IsShared()
{
	return (UMode == Shared) || (UMode == RdShared);
}

bool FileD::NotCached()
{
	if (UMode == Shared) goto label1;
	if (UMode != RdShared) return false;
label1:
	if (LMode == ExclMode) return false;
	return true;
}

bool FileD::Cached()
{
	return !NotCached();
}

WORD FileD::GetNrKeys()
{
	/*XKey* k = Keys;
	WORD n = 0;
	while (k != nullptr) { n++; k = k->Chain; }
	return n;*/
	return static_cast<WORD>(Keys.size());
}

void FileD::Reset()
{
	Name = "";
	FullName = "";
	RecLen = 0;
	RecPtr = nullptr;
	NRecs = 0;
	WasWrRec = false; WasRdOnly = false; Eof = false;
	Typ = rdb;        // 8= Fand 8; 6= Fand 16; X= .X; 0= RDB; C= CAT 
	Handle = nullptr;
	IRec = 0;
	FrstDispl = 0;
	TF = nullptr;
	ChptPos.IRec = 0;
	ChptPos.R = 0;
	TxtPosUDLI = 0;    // =0 if not present; urcuje zacatek odstavcu #U #D #L #I
	OrigFD = nullptr;    // like orig. or nil
	Drive = 0;         // 1=A, 2=B, else 0
	CatIRec = 0;
	FldD.clear();
	IsParFile = false; IsJournal = false; IsHlpFile = false;
	typSQLFile = false; IsSQLFile = false; IsDynFile = false;
	UMode = FileUseMode::Closed;
	LMode = NullMode; ExLMode = NullMode; TaLMode = NullMode;
	ViewNames = nullptr;  //after each string BYTE string with user codes 
	XF = nullptr;
	Keys.clear();
	Add.clear();
	nLDs = 0; LiOfs = 0;
}
