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

enum
{
	RPL_WELCOME = 1,
	RPL_MOTD = 372,
	RPL_STARTOFMOTD = 375,
	RPL_ENDOFMOTD = 376,
	ERR_NICKNAMEINUSE = 433,
	RPL_LISTSTART = 321,
	RPL_LIST = 322,
	RPL_LISTEND = 323,
};

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

std::vector<std::string> split(const std::string &str)
{
	std::vector<std::string> splits;
	std::istringstream iss(str);
	std::string line;

	while (std::getline(iss, line, ' '))
	{
		if (line.empty())
			splits.back() += ' ';
		else
			splits.push_back(line);
	}
	splits.back().erase(splits.back().length() - 1);
	return splits;
}

std::string c(int code)
{
	std::stringstream ss;
	ss << std::setw(3) << std::setfill('0') << code;
	return ss.str();
}

class User
{
private:
	std::string nickname;
	std::string username;
	std::string datastream;
	bool authenticated;
	bool registered;

public:
	User() : authenticated(false), registered(false)
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

	void set_auth(bool auth) { authenticated = auth; }
	bool get_auth() { return authenticated; }

	void set_registered(bool reg) { registered = reg; }
	bool get_registered() { return registered; }

	void append_data(const std::string &data) { datastream += data; }
	std::string &get_data() { return datastream; }
};

class Channel
{
private:
	std::string name;
	std::vector<User *> users;

public:
	Channel(const std::string &n) : name(n)
	{
	}

	void add_user(User &user)
	{
		users.push_back(&user);
	}

	void remove_user(User &user)
	{
		for (size_t i = 0; i < users.size(); i++)
		{
			if (users[i] == &user)
			{
				users.erase(users.begin() + i);
				break;
			}
		}
	}
	const std::string &get_name() { return name; }
	std::string get_users_count()
	{
		std::stringstream ss;
		ss << users.size();
		return ss.str();
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
	std::vector<Channel> channels;

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

		// Initialize channels
		channels.push_back(Channel("global"));
		channels.push_back(Channel("hentai"));
	}
	~Server()
	{
		if (info)
			freeaddrinfo(info);
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
			new_fd = accept(server_fd, NULL, NULL);
			if (new_fd == -1)
				break;

			fds.push_back(make_pfd(new_fd, POLLIN, 0));
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
			// send(pfd.fd, buffer, length, 0);
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
		std::vector<std::string> args = split(cmd);

		if (args.size() < 1)
			return;

		// User not authenticated cannot send commands other than PASS

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
			user.set_user(args[1]);
			welcome(pfd);
		}
		else if (args[0] == "NICK")
		{
			std::cout << "nick: `" << args[1] << "`" << std::endl;
			std::cout << "getnick: `" << user.get_nick() << "`" << std::endl;
			if (user.get_nick() == args[1])
				return;

			bool taken = find_user_by_nickname(args[1]) != NULL;
			std::cout << "setting nickname " << args[1] << ", taken: " << taken << std::endl;
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
			for (size_t i = 0; i < channels.size(); i++)
				send_message(pfd, ":" + name + " " + c(RPL_LIST) + " " + user.get_nick() + " " + channels[i].get_name() + " " + channels[i].get_users_count() + " :");
			send_message(pfd, ":" + name + " " + c(RPL_LISTEND) + " " + user.get_nick() + " :End of /LIST");	
		}
	}

	void parse_data(pollfd &pfd)
	{
		std::istringstream iss(users[pfd.fd].get_data());
		std::string line;

		std::cout << RED"PARSING DATA: " << escape(iss.str()) << RESET << std::endl;

		while (std::getline(iss, line))
		{
			std::cout << YELLOW << "Received from " << RESET << pfd.fd << YELLOW ": `" RESET << escape(line) << YELLOW "`" RESET << std::endl;
			try {
				if (line.substr(line.length() - 1) == "\r")
				{
					parse_command(pfd, line);
					users[pfd.fd].get_data().erase(0, line.length() + 1);
				}
				else
					break;
			}
			catch (...) {
				return;
			}
		}
		std::cout << RED"AFTER PARSING DATA: " << escape(users[pfd.fd].get_data()) << RESET << std::endl;
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