#include "Server.hpp"
#include "Channel.hpp"
#include "User.hpp"


CommandInfo Server::commands[] = {
	{"PASS", &Server::PASS, false},
	{"USER", &Server::USER, false},
	{"NICK", &Server::NICK, false},
	{"LIST", &Server::LIST, true},
	{"QUIT", &Server::QUIT, false},
	{"JOIN", &Server::JOIN, true},
	{"WHO", &Server::WHO, true},
	{"PRIVMSG", &Server::PRIVMSG, true},
	{"ISON", &Server::ISON, true},
	{"PART", &Server::PART, true},
	{"PING", &Server::PING, true},
	{"OPER", &Server::OPER, true},
	{"KICK", &Server::KICK, true},
	{"INVITE", &Server::INVITE, true},
	{"TOPIC", &Server::TOPIC, true},
	{"MODE", &Server::MODE, true},
	{"CAP", &Server::IGNORED, false},
	{"PROCTL", &Server::IGNORED, false},
	{"PONG", &Server::IGNORED, false},
};

bool Server::load_config_channel(std::ifstream &file)
{
	std::string ele[3];

	if (!std::getline(file, ele[0]))
		return false;
	if (!std::getline(file, ele[1]))
		return false;
	if (!std::getline(file, ele[2]))
		return false;

	std::string name, key, topic;
	for (size_t i = 0; i < 3; i++)
	{
		ele[i] = trimstr(ele[i]);
		std::vector<std::string> subargs = split(ele[i], ':', false);
		subargs.push_back("");

		std::string key = trimstr(subargs[0]);
		std::string value = "";
		if (subargs.size() > 1)
			value = trimstr(join(subargs.begin() + 1, subargs.end() - 1, ":"));

		if (key == "- name")
			name = "#" + value;
		else if (key == "- key")
			key = value;
		else if (key == "- topic")
			topic = value;
		else
			return false;
	}
	if (name.empty())
		return false;
	create_channel(name, key, topic);
	return true;
}

bool Server::load_config(const std::string &filename)
{
	std::ifstream file(filename.c_str());
	std::string line;

	if (!file.is_open())
		return false;

	while (std::getline(file, line))
	{
		line = trimstr(line);
		if (line.empty() || line[0] == '#')
			continue;
		std::vector<std::string> args = split(line, ':', false);
		if (args[0] == "channel")
		{
			if (load_config_channel(file))
				continue;
			return false;
		}
		if (args.size() < 2)
		{
			std::cout << "args.size() < 2" << std::endl;
			return false;
		}
		std::string key = trimstr(args[0]);
		std::string value = trimstr(args[1]);
		if (key.empty() || value.empty())
		{
			std::cout << key << " or " << value << " is empty" << std::endl;
			return false;
		}
		configs[key] = value;
	}
	return true;
}

bool Server::verify_server_name()
{
	std::string str = conf.name;

	if (str.length() > conf.max_server_name_length)
		return false;

	std::vector<std::string> splits = split(str, '.', false);

	if (splits.size() == 0)
		return false;

	for (size_t i = 0; i < splits.size(); i++)
	{
		std::string sub = splits[i];

		if (splits[i].empty())
			return false;
		if (!verify_string(sub.substr(0, 1), LETTER | DIGIT) || !verify_string(sub.substr(sub.length() - 1), LETTER | DIGIT) || !verify_string(sub.substr(1, sub.length() - 2), LETTER | DIGIT | DASH))

		{
			std::cout << "NOT VERIFIED " << std::endl;
			return false;
		}
	}
	return true;
}

Server::Server(const std::string &port, const std::string &pass) : running(true), info(NULL)
{
	insist(load_config("irc.yaml"), false, "failed to load config");

	conf.password = pass.empty() ? OPTIONAL_PCONF(server, password) : pass;
	conf.port = port.empty() ? OPTIONAL_PCONF(server, port) : port;
	REQUIRE_CONF(motd);
	REQUIRE_PCONF(server, name);
	REQUIRE_CONF(operator_username);
	REQUIRE_CONF(operator_password);
	REQUIRE_CONF(operator_password);
	REQUIRE_CONF_NUMBER(activity_timeout, int);
	REQUIRE_CONF_NUMBER(ping_timeout, int);
	REQUIRE_CONF_NUMBER(max_message_length, int);
	REQUIRE_CONF_NUMBER(max_nickname_length, int);
	REQUIRE_CONF_NUMBER(max_server_name_length, int);
	REQUIRE_CONF_NUMBER(max_channel_name_length, int);

	REQUIRE_CONF(bot.nickname);
	REQUIRE_CONF(bot.username);
	REQUIRE_CONF(bot.realname);
	REQUIRE_CONF_NUMBER(bot.fd, int);

	insist(verify_server_name(), false, "invalid server name");

	int on = 1;

	sockaddr_in addr = initialized<sockaddr_in>();

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(to_number<int>(conf.port));

	insist(server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP), -1, "socket failed");
	insist(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(int)), -1, "setsockopt failed");
	insist(fcntl(server_fd, F_SETFL, O_NONBLOCK), -1, "fcntl failed");
	insist(bind(server_fd, (sockaddr *)&addr, sizeof(addr)) != 0, true, "bind failed");
	insist(listen(server_fd, 69), -1, "listen failed");

	pfds.push_back(make_pfd(server_fd, POLLIN, 0));
}
Server::~Server()
{
	if (info)
		freeaddrinfo(info);
	for (UserList::iterator it = users.begin(); it != users.end(); ++it)
		delete it->second;
}

