#pragma once

#include "ft_irc.hpp"

class Channel;
class User;
class Server;

class User
{
private:
	std::string nickname;
	std::string username;
	std::string hostname;
	std::string realname;
	std::string datastream;
	bool authenticated;
	bool registered;
	int fd;
	bool server_operator;

public:
	User();

	~User();

	void set_nick(const std::string &nick);
	const std::string &get_nick();

	void set_user(const std::string &user);
	const std::string &get_user();

	void set_host(sockaddr &addr);
	const std::string &get_host();
	void set_host(const std::string &host);

	void set_real(const std::string &real);
	const std::string &get_real();

	void set_auth(bool auth);
	bool get_auth();

	void set_registered(bool reg);
	bool get_registered();

	void append_data(const std::string &data);
	std::string &get_data();

	void set_fd(int fd);
	int get_fd();

	bool is_server_operator();
	void set_server_operator(bool op);

	std::string get_prefixed_nick();

	std::string get_prefixed_nick(UserList &operators);

	std::string who_this(Channel &channel);
	
	std::string get_hostmask(std::string prefixed_nick);
};
