#pragma once

#include "IRCserver.hpp"

class Channel;
class User;
class Server;

enum
{
	MODE_INVITEONLY = 1 << 0,
	MODE_TOPIC = 1 << 1,
	MODE_KEY = 1 << 2,
	MODE_OPERATOR = 1 << 3,
	MODE_LIMIT = 1 << 4
};

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

	const std::string &get_name();
	void set_name(const std::string &n);

	const std::string &get_key();
	void set_key(const std::string &k);

	const std::string &get_topic();
	void set_topic(const std::string &t);

	int get_mode();
	void set_mode(int mode);

	int add_user(int fd, User *user, const std::string &key);
	bool has_user(int fd);
	void remove_user(int fd);

	UserList &get_users();
	std::string get_users_list();
	std::string get_users_count();
	std::vector<std::string> get_who_list();

	int add_operator(User *user);
	bool is_operator(User *user);
	void remove_operator(User *user);

	void invite(User *user);
	bool is_invited(User *user);
	void remove_invite(User *user);
};