void Server::create_channel(const std::string &name, const std::string &key, const std::string &topic)
{
	insist(verify_string(name, CHANNEL) && name.length() <= 50, false, "invalid channel name");
	insist(channels.find(name) == channels.end(), false, "channel already exists");
	insist(verify_string(key, KEY) && key.length() <= 23, false, "invalid channel key");
	channels[name] = Channel(name, key, topic);
}

void Server::run()
{
	while (running)
	{
		insist(poll(&pfds[0], pfds.size(), -1), -1, "poll failed");
		for (size_t i = 0; i < pfds.size(); i++)
			process_events(pfds[i].fd, pfds[i].revents);
		if (conf.bot.fd < 0)
		{
			if (bot_parse())
				parse_data(conf.bot.fd);
		}
	}
}

void Server::accept_connections()
{
	int new_fd = 0;

	while (true)
	{
		sockaddr addr;
		socklen_t len = sizeof(addr);

		new_fd = accept(server_fd, &addr, &len);
		if (new_fd == -1)
			break;
		fcntl(new_fd, F_SETFL, O_NONBLOCK);
		pfds.push_back(make_pfd(new_fd, POLLIN | POLLOUT, 0));
		users[new_fd] = new User();
		users[new_fd]->set_fd(new_fd);
		users[new_fd]->set_host(addr);
		std::cout << "Connection accepted on fd " << new_fd << std::endl;
	}
}

void Server::receive_data(int fd)
{
	char buffer[1024];
	int length;
	while (true)
	{
		length = recv(fd, buffer, sizeof(buffer) - 1, 0);
		if (length <= 0)
			break;

		buffer[length] = 0;
		std::cout << RED << "APPENDING: " << escape(buffer) << RESET << std::endl;
		users[fd]->append_data(std::string(buffer));
		users[fd]->set_last_activity();
	}
}

void Server::welcome(int fd)
{
	send_message(fd, ":" + conf.name + " " + c(RPL_WELCOME) + " " + users[fd]->get_nick() + " :kys " + users[fd]->get_nick() + "!" + users[fd]->get_user() + "@" + conf.name);
	send_message(fd, ":" + conf.name + " " + c(RPL_ISUPPORT) + " " + users[fd]->get_nick() + " CHANMODES=k,l,it :are supported by this server");
	send_message(fd, ":" + conf.name + " " + c(RPL_STARTOFMOTD) + " " + users[fd]->get_user() + " :- " + conf.name + " Message of the Day -");
	send_message(fd, ":" + conf.name + " " + c(RPL_MOTD) + " " + users[fd]->get_nick() + " :- " + conf.motd);
	send_message(fd, ":" + conf.name + " " + c(RPL_ENDOFMOTD) + " " + users[fd]->get_user() + " :End of /MOTD command.");
}

void Server::PASS(int fd, User *user, std::vector<std::string> &args)
{
	(void)args;
	(void)user;
	already_registered(fd);
}

void Server::USER(int fd, User *user, std::vector<std::string> &args)
{
	CHECK_ARGS(5);

	if (user->get_user() != "")
	{
		already_registered(fd);
		return;
	}

	std::string username = args[1];
	std::string realname = join(args.begin() + 4, args.end(), " ").substr(1);
	bool username_valid = verify_string(username, USERNAME);
	bool realname_valid = verify_string(realname, LETTER | SPACE);
	
	if (!username_valid || !realname_valid)
		return;

	user->set_user(username);
	user->set_real(realname);
	if (user->get_registered())
		welcome(fd);
	return;
}

