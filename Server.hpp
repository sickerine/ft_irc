#pragma once

#include "IRCserver.hpp"

class Channel;
class User;
class Server;

class Server
{
private:
	std::string host;
	std::string name;
	std::string password;
	std::string operator_username;
	std::string operator_password;
	bool running;

	// TCP stuff
	addrinfo *info;
	int server_fd;
	std::vector<pollfd> pfds;

	// IRC stuff
	UserList users;
	UserList operators;
	std::map<std::string, Channel> channels;
public:
	Server(const std::string &port, const std::string &pass);
	~Server();
	void create_channel(const std::string &name, const std::string &key, const std::string &topic);
	void run();
	void accept_connections();
	void receive_data(int fd);
	void welcome(int fd);
	void parse_command(int fd, const std::string &cmd);
	void parse_data(int fd);
	void process_events(int fd);
	void send_message(int fd, const std::string &message);
	void broadcast_message(Channel & channel, const std::string &message, User *except = NULL);
	void server_broadcast_message(const std::string &message, User *except = NULL);
	static pollfd make_pfd(int fd, int events, int revents);
	void terminate_connection(int fd);
	User *find_user_by_nickname(const std::string &nickname);
	void need_more_params(int fd, const std::string &command);
	void no_such_channel(int fd, const std::string &channel);
	void not_on_channel(int fd, const std::string &channel);
	void no_such_nick(int fd, const std::string &nickname);
	void user_not_in_channel(int fd, const std::string &nickname, const std::string &channel);
	bool is_operator(int fd);
	int add_operator(User *user);
	void remove_operator(User *user);
	void channel_operator_privileges_needed(int fd, const std::string &channel);
};
 