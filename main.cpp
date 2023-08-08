#include "ft_irc.hpp"

template <typename T>
T initialized()
{
	T tmp;
	std::memset(&tmp, 0, sizeof(T));
	return tmp;
}

template <typename T, typename U>
void insist(T ret, U err, const std::string &msg)
{
	if (ret == err)
		throw std::runtime_error(msg);
}

std::string escape(const std::string &str)
{
	std::string escaped;
	for (size_t i = 0; i < str.length(); i++)
	{
		if (str[i] == '\n')
			escaped += "\\n";
		else if (str[i] == '\r')
			escaped += "\\r";
		else
			escaped += str[i];
	}
	return escaped;
}

std::vector<std::string> split(const std::string &str, char delim, bool trim = false)
{
	std::vector<std::string> splits;
	std::istringstream iss(str);
	std::string line;

	while (std::getline(iss, line, delim))
	{
		if (line.empty())
			splits.back() += delim;
		else
			splits.push_back(line);
	}
	if (trim)
		splits.back().erase(splits.back().length() - 1);

	for (size_t i = 0; i < splits.size(); i++)
	{
		std::cout << "[" << i << "] " << splits[i] << std::endl;
	}
	return splits;
}

std::string c(int code)
{
	std::stringstream ss;
	ss << std::setw(3) << std::setfill('0') << code;
	return ss.str();
}

template <typename T>
std::string join(T begin, T end, std::string delim)
{
	std::string joined;
	for (T it = begin; it != end; it++)
	{
		joined += *it;
		if (it + 1 != end)
			joined += delim;
	}
	return joined;
}

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
	int status;

public:
	User() : authenticated(false), registered(false), status(REGULAR)
	{
		std::cout << "new user constructed" << std::endl;
	}

	~User()
	{
	}

	void set_nick(const std::string &nick) { nickname = nick; }
	const std::string &get_nick() { return nickname; }

	void set_user(const std::string &user) { username = user; }
	const std::string &get_user() { return username; }

	void set_host(sockaddr &addr) { hostname = inet_ntoa(((sockaddr_in *)&addr)->sin_addr); }
	const std::string &get_host() { return hostname; }
	void set_host(const std::string &host) { hostname = host; }

	void set_real(const std::string &real) { realname = real; }
	const std::string &get_real() { return realname; }

	void set_auth(bool auth) { authenticated = auth; }
	bool get_auth() { return authenticated; }

	void set_registered(bool reg) { registered = reg; }
	bool get_registered() { return registered; }

	void append_data(const std::string &data) { datastream += data; }
	std::string &get_data() { return datastream; }

	std::string get_prefixed_nick()
	{
		return std::string(status == OPERATOR ? "@" : "") + nickname;
	}

	std::string who_this()
	{
		return nickname + " " + username + " " + hostname + " * " + realname;
	}
	
	std::string get_hostmask()
	{
		return nickname + "!" + username + "@" + hostname;
	}
};

class Channel
{
private:
	std::string name;
	std::string key;
	std::string topic;
	std::map<int, User *> users;

public:
	Channel() {}
	Channel(const std::string &n, const std::string &k, const std::string &t) : name(n), key(k), topic(t)
	{

	}

	int add_user(pollfd &pfd, User &user, const std::string &key)
	{
		if (this->key != key)
			return ERR_BADCHANNELKEY;
		users[pfd.fd] = &user;
		return 0;
	}

	void remove_user(pollfd &pfd)
	{
		users.erase(pfd.fd);
	}
	const std::string &get_name() { return name; }
	std::string get_users_count()
	{
		std::stringstream ss;
		ss << users.size();
		return ss.str();
	}
	
	const std::string &get_topic() { return topic; }
	void set_topic(const std::string &t) { topic = t; }

	std::string get_users_list()
	{
		std::string list;
		for (std::map<int, User *>::iterator it = users.begin(); it != users.end(); it++)
		{
			list += it->second->get_prefixed_nick() + " ";
		}
		return list;
	}