void Server::NICK(int fd, User *user, std::vector<std::string> &args)
{
	CHECK_ARGS(2);

	std::string nickname = args[1];
	std::string nickname_first = nickname.substr(0, 1);
	std::string nickname_rest = nickname.substr(1);
	bool length_valid = nickname.length() <= conf.max_nickname_length;
	bool nickname_first_valid = verify_string(nickname_first, LETTER | SPECIAL);
	bool nickname_rest_valid = verify_string(nickname_rest, LETTER | DIGIT | SPECIAL | DASH);

	if (user->get_nick() == nickname)
		return;

	if (!length_valid || !nickname_first_valid || !nickname_rest_valid)
	{
		send_message(fd, ":" + conf.name + " " + c(ERR_ERRONEUSNICKNAME) + " " + nickname + " :Erroneous nickname");
		return;
	}

	if (find_user_by_nickname(nickname) != NULL)
	{
		send_message(fd, ":" + conf.name + " " + c(ERR_NICKNAMEINUSE) + " " + nickname + " :" + nickname);
		return;
	}

	user->set_nick(nickname);
	if (user->get_registered())
		send_message(fd, ":" + conf.name + " NICK " + nickname);
	user->set_registered(true);
	if (user->get_user() != "")
		welcome(fd);
}

void Server::LIST(int fd, User *user, std::vector<std::string> &args)
{
	(void)args;

	send_message(fd, ":" + conf.name + " " + c(RPL_LISTSTART) + " " + user->get_nick() + " Channel :Users Name");
	for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); it++)
		send_message(fd, ":" + conf.name + " " + c(RPL_LIST) + " " + user->get_nick() + " " + it->second.get_name() + " " + it->second.get_users_count() + " :" + it->second.get_topic());
	send_message(fd, ":" + conf.name + " " + c(RPL_LISTEND) + " " + user->get_nick() + " :End of /LIST");
}

void Server::QUIT(int fd, User *user, std::vector<std::string> &args)
{
	(void)user;
	(void)args;

	terminate_connection(fd);
	throw std::runtime_error("connection terminated");
}

void Server::JOIN(int fd, User *user, std::vector<std::string> &args)
{
	CHECK_ARGS(2);

	std::vector<std::string> params = split(args[1], ',');

	if (params.size() < 1)
	{
		need_more_params(fd, args[0]);
		return;
	}

	std::vector<std::string> keys;

	if (args.size() > 2)
		keys = split(args[2], ',');

	for (size_t i = 0; i < params.size(); i++)
	{
		std::string channel_key = keys.size() > i ? keys[i] : "";
		std::cout << "channel: " << params[i] << std::endl;
		std::cout << "channel_key: " << channel_key << std::endl;
		if (channels.find(params[i]) != channels.end())
		{
			Channel &channel = channels[params[i]];
			if (channel.is_invited(user))
			{
				if (!channel.has_mode(MODE_LIMIT) || channel.get_users().size() < channel.get_limit())
				{
					int ret = channel.add_user(fd, user, channel_key);
					if (ret == ERR_BADCHANNELKEY)
						send_message(fd, ":" + conf.name + " " + c(ERR_BADCHANNELKEY) + " " + user->get_nick() + " " + params[i] + " :Cannot join channel (+k)");
					else
					{
						channel.remove_invite(user);
						broadcast_message(channel, ":" + user->get_hostmask(user->get_nick()) + " JOIN :" + params[i]);
						send_message(fd, ":" + conf.name + " " + c(RPL_TOPIC) + " " + user->get_nick() + " " + params[i] + " :" + channel.get_topic());
						send_message(fd, ":" + conf.name + " " + c(RPL_NAMREPLY) + " " + user->get_nick() + " = " + params[i] + " :" + channel.get_users_list());
						send_message(fd, ":" + conf.name + " " + c(RPL_ENDOFNAMES) + " " + user->get_nick() + " " + params[i] + " :End of /NAMES list");
					}
				}
				else
					send_message(fd, ":" + conf.name + " " + c(ERR_CHANNELISFULL) + " " + user->get_nick() + " " + params[i] + " :Cannot join channel (+l)");
			}
			else
				send_message(fd, ":" + conf.name + " " + c(ERR_INVITEONLYCHAN) + " " + user->get_nick() + " " + params[i] + " :Cannot join channel (+i)");
		}
		else
			no_such_channel(fd, params[i]);
	}
}

void Server::WHO(int fd, User *user, std::vector<std::string> &args)
{
	CHECK_ARGS(2);
	CHECK_CHANNEL(args[1]);

	std::vector<std::string> who_list = channel.get_who_list();

	for (size_t i = 0; i < who_list.size(); i++)
		send_message(fd, ":" + conf.name + " " + c(RPL_WHOREPLY) + " " + user->get_nick() + " " + args[1] + " " + who_list[i]);
	send_message(fd, ":" + conf.name + " " + c(RPL_ENDOFWHO) + " " + user->get_nick() + " " + args[1] + " :End of /WHO list");
}

