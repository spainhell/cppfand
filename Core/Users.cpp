#include "Users.h"

#include <iomanip>
#include <sstream>


Users::Users()
{
}

Users::~Users()
{
}

void Users::set(const std::string& user_name, uint32_t user_code, const std::string& user_password, const std::set<uint16_t>& acc_rights)
{
	UserName = user_name;
	UserCode = user_code;
	UserPassWORD = user_password;
	AccRight = acc_rights;
}

void Users::set_acc_0(const std::string& user_name, uint32_t user_code, const std::string& user_password)
{
	UserName = user_name;
	UserCode = user_code;
	UserPassWORD = user_password;
	AccRight.clear();
}

void Users::clear()
{
	UserName.clear();
	UserCode = 0;
	UserPassWORD.clear();
	AccRight.clear();
}

std::string Users::get_user_name() const
{
	return UserName;
}

uint16_t Users::get_user_code() const
{
	return UserCode;
}

std::string Users::get_acc_rights() const
{
	std::string result;
	for (const uint16_t& acc_right : AccRight) {
		result += static_cast<int8_t>(acc_right);
	}
	return result;
}

void Users::set_user_name(const std::string& user_name)
{
	UserName = user_name;
}

void Users::set_user_code(uint32_t user_code)
{
	UserCode = user_code;
}

void Users::set_acc_rights(const std::string& acc_rights)
{
	AccRight.clear();
	for (const char& acc_right : acc_rights) {
		AccRight.insert(acc_right);
	}
}

void Users::set_acc_right(uint16_t acc_right)
{
	AccRight.clear();
	AccRight.insert(acc_right);
}
