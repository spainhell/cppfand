#include "TextFile.h"

TextFile::TextFile(std::string fileName, TextFileMode mode)
{
	_mode = mode;
	switch (mode) {
	case read: {
		_inputFile.open(fileName);
		break;
	}
	case write: {
		_outputFile.open(fileName, std::ios::out);
		break;
	}
	case append: {
		_outputFile.open(fileName, std::ios::out | std::ios::app);
		break;
	}
	default:;
	}
	if (_inputFile.fail() || _outputFile.fail())
	{
		//throw std::ios_base::failure(std::strerror(errno));
	}
}

std::string TextFile::GetLine()
{
	std::string line;
	if (_mode == read) {
		std::getline(_inputFile, line);
	}
	return line;
}