void Server::PRIVMSG(int fd, User *user, std::vector<std::string> &args)
{
	CHECK_ARGS(3);

	std::string message = join(args.begin() + 2, args.end(), " ");
	if (args[1][0] == '#')
	{
		CHECK_CHANNEL(args[1]);

		if (!channel.has_user(fd))
		{
			not_on_channel(fd, args[1]);
			return;
		}

		broadcast_message(channel, ":" + user->get_hostmask(user->get_nick()) + " PRIVMSG " + args[1] + " " + message, users[fd]);
	}
	else
	{
		User *target = find_user_by_nickname(args[1]);
		if (target == NULL)
		{
			no_such_nick(fd, args[1]);
			return;
		}

		send_message(target->get_fd(), ":" + user->get_hostmask(user->get_nick()) + " PRIVMSG " + target->get_nick() + " " + message);
	}
}

void Server::ISON(int fd, User *user, std::vector<std::string> &args)
{
	CHECK_ARGS(2);

	if (args[1][0] == '#')
	{
		if (channels.find(args[1]) == channels.end())
		{
			send_message(fd, ":" + conf.name + " " + c(RPL_NOWOFF) + " " + user->get_nick() + " " + args[1] + " :" + args[1] + " * * 0 is offline");
			return;
		}
	}
	else
	{
		User *user = find_user_by_nickname(args[1]);
		if (user == NULL)
		{
			send_message(fd, ":" + conf.name + " " + c(RPL_ISON) + " " + user->get_nick() + " :" + args[1]);
			return;
		}
	}
	send_message(fd, ":" + conf.name + " " + c(RPL_ISON) + " " + user->get_nick() + " :");
}

void Server::PART(int fd, User *user, std::vector<std::string> &args)
{
	CHECK_ARGS(3);
	CHECK_CHANNEL(args[1]);

	if (!channel.has_user(fd))
	{
		not_on_channel(fd, args[1]);
		return;
	}

	broadcast_message(channel, ":" + user->get_hostmask(user->get_nick()) + " PART " + args[1] + " " + join(args.begin() + 2, args.end(), " "));

	channel.remove_user(fd);
}

void Server::PING(int fd, User *user, std::vector<std::string> &args)
{
	(void)user;

	std::string message = join(args.begin() + 1, args.end(), " ");
	send_message(fd, ":" + conf.name + " PONG " + conf.name + " :" + message);
}

void Server::OPER(int fd, User *user, std::vector<std::string> &args)
{
	CHECK_ARGS(3);

	bool authenticated = args[1] == conf.operator_username && args[2] == conf.operator_password;
	if (authenticated)
	{
		operators[fd] = user;
		user->set_server_operator(true);
		send_message(fd, ":" + conf.name + " " + c(RPL_YOUREOPER) + " " + user->get_nick() + " :You are now an IRC operator");
		server_broadcast_message(":" + conf.name + " MODE " + user->get_nick() + " :+o");
	}
	else
	{
		send_message(fd, ":" + conf.name + " " + c(ERR_PASSWDMISMATCH) + " " + user->get_nick() + " :Password incorrect");
	}
}

void Server::KICK(int fd, User *user, std::vector<std::string> &args)
{
	CHECK_ARGS(3);
	CHECK_CHANNEL(args[1]);

	if (!channel.has_user(fd))
	{
		not_on_channel(fd, args[1]);
		return;
	}

	OPER_START();
	User *target = find_user_by_nickname(args[2]);

	if (target == NULL)
	{
		no_such_nick(fd, args[2]);
		return;
	}

	if (channel.has_user(target->get_fd()))
	{
		broadcast_message(channel, ":" + user->get_hostmask(user->get_nick()) + " KICK " + args[1] + " " + args[2] + " :");
		channel.remove_user(target->get_fd());
	}
	else
		user_not_in_channel(fd, args[2], channel.get_name());
	OPER_END();
}

void Server::INVITE(int fd, User *user, std::vector<std::string> &args)
{
	CHECK_ARGS(3);
	CHECK_CHANNEL(args[2]);

	if (!channel.has_user(fd))
	{
		not_on_channel(fd, args[2]);
		return;
	}

	User *target = find_user_by_nickname(args[1]);

	if (target == NULL)
	{
		no_such_nick(fd, args[1]);
		return;
	}

	std::cout << "target: " << target->get_nick() << std::endl;

	if (user->is_server_operator() || channel.is_operator(user))
	{
		if (channel.has_user(target->get_fd()))
		{
			send_message(fd, ":" + conf.name + " " + c(ERR_USERONCHANNEL) + " " + user->get_nick() + " " + args[1] + " " + args[2] + " :is already on channel");
			return;
		}
		send_message(target->get_fd(), ":" + user->get_hostmask(user->get_nick()) + " INVITE " + target->get_nick() + " " + args[2]);
		send_message(fd, ":" + conf.name + " " + c(RPL_INVITING) + " " + user->get_nick() + " " + args[1] + " " + args[2]);
		channel.invite(target);
	}
	else
		channel_operator_privileges_needed(fd, channel.get_name());
}

