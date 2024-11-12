#include "LogicControl.h"
#include "models/FrmlElem.h"

LogicControl::LogicControl()
= default;

LogicControl::LogicControl(const LogicControl& orig)
{
	if (orig.Bool != nullptr) {
		Bool = CopyFrmlElem(orig.Bool);
	}
	HelpName = orig.HelpName;
	if (orig.TxtZ != nullptr) {
		TxtZ = CopyFrmlElem(orig.TxtZ);
	}
	Warning = orig.Warning;
}

LogicControl::~LogicControl()
= default;
