#pragma once
#include <string>

class FrmlElem;

/// <summary>
/// Logic control class - #L
///	Represents a logic control definition in '#I' of chapter F
/// </summary>
class LogicControl
{
public:
	LogicControl();
	LogicControl(const LogicControl& orig);
	~LogicControl();

	FrmlElem* Bool = nullptr;
	std::string HelpName;
	FrmlElem* TxtZ = nullptr;
	bool Warning = false;
};