	std::vector<std::string> get_who_list()
	{
		std::vector<std::string> list;
		for (std::map<int, User *>::iterator it = users.begin(); it != users.end(); it++)
		{
			list.push_back(it->second->who_this());
		}
		return list;
	}

};

class Server
{
private:
	std::string password;
	std::string name;
	addrinfo *info;
	int server_fd;
	std::vector<pollfd> fds;
	std::map<int, User> users;
	bool running;
	std::string host;
	std::map<std::string, Channel> channels;

public:
	Server(const std::string &port, const std::string &pass) : password(pass), name("globalhost"), info(NULL), running(true), host("10.12.4.7")
	{
		int on = 1;
		addrinfo hints = initialized<addrinfo>();

		hints.ai_family = AF_INET;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_socktype = SOCK_STREAM;

		insist(getaddrinfo(host.c_str(), port.c_str(), &hints, &info), -1, "getaddrinfo failed");
		insist(info == NULL, true, "info is null");

		server_fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

		insist(server_fd, -1, "socket failed");
		insist(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(int)), -1, "setsockopt failed");
		insist(fcntl(server_fd, F_SETFL, O_NONBLOCK), -1, "fcntl failed");
		insist(bind(server_fd, info->ai_addr, info->ai_addrlen) != 0, true, "bind failed");
		insist(listen(server_fd, 69), -1, "listen failed");

		fds.push_back(make_pfd(server_fd, POLLIN, 0));

