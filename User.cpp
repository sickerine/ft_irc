#include "User.hpp"
#include "Channel.hpp"
#include "Server.hpp"

User::User() : registered(false), authenticated(false), server_operator(false), fd(-1)
{
	last_activity = std::time(NULL);
	last_ping = std::time(NULL);
}

User::~User()
{
}

void User::set_nick(const std::string &nick) { nickname = nick; }
const std::string &User::get_nick() { return nickname; }

void User::set_user(const std::string &user) { username = user; }
const std::string &User::get_user() { return username; }

void User::set_host(sockaddr &addr) { hostname = inet_ntoa(((sockaddr_in *)&addr)->sin_addr); }
void User::set_host(const std::string &host) { hostname = host; }
const std::string &User::get_host() { return hostname; }

void User::set_real(const std::string &real) { realname = real; }
const std::string &User::get_real() { return realname; }

void User::set_auth(bool auth) { authenticated = auth; }
bool User::get_auth() { return authenticated; }

void User::set_registered(bool reg) { registered = reg; }
bool User::get_registered() { return registered; }

void User::append_data(const std::string &data) { datastream += data; }
std::string &User::get_data() { return datastream; }

void User::set_fd(int fd) { this->fd = fd; }
int User::get_fd() { return fd; }

bool User::is_server_operator() { return server_operator; }
void User::set_server_operator(bool op) { server_operator = op; }

std::string User::get_prefixed_nick(UserList &operators)
{
	std::string prefix = "";

	if (operators.find(fd) != operators.end())
		prefix += "@";
	return prefix + nickname;
}

std::string User::who_this(Channel &channel)
{
	return username + " " + hostname + " * " + nickname + " H" + (server_operator ? "*" : "") + (channel.is_operator(this) ? "@" : "") + " :0 " + realname;
}

std::string User::get_hostmask(std::string prefixed_nick)
{
	return prefixed_nick + "!" + username + "@" + hostname;
}

void User::set_sendbuffer(const std::string &buffer) { sendbuffer = buffer; }
std::string &User::get_sendbuffer() { return sendbuffer; }
void User::clear_sendbuffer() { sendbuffer.clear(); }
void User::append_sendbuffer(const std::string &buffer) { sendbuffer += buffer; }

void User::set_last_activity() { last_activity = std::time(NULL); }
time_t User::get_last_activity() { return last_activity; }

void User::set_last_ping() { last_ping = std::time(NULL); }
time_t User::get_last_ping() { return last_ping; }
