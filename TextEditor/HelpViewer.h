#pragma once
#include "TextEditor.h"

struct WordPosition
{
	size_t start_line = 0;
	size_t start_index = 0;
	size_t end_line = 0;
	size_t end_index = 0;
};

class HelpViewer : public TextEditor
{
public:
	HelpViewer();
	~HelpViewer() override;

	void ViewHelp(std::string& help_text, size_t& text_pos);

private:
	void Background() override;
	void FindAllWords();
	bool WordFind(WORD i);
	void SetWord();
	WORD WordNo2() override;
	size_t WordNo(size_t I) override;
	bool WordExistsOnActualScreen() override;
	void ProcessHelpMove(uint16_t pressed_key) override;
	void HelpLU(char dir);
	void HelpRD(char dir);
	void ClrWord() override;
	void ProcessHelpMode() override;
	void ProcessPageUp();
	void ProcessPageDown();

	std::vector<WordPosition> _word_list; // list of help words
	size_t _word_index = 0;
	std::vector<WORD> brkKeys;
	std::vector<EdExitD*> exitD;
};

