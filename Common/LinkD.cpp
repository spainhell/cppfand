#include "LinkD.h"

LinkD::LinkD()
{
}

LinkD::LinkD(const LinkD& orig)
{
	this->IndexRoot = orig.IndexRoot;
	this->MemberRef = orig.MemberRef;
	this->Args = orig.Args;
	this->FromFile = orig.FromFile;
	this->ToFile = orig.ToFile;
	this->ToKey = orig.ToKey;
	this->RoleName = orig.RoleName;
}
