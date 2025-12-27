#include "RprtOpt.h"

RprtOpt::RprtOpt()
{
}

RprtOpt::~RprtOpt()
{
}

void RprtOpt::CopyTo(RprtOpt* dst)
{
	dst->FDL = FDL;
	dst->Path = Path;
	dst->CatIRec = CatIRec;
	dst->UserSelFlds = UserSelFlds;
	dst->UserCondQuest = UserCondQuest;
	dst->FromStr = FromStr;
	dst->SyntxChk = SyntxChk;
	dst->Times = Times;
	dst->Mode = Mode;
	dst->RprtPos = RprtPos;
	dst->Flds = Flds;
	dst->Ctrl = Ctrl;
	dst->Sum = Sum;
	dst->SK = SK;
	dst->WidthFrml = WidthFrml;
	dst->Head = Head;
	dst->Width = Width;
	dst->CondTxt = CondTxt;
	dst->HeadTxt = HeadTxt;
	dst->Style = Style;
	dst->Edit = Edit;
	dst->PrintCtrl = PrintCtrl;
}
