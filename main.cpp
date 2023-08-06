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

bool starts_with(const std::string &str, const std::string &prefix)
{
	return str.compare(0, prefix.size(), prefix) == 0;
}

std::vector<std::string> split(const std::string &str)
{
	std::vector<std::string> splits;
	std::istringstream iss(str);
	std::string line;

	while (std::getline(iss, line, ' ')) {
		if (line.empty())
			splits.back() += ' ';
		else
			splits.push_back(line);
	}
	splits.back().erase(splits.back().length() - 1);
	return splits;
}

// fasfjasjfadjksABCABCABCdsdjsdsj

class User
{
	private:
		std::string nickname;
		std::string username;
		std::string datastream;
		bool authenticated;
	public:
		User() : authenticated(false) 
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

		void set_auth() { authenticated = true; }
		bool get_auth() { return authenticated; }

		void append_data(const std::string &data) { datastream += data; }
		std::string &get_data() { return datastream; }

};

class Server
{
private:
	std::string password;
	addrinfo *info;
	int server_fd;
	std::vector<pollfd> fds;
	std::map<int, User> users;
	bool running;

public:
	Server(const std::string &port, const std::string &pass) : password(pass), info(NULL), running(true)
	{
		int on = 1;
		addrinfo hints = initialized<addrinfo>();

		hints.ai_family = AF_INET;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_socktype = SOCK_STREAM;

		insist(getaddrinfo(NULL, port.c_str(), &hints, &info), -1, "getaddrinfo failed");
		insist(info == NULL, true, "info is null");

		server_fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

		insist(server_fd, -1, "socket failed");
		insist(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(int)), -1, "setsockopt failed");
		insist(fcntl(server_fd, F_SETFL, O_NONBLOCK), -1, "fcntl failed");
		insist(bind(server_fd, info->ai_addr, info->ai_addrlen) != 0, true, "bind failed");
		insist(listen(server_fd, 69), -1, "listen failed");

		fds.push_back(make_pfd(server_fd, POLLIN, 0));
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

	void receive_data(pollfd & pfd)
	{
		char buffer[1024];
		int	length;
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
		std::string welcome = ":slime.gay 001 " + users[pfd.fd].get_nick() + ": Welcome!";
		std::cout << welcome << std::endl;
		send(pfd.fd, welcome.c_str(), welcome.length(), 0);
	}

	void parse_command(pollfd &pfd, const std::string &cmd)
	{
		(void)pfd;
		std::vector<std::string> args = split(cmd);
		if (args.size() < 1)
			return;

		if (args[0] == "PROTOCTL")
			std::cout << "Go fuck yourself." << std::endl;
		else if (args[0] == "PASS")
		{
			std::cout << "pass: `" << args[1] << "`" << std::endl;
			if (args[1] == password)
				std::cout << "yes" << std::endl;
			else
				std::cout << "no" << std::endl;
		}
		else if (args[0] == "USER")
		{
			users[pfd.fd].set_user(args[1]);
			welcome(pfd);
		}
		else if (args[0] == "NICK")
		{
			users[pfd.fd].set_nick(args[1]);
		}
	}

	void parse_data(pollfd &pfd)
	{
		std::istringstream iss(users[pfd.fd].get_data());
		std::string line;

		while (std::getline(iss, line))
		{
			std::cout << YELLOW << line << RESET << std::endl;
			parse_command(pfd, line);
		}
	}

	void process_events(pollfd &pfd)
	{
		if (pfd.fd == server_fd)
			accept_connections();
		else {
			receive_data(pfd);
			parse_data(pfd);
		}
	}

	static pollfd make_pfd(int fd, int events, int revents)
	{
		pollfd pfd = initialized<pollfd>();

		pfd.fd = fd;
		pfd.events = events;
		pfd.revents = revents;
		return pfd;
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