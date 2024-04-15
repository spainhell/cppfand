#include "DataEditorParams.h"

DataEditorParams::DataEditorParams()
{
	m_params["^Y"] = &this->NoDelete;
	m_params["?Y"] = &this->VerifyDelete;
	m_params["^N"] = &this->NoCreate;
	m_params["F1"] = &this->F1Mode;
	m_params["F2"] = &this->OnlyAppend;
	m_params["F3"] = &this->OnlySearch;
	m_params["01"] = &this->Only1Record;
	m_params["!!"] = &this->OnlyTabs;
	m_params["??"] = &this->NoESCPrompt;
	m_params["?E"] = &this->MustESCPrompt;
	m_params["?N"] = &this->Prompt158;
	m_params["<="] = &this->NoSrchMsg;
	m_params["R2"] = &this->WithBoolDispl;
	m_params["24"] = &this->Mode24;
	m_params["CO"] = &this->NoCondCheck;
	m_params["LI"] = &this->F3LeadIn;
	m_params["->"] = &this->LUpRDown;
	m_params["^M"] = &this->MouseEnter;
	m_params["EX"] = &this->TTExit;
	m_params["WX"] = &this->MakeWorkX;
	m_params["S7"] = &this->NoShiftF7Msg;
	m_params["#A"] = &this->MustAdd;
	m_params["#L"] = &this->MustCheck;
	m_params["SL"] = &this->SelMode;
}

int DataEditorParams::SetFromString(std::string mode, bool error_stop)
{
	int result = 0;

	// parse mode to set flags
	for (size_t i = 0; i < mode.length(); i += 2) {
		std::string key = mode.substr(i, 2);
		// convert key to lower case
		key[0] = static_cast<char>(toupper(key[0]));
		key[1] = static_cast<char>(toupper(key[1]));
		if (m_params.contains(key)) {
			*m_params[key] = true;
		}
		else {
			if (error_stop) {
				result = 92; // invalid mode
				break;
			}
			else {
				continue;
			}
		}
	}

	return result;
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

