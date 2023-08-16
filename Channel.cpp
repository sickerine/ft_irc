#include "Channel.hpp"
#include "Server.hpp"
#include "User.hpp"

Channel::Channel() {}
Channel::Channel(const std::string &n, const std::string &k, const std::string &t) : name(n), key(k), topic(t), mode(0), limit(0)
{
	if (key != "")
		mode |= MODE_KEY;
}

const std::string &Channel::get_name() { return name; }
void Channel::set_name(const std::string &n) { name = n; }

const std::string &Channel::get_key()
{
	if ((mode & MODE_KEY) == 0)
		key = "";
	return key;
}
void Channel::set_key(const std::string &k) { key = k; }

const std::string &Channel::get_topic() { return topic; }
void Channel::set_topic(const std::string &t) { topic = t; }

int Channel::get_mode() { return mode; }
bool Channel::has_mode(int mode) { return this->mode & mode; }
void Channel::set_mode(int mode) { this->mode = mode; }

size_t Channel::get_limit() { return limit; }
void Channel::set_limit(size_t limit) { this->limit = limit; }

int Channel::add_user(int fd, User *user, const std::string &key)
{
	if (get_key() != "" && get_key() != key)
		return ERR_BADCHANNELKEY;
	users[fd] = user;
	return 0;
}

bool Channel::has_user(int fd)
{
	return users.find(fd) != users.end();
}

void Channel::remove_user(int fd)
{
	users.erase(fd);
}

UserList &Channel::get_users() { return users; }

std::string Channel::get_users_list()
{
	std::string list;

	for (UserList::iterator it = users.begin(); it != users.end(); it++)
		list += it->second->get_prefixed_nick(operators) + " ";
	return list;
}

std::string Channel::get_users_count()
{
	return to_string(users.size());
}

std::vector<std::string> Channel::get_who_list()
{
	std::vector<std::string> list;

	for (UserList::iterator it = users.begin(); it != users.end(); it++)
		list.push_back(it->second->who_this(*this));
	return list;
}

int Channel::add_operator(User *user)
{
	operators[user->get_fd()] = user;
	return 0;
}

bool Channel::is_operator(User *user)
{
	return operators.find(user->get_fd()) != operators.end();
}

void Channel::remove_operator(User *user)
{
	operators.erase(user->get_fd());
}

void Channel::invite(User *user)
{
	invited[user->get_fd()] = user;
}

bool Channel::is_invited(User *user)
{
	return invited.find(user->get_fd()) != invited.end() || (mode & MODE_INVITEONLY) == 0;
}

void Channel::remove_invite(User *user)
{
	invited.erase(user->get_fd());
}
