#pragma once

#include "ft_irc.hpp"

class Channel;
class User;
class Server;

class Channel
{
private:
	std::string name;
	std::string key;
	std::string topic;
	int mode;
	UserList users;
	UserList operators;
	UserList invited;

public:
	Channel();
	Channel(const std::string &n, const std::string &k, const std::string &t);
	int add_user(int fd, User *user, const std::string &key);
	void remove_user(int fd);
	const std::string &get_name();
	std::string get_users_count();

	const std::string &get_topic();
	void set_topic(const std::string &t);

	void set_mode(int mode);
	int get_mode();

	UserList &get_users();

	std::string get_users_list();

	std::vector<std::string> get_who_list();

	bool has_user(User *user);

	bool has_user(int fd);

	bool is_operator(User *user);

	bool is_operator(int fd);

	int add_operator(User *user);

	int remove_operator(User *user);

	bool is_invited(User *user);

	void invite(User *user);

	void remove_invite(User *user);
};
