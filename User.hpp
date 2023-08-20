#pragma once

#include "IRCserver.hpp"

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
	std::string sendbuffer;
	bool registered;
	bool authenticated;
	bool server_operator;
	time_t last_activity;
	time_t last_ping;
	int fd;

public:
	User();
	~User();

	void set_nick(const std::string &nick);
	const std::string &get_nick();

	void set_user(const std::string &user);
	const std::string &get_user();

	void set_host(sockaddr &addr);
	void set_host(const std::string &host);
	const std::string &get_host();

	void set_real(const std::string &real);
	const std::string &get_real();

	void set_sendbuffer(const std::string &buffer);
	std::string &get_sendbuffer();
	void clear_sendbuffer();
	void append_sendbuffer(const std::string &buffer);

	void set_last_activity();
	time_t get_last_activity();

	void set_last_ping();
	time_t get_last_ping();

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

	std::string get_prefixed_nick(UserList &operators);
	std::string get_hostmask(std::string prefixed_nick);

	std::string who_this(Channel &channel);
};
