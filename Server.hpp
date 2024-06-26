#pragma once

#include "IRCserver.hpp"

#define INVALID_COMMAND -1

class Channel;
class User;
class Server;

typedef struct CommandInfo
{
	std::string name;
	void (Server::*func)(int, User *, std::vector<std::string> &);
	bool need_registered;
} CommandInfo;

enum {
	LETTER = 1 << 0,
	SPECIAL = 1 << 1,
	DIGIT = 1 << 2,
	DASH = 1 << 3,
	USERNAME = 1 << 4,
	CHANNEL = 1 << 5,
	KEY = 1 << 6,
	SPACE = 1 << 7,
};

class Server
{
private:

	struct map_string_comparator : std::binary_function<std::string, std::string, bool>
	{
		bool operator()(const std::string &s1, const std::string &s2) const;
	};

	struct
	{
		int port;
		std::string name;
		std::string password;
		std::string operator_username;
		std::string operator_password;
		std::vector<std::string> motd;
		time_t activity_timeout;
		time_t ping_timeout;
		size_t max_message_length;
		size_t max_nickname_length;
		size_t max_server_name_length;
		size_t max_channel_name_length;
		int channel_creation;

		struct
		{
			std::string nickname;
			std::string username;
			std::string realname;
			int fd;
		} bot;
	} conf;
	bool running;
	bool repoll;

	// TCP stuff
	addrinfo *info;
	int server_fd;
	std::vector<pollfd> pfds;

	// IRC stuff
	static CommandInfo commands[];
	UserList users;
	UserList operators;
	std::map<std::string, Channel, map_string_comparator> channels;
	std::map<std::string, std::string> configs;

public:
	Server(const std::string &port, const std::string &pass);
	~Server();
	void run();

	// Networking
	void accept_connections();
	void receive_data(int fd);
	void process_events(int fd, int revents);
	static pollfd make_pfd(int fd, int events, int revents);
	void terminate_connection(int fd);

	// Parsing
	void parse_command(int fd, const std::string &cmd);
	void parse_data(int fd);

	// Validation
	bool verify_string(const std::string &str, int modes);
	bool verify_nickname(const std::string &nickname);
	bool verify_server_name();
	int is_valid_command(const std::string &line);

	// Broadcast
	void send_message(int fd, const std::string &message);
	void broadcast_message(Channel &channel, const std::string &message, User *except = NULL);
	void server_broadcast_message(const std::string &message, User *except = NULL);
	void broadcast_user_channels(int fd, const std::string &message, User *except = NULL);

	// Helpers
	void create_channel(const std::string &name, const std::string &key, const std::string &topic);
	User *find_user_by_nickname(const std::string &nickname);
	void welcome(int fd);

	// Operators
	bool is_operator(int fd);
	int add_operator(User *user);
	void remove_operator(User *user);

	// Errors
	void need_more_params(int fd, const std::string &command);
	void no_such_channel(int fd, const std::string &channel);
	void not_on_channel(int fd, const std::string &channel);
	void no_such_nick(int fd, const std::string &nickname);
	void not_registered(int fd);
	void user_not_in_channel(int fd, const std::string &nickname, const std::string &channel);
	void channel_operator_privileges_needed(int fd, const std::string &channel);
	void already_registered(int fd);
	void unknown_command(int fd, const std::string &command);

	// Conf
	bool load_config(const std::string &filename);
	bool load_config_channel(std::ifstream &file);

	// Bot
	void initialize_bot();
	bool bot_parse();
	void bot_response(std::string message);

	// Command Handlers
	void PASS(int fd, User *user, std::vector<std::string> &args);
	void USER(int fd, User *user, std::vector<std::string> &args);
	void NICK(int fd, User *user, std::vector<std::string> &args);
	void LIST(int fd, User *user, std::vector<std::string> &args);
	void QUIT(int fd, User *user, std::vector<std::string> &args);
	void JOIN(int fd, User *user, std::vector<std::string> &args);
	void WHO(int fd, User *user, std::vector<std::string> &args);
	void PRIVMSG(int fd, User *user, std::vector<std::string> &args);
	void ISON(int fd, User *user, std::vector<std::string> &args);
	void PART(int fd, User *user, std::vector<std::string> &args);
	void PING(int fd, User *user, std::vector<std::string> &args);
	void OPER(int fd, User *user, std::vector<std::string> &args);
	void KICK(int fd, User *user, std::vector<std::string> &args);
	void INVITE(int fd, User *user, std::vector<std::string> &args);
	void TOPIC(int fd, User *user, std::vector<std::string> &args);
	void MODE(int fd, User *user, std::vector<std::string> &args);
	void IGNORED(int fd, User *user, std::vector<std::string> &args);
};