void Server::TOPIC(int fd, User *user, std::vector<std::string> &args)
{
	CHECK_ARGS(2);
	CHECK_CHANNEL(args[1]);

	if (!channel.has_user(fd))
	{
		not_on_channel(fd, args[1]);
		return;
	}

	if (args.size() == 2)
	{
		send_message(fd, ":" + conf.name + " " + c(RPL_TOPIC) + " " + user->get_nick() + " " + args[1] + " :" + channel.get_topic());
		return;
	}

	if (user->is_server_operator() || channel.is_operator(user) || !channel.has_mode(MODE_TOPIC))
	{
		channel.set_topic(join(args.begin() + 2, args.end(), " ").substr(1));
		broadcast_message(channel, ":" + user->get_hostmask(user->get_nick()) + " TOPIC " + args[1] + " :" + channel.get_topic());
	}
	else
		channel_operator_privileges_needed(fd, channel.get_name());
}

void Server::MODE(int fd, User *user, std::vector<std::string> &args)
{
	CHECK_ARGS(2);
	CHECK_CHANNEL(args[1]);

	if (!channel.has_user(fd))
	{
		not_on_channel(fd, args[1]);
		return;
	}

	if (args.size() == 2)
	{
		std::string modes = "n";
		std::string values;

		if (channel.has_mode(MODE_INVITEONLY))
			modes += "i";
		if (channel.has_mode(MODE_TOPIC))
			modes += "t";
		if (channel.has_mode(MODE_KEY))
		{
			modes += "k";
			values += channel.get_key() + " ";
		}
		if (channel.has_mode(MODE_LIMIT))
		{
			modes += "l";
			values += to_string(channel.get_limit());
		}

		send_message(fd, ":" + conf.name + " " + c(RPL_CHANNELMODEIS) + " " + user->get_nick() + " " + args[1] + " +" + modes + " " + values);
		return;
	}

	char operation = args[2][0];

	if (operation != '+' && operation != '-')
	{
		send_message(fd, ":" + conf.name + " " + c(ERR_UNKNOWNMODE) + " " + user->get_nick() + " " + args[2] + " :retard");
		return;
	}

	int mode = 0;
	bool err = false;
	size_t arg_idx = 3;
	for (size_t i = 1; i < args[2].length(); i++)
	{
		switch (args[2][i])
		{
			SET_MODE_OR_ERR('i', MODE_INVITEONLY);
			SET_MODE_OR_ERR('t', MODE_TOPIC);

			SET_MODE_OR_ERR('o', MODE_OPERATOR);
			SET_MODE_OR_ERR('k', MODE_KEY);
			SET_MODE_OR_ERR('l', MODE_LIMIT);
		default:
			err = true;
			break;
		}
		if (err)
		{
			send_message(fd, ":" + conf.name + " " + c(ERR_UNKNOWNMODE) + " " + user->get_nick() + " " + args[2][i] + " :retard");
			return;
		}
		else
		{
			if (mode & MODE_INVITEONLY)
			{
				OPER_START();
				if (operation == '+')
					channel.set_mode(channel.get_mode() | MODE_INVITEONLY);
				else
					channel.set_mode(channel.get_mode() & ~MODE_INVITEONLY);
				broadcast_message(channel, ":" + user->get_hostmask(user->get_nick()) + " MODE " + args[1] + " :" + operation + "i");
				OPER_END();
			}
			if (mode & MODE_OPERATOR)
			{
				OPER_START();
				CHECK_ARGS(arg_idx + 1);
				User *target = find_user_by_nickname(args[arg_idx]);
				if (target)
				{
					if (channel.has_user(target->get_fd()))
					{
						if (operation == '+')
							channel.add_operator(target);
						else if (operation == '-')
							channel.remove_operator(target);
						broadcast_message(channel, ":" + conf.name + " " + c(RPL_CHANNELMODEIS) + " " + user->get_nick() + " " + args[1] + " " + operation + "o " + target->get_nick());
					}
					else
						user_not_in_channel(fd, args[arg_idx], channel.get_name());
				}
				else
					no_such_nick(fd, args[arg_idx]);
				OPER_END();
				arg_idx++;
			}
			if (mode & MODE_TOPIC)
			{
				OPER_START();
				if (operation == '+')
					channel.set_mode(channel.get_mode() | MODE_TOPIC);
				else
					channel.set_mode(channel.get_mode() & ~MODE_TOPIC);
				broadcast_message(channel, ":" + user->get_hostmask(user->get_nick()) + " MODE " + args[1] + " :" + operation + "t");
				OPER_END();
			}
			if (mode & MODE_LIMIT)
			{
				OPER_START();
				if (operation == '+')
				{
					CHECK_ARGS(arg_idx + 1);
					channel.set_mode(channel.get_mode() | MODE_LIMIT);
					channel.set_limit(std::atoi(args[arg_idx].c_str()));
					broadcast_message(channel, ":" + user->get_hostmask(user->get_nick()) + " MODE " + args[1] + " :" + operation + "l " + args[arg_idx]);
					arg_idx++;
				}
				else if (operation == '-' && channel.has_mode(MODE_LIMIT))
				{
					channel.set_mode(channel.get_mode() & ~MODE_LIMIT);
					broadcast_message(channel, ":" + user->get_hostmask(user->get_nick()) + " MODE " + args[1] + " :" + operation + "l");
				}
				OPER_END();
			}
			if (mode & MODE_KEY)
			{
				OPER_START();
				CHECK_ARGS(arg_idx + 1);
				if (operation == '+' && !channel.has_mode(MODE_KEY))
				{
					channel.set_mode(channel.get_mode() | MODE_KEY);
					channel.set_key(args[arg_idx]);
					broadcast_message(channel, ":" + user->get_hostmask(user->get_nick()) + " MODE " + args[1] + " :" + operation + "k " + channel.get_key());
				}
				else if (operation == '-' && channel.has_mode(MODE_KEY))
				{
					channel.set_mode(channel.get_mode() & ~MODE_KEY);
					broadcast_message(channel, ":" + user->get_hostmask(user->get_nick()) + " MODE " + args[1] + " :" + operation + "k");
				}
				OPER_END();
				arg_idx++;
			}
		}
		mode = 0;
	}
}

