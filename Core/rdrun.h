#pragma once

#include "access.h"
#include "../fandio/FieldDescr.h"
#include "legacy.h"
#include "../DataEditor/EditD.h"
#include "models/FrmlElem.h"
#include "../fandio/XScan.h"
#include "../MergeReport/BlkD.h"


struct OutpRD;
struct OutpFD;
class Instr;
class Instr_proc;
struct EdExitD;
class FieldDescr;

enum class MInstrCode { _zero, _move, _output, _locvar, _parfile, _ifThenElse };

struct AssignD
{
	MInstrCode Kind = MInstrCode::_zero;
	Record* inputRecord = nullptr;
	FieldDescr* inputField = nullptr;
	Record* outputRecord = nullptr;
	FieldDescr* outputField = nullptr;
	bool Add = false; FrmlElem* Frml = nullptr; FieldDescr* OFldD = nullptr;
	bool Add1 = false; FrmlElem* Frml1 = nullptr; LocVar* LV = nullptr;
	bool Add2 = false; FrmlElem* Frml2 = nullptr;
	FileD* FD = nullptr; FieldDescr* PFldD = nullptr;
	FrmlElem* Bool = nullptr;
	std::vector<AssignD*> Instr;
	std::vector<AssignD*> ElseInstr;
};


/* Report */

struct LvDescr {
	LvDescr* Chain = nullptr;
	LvDescr* ChainBack = nullptr;
	std::vector<FrmlElemSum*> ZeroLst;
	std::vector<BlkD*> Hd;
	std::vector<BlkD*> Ft;
	FieldDescr* Fld = nullptr;
};

struct EdExKeyD
{
	std::string KeyName;
	uint8_t Break = 0;
	uint16_t KeyCode = 0;
};

struct EdExitD
{
	std::vector<EdExKeyD> Keys;
	bool AtWrRec = false, AtNewRec = false, NegFlds = false;
	std::vector<FieldDescr*> Flds;    /*in edittxt !used*/
	char Typ = 0;
	void* RO = nullptr;
	Instr_proc* Proc = nullptr;       /*in edittxt only "P","Q"*/
	/*"Q" quit   #0 dummy*/
};

struct ERecTxtD
{
	WORD N;
	std::vector<std::string> SL;
};

enum class CpOption {cpNo, cpFix, cpVar, cpTxt};

struct CopyD
{
	// pstring* Path1; /*FrmlPtr if cpList*/
	std::string Path1;
	WORD CatIRec1;
	FileD* FD1;
	XKey* ViewKey;
	bool WithX1;
	CpOption Opt1;
	// pstring* Path2; /*  "  */
	std::string Path2;
	WORD CatIRec2;
	FileD* FD2;
	bool WithX2;
	CpOption Opt2;
	FileD* HdFD;
	FieldDescr* HdF;
	bool Append, NoCancel;
	uint8_t Mode;
};

struct ChoiceD
{
	std::string HelpName;
	bool Displ = false, DisplEver = false, Enabled = false, TxtConst = false;
	FrmlElem* Condition = nullptr;
	std::vector<Instr*> v_instr;
	FrmlElem* TxtFrml = nullptr;
	std::string Txt;
};

struct WrLnD
{
	FrmlElem* Frml = nullptr;
	char Typ = '\0'; /* S, B, F, D */
	uint8_t N = 0, M = 0;
	std::string Mask;
};

struct LockD
{
	FileD* FD = nullptr;
	FrmlElem* Frml = nullptr;
	LockMode Md, OldMd;
	int32_t N = 0;
};

struct TypAndFrml
{
	char FTyp = '\0';
	FrmlElem* Frml = nullptr; 
	bool FromProlog = false, IsRetPar = false;
	FileD* FD = nullptr; 
	Record* record = nullptr;
	FrmlElem* TxtFrml = nullptr; 
	std::string Name; // if RecPtr != nullptr
};


extern XString OldMXStr;	/* Merge */

extern bool Join;
extern bool PrintView;		/* Report */
extern TextFile Rprt;		// puvodne text - souvisi s text. souborem

extern std::vector<FrmlElemSum*> PFZeroLst;
//extern LvDescr* FrstLvM;
//extern LvDescr* LstLvM; /* LstLvM->Ft=RF */
extern bool SelQuest;
	/* Edit */
extern FrmlElem* PgeSizeZ, *PgeLimitZ;
extern EditD* EditDRoot;
extern bool CompileFD, EditRdbMode;
extern LocVarBlock LVBD;

extern std::string CalcTxt;
struct MergOpSt { char Op; double Group; };
extern MergOpSt MergOpGroup;

void ResetLVBD();
void CrIndRec(FileD* file_d, Record* record);
Record* Link(FileD* file_d, Additive* add_d, int& n, char& kind2, Record* record);
bool TransAdd(FileD* file_d, Additive* AD, FileD* FD, Record* RP, Record* new_record, int N, char Kind2, bool Back);
void WrUpdRec(FileD* file_d, Additive* add_d, FileD* fd, Record* rp, Record* new_record, int n);
bool Assign(FileD* file_d, Additive* add_d, Record* record);
bool LockForAdd(FileD* file_d, WORD kind, bool Ta, LockMode& md);

bool RunAddUpdate(FileD* file_d, char kind, Record* old_record, LinkD* not_link_d, Record* record);
bool RunAddUpdate(FileD* file_d, char kind, Record* old_record, bool back, Additive* stop_add_d, LinkD* not_link_d, Record* record);

bool TestExitKey(WORD KeyCode, EdExitD* X);
void SetCompileAll();
