#pragma once
#include <cstdint>

struct ERecTxtD;

class DataEditorParams
{
public:
	bool Append = false;
	bool Select = false;
	bool WasUpdated = false;
	bool EdRecVar = false;
	bool AddSwitch = false;
	bool ChkSwitch = false;
	bool WarnSwitch = false;
	bool Subset = false;
	bool NoDelTFlds = false;
	bool WasWK = false;
	bool NoDelete = false;
	bool VerifyDelete = false;
	bool NoCreate = false;
	bool F1Mode = false;
	bool OnlyAppend = false;
	bool OnlySearch = false;
	bool Only1Record = false;
	bool OnlyTabs = false;
	bool NoESCPrompt = false;
	bool MustESCPrompt = false;
	bool Prompt158 = false;
	bool NoSrchMsg = false;
	bool WithBoolDispl = false;
	bool Mode24 = false;
	bool NoCondCheck = false;
	bool F3LeadIn = false;
	bool LUpRDown = false;
	bool MouseEnter = false;
	bool TTExit = false;
	bool MakeWorkX = false;
	bool NoShiftF7Msg = false;
	bool MustAdd = false;
	bool MustCheck = false;
	bool SelMode = false;
	bool HasIndex = false;
	bool HasTF = false;
	bool NewDisplLL = false;
};