void Server::IGNORED(int fd, User *user, std::vector<std::string> &args)
{
	(void)fd;
	(void)user;
	(void)args;
}

void Server::parse_command(int fd, const std::string &cmd)
{
	User *user = users[fd];
	int command_idx = is_valid_command(cmd);

	if (command_idx == INVALID_COMMAND)
	{
		user->get_registered() ? unknown_command(fd, cmd) : not_registered(fd);
		return ;
	}
		
	std::vector<std::string> args = split(cmd, ' ', true);

	if (args.size() < 1)
		return;

	if (!user->get_auth())
	{
		if (args[0] == "PASS")
		{
			CHECK_ARGS(2);

			if (user->get_registered())
			{
				already_registered(fd);
				return;
			}

			if (args[1] == conf.password)
				user->set_auth(true);
		}
		else
		{
			not_registered(fd);
		}
	}
	else
	{
		if (commands[command_idx].need_registered && !user->get_registered())
			not_registered(fd);
		else
			(this->*commands[command_idx].func)(fd, user, args);
	}
}

void Server::parse_data(int fd)
{
	std::istringstream iss(users[fd]->get_data());
	std::string line;

	if (users[fd]->get_data().length() > conf.max_message_length)
	{
		terminate_connection(fd);
		return;
	}

	std::cout << RED "PARSING DATA: " << escape(iss.str()) << RESET << std::endl;
	while (std::getline(iss, line))
	{
		std::cout << YELLOW << "Received from " << RESET << fd << YELLOW ": `" RESET << escape(line) << YELLOW "`" RESET << std::endl;
		try
		{
			if (line.substr(line.length() - 1) == "\r")
			{
				parse_command(fd, line);
				users[fd]->get_data().erase(0, line.length() + 1);
			}
			else
				break;
		}
		catch (...)
		{
			return;
		}
	}
	std::cout << RED "AFTER PARSING DATA: " << escape(users[fd]->get_data()) << RESET << std::endl;
}

int Server::is_valid_command(const std::string &line)
{
	for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
	{
		if (line.substr(0, commands[i].name.length() + 1) == commands[i].name + " ")
			return i;
	}
	return INVALID_COMMAND;
}

