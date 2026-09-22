#include "HelpViewer.h"
#include "../Drivers/constants.h"

HelpViewer::HelpViewer() : TextEditor(EditorMode::Help, TextType::Memo)
{
	TxtColor = screen.colors.hNorm;
	FillChar(ColKey, 8, screen.colors.tCtrl);
	ColKey[5] = screen.colors.hSpec;
	ColKey[3] = screen.colors.hHili;
	ColKey[1] = screen.colors.hMenu;

	brkKeys.push_back(__F1);
	brkKeys.push_back(__F6);
	brkKeys.push_back(__F10);
	brkKeys.push_back(__CTRL_HOME);
	brkKeys.push_back(__CTRL_END);
}

HelpViewer::~HelpViewer()
{
}

/// Odkazy a pohyb mezi nimi si resi hostitel sam (WpfHost/TextEditWindow.xaml.cs):
/// najde je jako useky mezi dvojicemi 0x13 a zvoleny vrati pres gc->LexWord,
/// odkud si ho bere EditorHelp.cpp. Tady uz zbyva jen predat text.
void HelpViewer::ViewHelp(std::string& help_text, size_t& text_pos)
{
	bool Srch = false;
	bool Upd = false;
	int Scr = 0;

	EditText(EditorMode::Help, TextType::Memo, "", "", help_text, 0xFFF0, text_pos, Scr,
		brkKeys, exitD, Srch, Upd, 142, 145, nullptr);
}
