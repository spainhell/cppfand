#include "DataEditorParams.h"

DataEditorParams::DataEditorParams()
{
}

void DataEditorParams::CopyParams(const DataEditorParams* src, DataEditorParams* dst)
{
	dst->AddSwitch = src->AddSwitch;
	dst->Append = src->Append;
	dst->EdRecVar = src->EdRecVar;
	dst->F1Mode = src->F1Mode;
	dst->F3LeadIn = src->F3LeadIn;
	dst->ChkSwitch = src->ChkSwitch;
	dst->LUpRDown = src->LUpRDown;
	dst->MakeWorkX = src->MakeWorkX;
	dst->Mode24 = src->Mode24;
	dst->MouseEnter = src->MouseEnter;
	dst->MustAdd = src->MustAdd;
	dst->MustESCPrompt = src->MustESCPrompt;
	dst->MustCheck = src->MustCheck;
	dst->NoCondCheck = src->NoCondCheck;
	dst->NoCreate = src->NoCreate;
	dst->NoDelete = src->NoDelete;
	dst->NoDelTFlds = src->NoDelTFlds;
	dst->NoESCPrompt = src->NoESCPrompt;
	dst->NoShiftF7Msg = src->NoShiftF7Msg;
	dst->NoSrchMsg = src->NoSrchMsg;
	dst->Only1Record = src->Only1Record;
	dst->OnlyAppend = src->OnlyAppend;
	dst->OnlySearch = src->OnlySearch;
	dst->OnlyTabs = src->OnlyTabs;
	dst->Prompt158 = src->Prompt158;
	dst->Select = src->Select;
	dst->SelMode = src->SelMode;
	dst->Subset = src->Subset;
	dst->TTExit = src->TTExit;
	dst->VerifyDelete = src->VerifyDelete;
	dst->WarnSwitch = src->WarnSwitch;
	dst->WasUpdated = src->WasUpdated;
	dst->WasWK = src->WasWK;
	dst->WithBoolDispl = src->WithBoolDispl;
}