void Server::process_events(int fd, int revents)
{
	// std::cout << MAGENTA << "revents: " << revents << RESET << std::endl;
	if (revents & POLLIN)
	{
		if (fd == server_fd)
		{
			accept_connections();
		}
		else
		{
			receive_data(fd);
			parse_data(fd);
		}
	}
	if (revents & (POLLHUP | POLLERR))
	{
		terminate_connection(fd);
		return;
	}
	if (revents & POLLOUT)
	{
		if (users.find(fd) != users.end() && users[fd]->get_sendbuffer().length() > 0)
		{
			send(fd, users[fd]->get_sendbuffer().c_str(), users[fd]->get_sendbuffer().length(), 0);
			users[fd]->clear_sendbuffer();
		}
	}
	if (fd != server_fd && users.find(fd) != users.end())
	{
		time_t ping = conf.activity_timeout;
		time_t timeout = conf.ping_timeout;
		time_t now = time(NULL);

		if (now - users[fd]->get_last_activity() >= ping)
		{
			if (users[fd]->get_last_ping() <= users[fd]->get_last_activity())
			{
				send_message(fd, "PING " + to_string(now));
				users[fd]->set_last_ping();
			}
			else if (now - users[fd]->get_last_ping() >= timeout)
			{
				terminate_connection(fd);
				return;
			}
		}
	}
}

void Server::send_message(int fd, const std::string &message)
{
	std::cout << GREEN << "Sending to " << RESET << fd << GREEN ": `" RESET << escape(message) << GREEN "`" RESET << std::endl;
	std::string ircmsg(message + "\r\n");
	users[fd]->append_sendbuffer(ircmsg);
	// send(fd, ircmsg.c_str(), ircmsg.length(), 0);
}

void Server::broadcast_message(Channel &channel, const std::string &message, User *except)
{
	std::cout << BLUE << "Broadcasting to " << RESET << channel.get_name() << BLUE ": `" RESET << escape(message) << BLUE "`" RESET << std::endl;
	std::string ircmsg(message + "\r\n");
	for (UserList::iterator it = channel.get_users().begin(); it != channel.get_users().end(); ++it)
	{
		if (it->second != except)
			users[it->first]->append_sendbuffer(ircmsg);
	}
}

void Server::server_broadcast_message(const std::string &message, User *except)
{
	std::cout << PURPLE << "Broadcasting to " << RESET << "Server" << PURPLE ": `" RESET << escape(message) << PURPLE "`" RESET << std::endl;
	std::string ircmsg(message + "\r\n");
	for (UserList::iterator it = users.begin(); it != users.end(); ++it)
	{
		if (it->second != except && it->second->get_registered())
			users[it->first]->append_sendbuffer(ircmsg);
	}
}

void Server::broadcast_user_channels(int fd, const std::string &message)
{
	for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); ++it)
	{
		if (it->second.has_user(fd))
			broadcast_message(it->second, message, users[fd]);
	}
}

pollfd Server::make_pfd(int fd, int events, int revents)
{
	pollfd pfd = initialized<pollfd>();

	pfd.fd = fd;
	pfd.events = events;
	pfd.revents = revents;
	return pfd;
}

void Server::terminate_connection(int fd)
{
	broadcast_user_channels(fd, ":" + users[fd]->get_hostmask(users[fd]->get_nick()) + " QUIT :Client closed connection");
	for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); ++it)
		it->second.remove_user(fd);
	users.erase(fd);
	for (std::vector<pollfd>::iterator it = pfds.begin(); it != pfds.end(); ++it)
	{
		if (it->fd == fd)
		{
			pfds.erase(it);
			break;
		}
	}
	close(fd);
}

User *Server::find_user_by_nickname(const std::string &nickname)
{
	for (UserList::iterator it = users.begin(); it != users.end(); ++it)
	{
		if (it->second->get_nick() == nickname)
			return it->second;
	}
	return NULL;
}

void Server::need_more_params(int fd, const std::string &command)
{
	send_message(fd, ":" + conf.name + " " + c(ERR_NEEDMOREPARAMS) + " " + users[fd]->get_nick() + " " + command + " :Not enough parameters");
}

void Server::no_such_channel(int fd, const std::string &channel)
{
	send_message(fd, ":" + conf.name + " " + c(ERR_NOSUCHCHANNEL) + " " + users[fd]->get_nick() + " " + channel + " :No such channel");
}

void Server::not_on_channel(int fd, const std::string &channel)
{
	send_message(fd, ":" + conf.name + " " + c(ERR_NOTONCHANNEL) + " " + users[fd]->get_nick() + " " + channel + " :You're not on that channel");
}

void Server::no_such_nick(int fd, const std::string &nickname)
{
	send_message(fd, ":" + conf.name + " " + c(ERR_NOSUCHNICK) + " " + users[fd]->get_nick() + " " + nickname + " :No such nick/channel");
}

void Server::user_not_in_channel(int fd, const std::string &nickname, const std::string &channel)
{
	send_message(fd, ":" + conf.name + " " + c(ERR_USERNOTINCHANNEL) + " " + users[fd]->get_nick() + " " + nickname + " " + channel + " :They aren't on that channel");
}

void Server::channel_operator_privileges_needed(int fd, const std::string &channel)
{
	send_message(fd, ":" + conf.name + " " + c(ERR_CHANOPRIVSNEEDED) + " " + users[fd]->get_nick() + " " + channel + " :You're not channel operator");
}

