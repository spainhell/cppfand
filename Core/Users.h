#pragma once
#include <set>
#include <string>

class Users
{
public:
	Users();
	~Users();
	void set(const std::string& user_name, uint32_t user_code, const std::string& user_password, const std::set<uint16_t>& acc_rights);
	void set_acc_0(const std::string& user_name, uint32_t user_code, const std::string& user_password);
	void clear();

	std::string get_user_name() const;
	uint16_t get_user_code() const;
	std::string get_acc_rights() const;

	void set_user_name(const std::string& user_name);
	void set_user_code(uint32_t user_code);
	void set_acc_rights(const std::string& acc_rights);
	void set_acc_right(uint16_t acc_right);

private:
	std::string UserName;
	uint32_t UserCode;
	std::string UserPassWORD;
	std::set<uint16_t> AccRight;
};

