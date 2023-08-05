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

class Server
{
private:
	std::string password;
	addrinfo *info;
	int server_fd;
	std::vector<pollfd> fds;
	std::map<int, std::stringstream> data_streams;
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
			data_streams[pfd.fd] << std::string(buffer);
			send(pfd.fd, buffer, length, 0);
		}
	}

#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

	void parse_data(pollfd &pfd)
	{
		// std::cout << YELLOW << "Parsing data on fd " << pfd.fd << ", good? " << data_streams[pfd.fd].good() << RESET << std::endl;
         std::string         line;

        while( std::getline(data_streams[pfd.fd], line) )
		{
			std::cout << line << std::endl;
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