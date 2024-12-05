#include "LinkD.h"

LinkD::LinkD()
{
}

LinkD::LinkD(const LinkD& orig)
{
	this->IndexRoot = orig.IndexRoot;
	this->MemberRef = orig.MemberRef;
	this->Args = orig.Args;
	this->FromFD = orig.FromFD;
	this->ToFD = orig.ToFD;
	this->ToKey = orig.ToKey;
	this->RoleName = orig.RoleName;
}