void Server::already_registered(int fd)
{
	send_message(fd, ":" + conf.name + " " + c(ERR_ALREADYREGISTRED) + " " + users[fd]->get_nick() + " :You may not reregister");
}

void Server::unknown_command(int fd, const std::string &command)
{
	send_message(fd, ":" + conf.name + " " + c(ERR_UNKNOWNCOMMAND) + " " + users[fd]->get_nick() + " " + command + " :Unknown command");
}

void Server::not_registered(int fd)
{
	send_message(fd, ":" + conf.name + " " + c(ERR_NOTREGISTERED) + " " + users[fd]->get_nick() + " :You have not registered");
}

bool Server::is_operator(int fd)
{
	return operators.find(fd) != operators.end();
}

int Server::add_operator(User *user)
{
	operators[user->get_fd()] = user;
	return 0;
}

void Server::remove_operator(User *user)
{
	operators.erase(user->get_fd());
}

void Server::initialize_bot()
{
	users[conf.bot.fd] = new User();
	users[conf.bot.fd]->set_fd(conf.bot.fd);
	users[conf.bot.fd]->set_nick(conf.bot.nickname);
	users[conf.bot.fd]->set_user(conf.bot.username);
	users[conf.bot.fd]->set_real(conf.bot.realname);
	users[conf.bot.fd]->set_host("0.0.0.0");
	users[conf.bot.fd]->set_auth(true);
	users[conf.bot.fd]->set_registered(true);
	users[conf.bot.fd]->set_server_operator(true);

	for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); it++)
	{
		it->second.add_user(conf.bot.fd, users[conf.bot.fd], it->second.get_key());
		it->second.add_operator(users[conf.bot.fd]);
	}
}

bool Server::bot_parse()
{
	int fd = conf.bot.fd;
	if (users[fd]->get_sendbuffer().find("\r") == std::string::npos)
		return false;

	std::istringstream iss(users[fd]->get_sendbuffer());
	std::string line;

	std::cout << ORANGE "BOT PARSING DATA: " << escape(iss.str()) << RESET << std::endl;

	while (std::getline(iss, line))
	{
		std::cout << MUSTARD << "BOT RECEIVED : `" << RESET << escape(line) << MUSTARD "`" RESET << std::endl;
		if (line.substr(line.length() - 1) == "\r")
		{
			bot_response(line);
			users[fd]->get_sendbuffer().erase(0, line.length() + 1);
		}
		else
			return false;
	}

	std::cout << ORANGE "AFTER PARSING BOT DATA: " << escape(users[fd]->get_sendbuffer()) << RESET << std::endl;

	return true;
}

std::string get_nickname_from_hostmask(std::string hostmask)
{
	std::vector<std::string> args = split(hostmask, '!');
	return args[0].substr(1);
}

void Server::bot_response(std::string message)
{
	std::vector<std::string> args = split(message, ' ');
	static std::string all_responses[] = {
		"It is certain", "It is decidedly so", "Without a doubt",
		"Yes definitely", "You may rely on it", "As I see it, yes",
		"Most likely", "Outlook good", "Yes", "Signs point to yes",
		"Reply hazy try again", "Ask again later", "Better not tell you now",
		"Cannot predict now", "Concentrate and ask again",
		"Don't count on it", "My reply is no", "My sources say no",
		"Outlook not so good", "Very doubtful"};

	for (size_t i = 0; i < args.size(); i++)
		std::cout << "args[" << i << "] = " << args[i] << std::endl;

	if (args[1] == "PRIVMSG")
	{
		std::srand(time(NULL));

		std::string received_message = join(args.begin() + 3, args.end(), " ");
		int random_idx = std::rand() % sizeof(all_responses) / sizeof(std::string);
		bool forced_response = received_message.find("fuck") != std::string::npos;
		std::string direction = get_nickname_from_hostmask(args[0]);
		std::string response = forced_response ? "you" : all_responses[random_idx];

		if (args[2] != conf.bot.nickname)
			direction = args[2];

		if (received_message.find(conf.bot.nickname) != std::string::npos || args[2] == conf.bot.nickname || forced_response)
			users[conf.bot.fd]->append_data("PRIVMSG " + direction + " :" + response + "\r\n");
	}
}

bool Server::map_string_comparator::operator()(const std::string &s1, const std::string &s2) const
{
	std::string s1_upper(s1);
	std::string s2_upper(s2);
	std::transform(s1_upper.begin(), s1_upper.end(), s1_upper.begin(), ::toupper);
	std::transform(s2_upper.begin(), s2_upper.end(), s2_upper.begin(), ::toupper);
	return (s1_upper < s2_upper);
}