		create_channel("#global", "123", "safe");
		create_channel("#hentai", "123", "nsfw");
	}
	~Server()
	{
		if (info)
			freeaddrinfo(info);
	}

	void create_channel(const std::string &name, const std::string &key, const std::string &topic)
	{
		channels[name] = Channel(name, key, topic);
	}

	void run()
	{
		while (running)
		{
			insist(poll(&fds[0], fds.size(), -1), -1, "poll failed");
			for (size_t i = 0; i < fds.size(); i++)
				process_events(fds[i]);
		}
	}

	void accept_connections()
	{
		int new_fd = 0;

		while (true)
		{
			sockaddr addr;
			socklen_t len = sizeof(addr);

			new_fd = accept(server_fd, &addr, &len);
			if (new_fd == -1)
				break;

			fds.push_back(make_pfd(new_fd, POLLIN, 0));
			users[new_fd] = User();
			users[new_fd].set_host(addr);
			std::cout << "Connection accepted on fd " << new_fd << std::endl;
		}
	}

	void receive_data(pollfd &pfd)
	{
		char buffer[1024];
		int length;
		while (true)
		{
			length = recv(pfd.fd, buffer, sizeof(buffer) - 1, 0);
			if (length == -1)
				break;

			buffer[length] = 0;
			users[pfd.fd].append_data(std::string(buffer));
		}
	}

	void welcome(pollfd &pfd)
	{
		send_message(pfd, ":" + name + " " + c(RPL_WELCOME) + " " + users[pfd.fd].get_nick() + " :kys " + users[pfd.fd].get_nick() + "!" + users[pfd.fd].get_user() + "@" + name);
		send_message(pfd, ":" + name + " " + c(RPL_STARTOFMOTD) + " " + users[pfd.fd].get_user() + " :- " + name + " Message of the Day -");
		send_message(pfd, ":" + name + " " + c(RPL_MOTD) + " " + users[pfd.fd].get_nick() + " :- LOLOLOLOLOLOLOLOLOLOOLOLOLOLOLOLOLOLOLOLOLOLOLOLOLOLOLOOLOLOLOLOLOLOLOLOLOLOLOLOLOLOLOLOLOOLOLOLOLOLOLOLO");
		send_message(pfd, ":" + name + " " + c(RPL_ENDOFMOTD) + " " + users[pfd.fd].get_user() + " :End of /MOTD command.");
	}

	void parse_command(pollfd &pfd, const std::string &cmd)
	{
		User &user = users[pfd.fd];
		std::vector<std::string> args = split(cmd, ' ', true);

		if (args.size() < 1)
			return;

		if (!user.get_auth())
		{
			if (args[0] == "PASS")
			{
				std::cout << "pass: `" << args[1] << "`" << std::endl;
				if (args[1] == password)
				{
					std::cout << "yes" << std::endl;
					user.set_auth(true);
				}
				else
					std::cout << "no" << std::endl;
			}
			else if (args[0] != "CAP" && args[0] != "PROTOCTL")
			{
				terminate_connection(pfd);
				throw std::runtime_error("connection terminated");
			}
		}
		else if (args[0] == "USER")
		{
			if (args.size() < 5)
			{
				need_more_params(pfd, args[0]);
				return;
			}
			user.set_user(args[1]);
			user.set_real(join(args.begin() + 4, args.end(), " "));
			welcome(pfd);
		}
		else if (args[0] == "NICK")
		{
			if (user.get_nick() == args[1])
				return;

			bool taken = find_user_by_nickname(args[1]) != NULL;
			if (taken)
			{
				send_message(pfd, ":" + name + " " + c(ERR_NICKNAMEINUSE) + " " + args[1] + " :" + args[1]);
				return;
			}
			user.set_nick(args[1]);
			if (user.get_registered())
				send_message(pfd, ":" + name + " NICK " + args[1]);
			user.set_registered(true);
		}
		else if (args[0] == "LIST")
		{
			send_message(pfd, ":" + name + " " + c(RPL_LISTSTART) + " " + user.get_nick() + " Channel :Users Name");
			for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); it++)
				send_message(pfd, ":" + name + " " + c(RPL_LIST) + " " + user.get_nick() + " " + it->second.get_name() + " " + it->second.get_users_count() + " :");
			send_message(pfd, ":" + name + " " + c(RPL_LISTEND) + " " + user.get_nick() + " :End of /LIST");
		}
		else if (args[0] == "QUIT")
		{
			terminate_connection(pfd);
			throw std::runtime_error("connection terminated");
		}
		else if (args[0] == "JOIN")
		{
			if (args.size() < 2)
			{
				need_more_params(pfd, args[0]);
				return;
			}

			std::vector<std::string> params = split(args[1], ',');

			if (params.size() < 1)
			{
				need_more_params(pfd, args[0]);
				return;
			}

			std::vector<std::string> keys;

			if (args.size() > 2)
				keys = split(args[2], ',');

			for (size_t i = 0; i < params.size(); i++)
			{
				std::string channel_key = keys.size() > i ? keys[i] : "";
				if (channels.find(params[i]) != channels.end())
				{
					int ret = channels[params[i]].add_user(pfd, user, channel_key);
					if (ret == ERR_BADCHANNELKEY)
						send_message(pfd, ":" + name + " " + c(ERR_BADCHANNELKEY) + " " + user.get_nick() + " " + params[i] + " :Cannot join channel (+k)");
					else
					{
						send_message(pfd, ":" + user.get_hostmask() + " JOIN :" + params[i]);
						send_message(pfd, ":" + name + " " + c(RPL_TOPIC) + " " + user.get_nick() + " " + params[i] + " :" + channels[params[i]].get_topic());
						send_message(pfd, ":" + name + " " + c(RPL_NAMREPLY) + " " + user.get_nick() + " = " + params[i] + " :" + channels[params[i]].get_users_list());
						send_message(pfd, ":" + name + " " + c(RPL_ENDOFNAMES) + " " + user.get_nick() + " " + params[i] + " :End of /NAMES list");
					}
				}
				else
					no_such_channel(pfd, params[i]);
			}
		}
		else if (args[0] == "WHO")
		{
			if (args.size() < 2)
			{
				need_more_params(pfd, args[0]);
				return;
			}

			if (channels.find(args[1]) == channels.end())
			{
				no_such_channel(pfd, args[1]);
				return;
			}

			std::vector<std::string> who_list = channels[args[1]].get_who_list();

			for (size_t i = 0; i < who_list.size(); i++)
				send_message(pfd, ":" + name + " " + c(RPL_WHOREPLY) + " " + user.get_nick() + " " + args[1] + "	 " + who_list[i]);
			send_message(pfd, ":" + name + " " + c(RPL_ENDOFWHO) + " " + user.get_nick() + " " + args[1] + " :End of /WHO list");
		}
	}

	void parse_data(pollfd &pfd)
	{
		std::istringstream iss(users[pfd.fd].get_data());
		std::string line;

		std::cout << RED "PARSING DATA: " << escape(iss.str()) << RESET << std::endl;

		while (std::getline(iss, line))
		{
			std::cout << YELLOW << "Received from " << RESET << pfd.fd << YELLOW ": `" RESET << escape(line) << YELLOW "`" RESET << std::endl;
			try
			{
				if (line.substr(line.length() - 1) == "\r")
				{
					parse_command(pfd, line);
					users[pfd.fd].get_data().erase(0, line.length() + 1);
				}
				else
					break;
			}
			catch (...)
			{
				return;
			}
		}
		std::cout << RED "AFTER PARSING DATA: " << escape(users[pfd.fd].get_data()) << RESET << std::endl;
	}

	void process_events(pollfd &pfd)
	{
		if (pfd.fd == server_fd)
			accept_connections();
		else
		{
			receive_data(pfd);
			parse_data(pfd);
		}
	}

	void send_message(pollfd &pfd, const std::string &message)
	{
		std::cout << GREEN << "Sending to " << RESET << pfd.fd << GREEN ": `" RESET << escape(message) << GREEN "`" RESET << std::endl;
		std::string ircmsg(message + "\r\n");
		send(pfd.fd, ircmsg.c_str(), ircmsg.length(), 0);
	}

	static pollfd make_pfd(int fd, int events, int revents)
	{
		pollfd pfd = initialized<pollfd>();

		pfd.fd = fd;
		pfd.events = events;
		pfd.revents = revents;
		return pfd;
	}

	void terminate_connection(pollfd &pfd)
	{
		close(pfd.fd);
		for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); ++it)
			it->second.remove_user(pfd);
		users.erase(pfd.fd);
		for (std::vector<pollfd>::iterator it = fds.begin(); it != fds.end(); ++it)
		{
			if (it->fd == pfd.fd)
			{
				fds.erase(it);
				break;
			}
		}
	}

	User *find_user_by_nickname(const std::string &nickname)
	{
		for (std::map<int, User>::iterator it = users.begin(); it != users.end(); ++it)
		{
			if (it->second.get_nick() == nickname)
				return &it->second;
		}
		return NULL;
	}

	void need_more_params(pollfd &pfd, const std::string &command)
	{
		send_message(pfd, ":" + name + " " + c(ERR_NEEDMOREPARAMS) + " " + users[pfd.fd].get_nick() + " " + command + " :Not enough parameters");
	}

	void no_such_channel(pollfd &pfd, const std::string &channel)
	{
		send_message(pfd, ":" + name + " " + c(ERR_NOSUCHCHANNEL) + " " + users[pfd.fd].get_nick() + " " + channel + " :No such channel");
	}
};

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cout << "Usage: ./ircserv <port> <password>" << std::endl;
		return (EXIT_FAILURE);
	}

	try
	{
		Server server(argv[1], argv[2]);
		std::cout << "Server running on port " << argv[1] << std::endl;
		server.run();
	}
	catch (std::exception &e)
	{
		std::cout << e.what() << std::endl;
	}
	return (EXIT_SUCCESS);
}