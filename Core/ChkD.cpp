#include "ChkD.h"
#include "models/FrmlElem.h"

ChkD::ChkD(const ChkD& orig)
{
	if (orig.Bool != nullptr) Bool = CopyFrmlElem(orig.Bool);
	HelpName = orig.HelpName;
	if (orig.TxtZ != nullptr) TxtZ = CopyFrmlElem(orig.TxtZ);
	Warning = orig.Warning;
}
