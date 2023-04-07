#pragma once
#include <fstream>
#include <string>

enum TextFileMode { read, write, append };

class TextFile
{
public:
	TextFile(std::string fileName, TextFileMode mode);
	std::string GetLine();

private:
	TextFileMode _mode;
	std::ifstream _inputFile;
	std::ofstream _outputFile;
};

