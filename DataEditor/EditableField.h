#pragma once
#include <cstdint>
#include <vector>

class FieldDescr;
class LogicControl;
class FrmlElem;
class Dependency;
class XKey;

class EditableField
{
public:
	FieldDescr* FldD = nullptr;
	std::vector<LogicControl*> Checks;
	FrmlElem* Impl = nullptr;
	std::vector<Dependency*> Dependencies;
	std::vector<XKey*> KL;
	std::uint8_t Page = 0, Col = 0, Ln = 0, L = 0;
	uint16_t ScanNr = 0;
	bool Tab = false, Dupl = false, Used = false;
	bool EdU = false, EdN = false;
	bool Ed(bool IsNewRec);
};
