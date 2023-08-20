#include "Server.hpp"

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
		server.initialize_bot();
		server.run();
	}
	catch (std::exception &e)
	{
		std::cout << e.what() << std::endl;
	}
	return (EXIT_SUCCESS);
}
