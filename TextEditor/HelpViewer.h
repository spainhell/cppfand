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
	HelpViewer(std::string help_text, size_t text_pos);
	~HelpViewer() override;

	void InitHelpViewEditor(std::string& help_text, size_t& text_pos);

private:
	void Background() override;
	bool WordFind(WORD i, size_t& word_begin, size_t& word_end, size_t& line_nr) override;
	void SetWord(size_t word_begin, size_t word_end) override;
	WORD WordNo2() override;
	size_t WordNo(size_t I) override;
	bool WordExist() override;
	void HelpLU(char dir) override;
	void HelpRD(char dir) override;
	void ClrWord() override;
	void ProcessHelpMode() override;
	void ProcessPageUp() override;
	void ProcessPageDown() override;

	WordPosition _word; // last help word position
};

