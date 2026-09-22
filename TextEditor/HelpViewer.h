#pragma once
#include "TextEditor.h"

/// Nahled napovedy. Vykreslovani, odkazy i pohyb mezi nimi prevzal hostitel,
/// takze tady zbyvaji jen barvy podle FAND.CFG a ukoncovaci klavesy, podle
/// kterych se pak vetvi EditorHelp.cpp.
class HelpViewer : public TextEditor
{
public:
	HelpViewer();
	~HelpViewer() override;

	void ViewHelp(std::string& help_text, size_t& text_pos);

private:
	std::vector<WORD> brkKeys;
	std::vector<EdExitD*> exitD;
};
