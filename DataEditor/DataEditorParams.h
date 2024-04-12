#pragma once
#include <cstdint>

struct ERecTxtD;

class DataEditorParams
{
public:
	DataEditorParams();
	static void CopyParams(const DataEditorParams* src, DataEditorParams* dst);

	bool AddSwitch = false;
	bool Append = false;
	bool EdRecVar = false;
	bool F1Mode = false;
	bool F3LeadIn = false;
	bool ChkSwitch = false;
	bool LUpRDown = false;
	bool MakeWorkX = false;
	bool Mode24 = false;
	bool MouseEnter = false;
	bool MustAdd = false;
	bool MustESCPrompt = false;
	bool MustCheck = false;
	bool NoCondCheck = false;
	bool NoCreate = false;
	bool NoDelete = false;
	bool NoDelTFlds = false;
	bool NoESCPrompt = false;
	bool NoShiftF7Msg = false;
	bool NoSrchMsg = false;
	bool Only1Record = false;
	bool OnlyAppend = false;
	bool OnlySearch = false;
	bool OnlyTabs = false;
	bool Prompt158 = false;
	bool Select = false;
	bool SelMode = false;
	bool Subset = false;
	bool TTExit = false;
	bool VerifyDelete = false;
	bool WarnSwitch = false;
	bool WasUpdated = false;
	bool WasWK = false;
	bool WithBoolDispl = false;
};
