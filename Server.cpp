#include "Server.hpp"
#include "Channel.hpp"
#include "User.hpp"

Server::Server(const std::string &port, const std::string &pass) : password(pass), name("globalhost"), info(NULL), running(true), host("10.11.7.6"), operator_username("operator"), operator_password("password")
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

    pfds.push_back(make_pfd(server_fd, POLLIN, 0));

    create_channel("#global", "", "safe");
    create_channel("#hentai", "abc", "nsfw");
}
Server::~Server()
{
    if (info)
        freeaddrinfo(info);
    // for (UserList::iterator it = users.begin(); it != users.end(); ++it)
    // 	delete it->second;
}

void Server::create_channel(const std::string &name, const std::string &key, const std::string &topic)
{
    channels[name] = Channel(name, key, topic);
}

void Server::run()
{
    while (running)
    {
        insist(poll(&pfds[0], pfds.size(), -1), -1, "poll failed");
        for (size_t i = 0; i < pfds.size(); i++)
            process_events(pfds[i].fd);
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
        pfds.push_back(make_pfd(new_fd, POLLIN, 0));
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
        if (length == -1)
            break;

        buffer[length] = 0;
        users[fd]->append_data(std::string(buffer));
    }
}

void Server::welcome(int fd)
{
    send_message(fd, ":" + name + " " + c(RPL_WELCOME) + " " + users[fd]->get_nick() + " :kys " + users[fd]->get_nick() + "!" + users[fd]->get_user() + "@" + name);
    send_message(fd, ":" + name + " " + c(RPL_STARTOFMOTD) + " " + users[fd]->get_user() + " :- " + name + " Message of the Day -");
    send_message(fd, ":" + name + " " + c(RPL_MOTD) + " " + users[fd]->get_nick() + " :- LOLOLOLOLOLOLOLOLOLOOLOLOLOLOLOLOLOLOLOLOLOLOLOLOLOLOLOOLOLOLOLOLOLOLOLOLOLOLOLOLOLOLOLOLOOLOLOLOLOLOLOLO");
    send_message(fd, ":" + name + " " + c(RPL_ENDOFMOTD) + " " + users[fd]->get_user() + " :End of /MOTD command.");
}

void Server::parse_command(int fd, const std::string &cmd)
{
    User *user = users[fd];
    std::vector<std::string> args = split(cmd, ' ', true);

    if (args.size() < 1)
        return;

    if (!user->get_auth())
    {
        if (args[0] == "PASS")
        {
            std::cout << "pass: `" << args[1] << "`" << std::endl;
            if (args[1] == password)
            {
                std::cout << "yes" << std::endl;
                user->set_auth(true);
            }
            else
                std::cout << "no" << std::endl;
        }
        else if (args[0] != "CAP" && args[0] != "PROTOCTL")
        {
            terminate_connection(fd);
            throw std::runtime_error("connection terminated");
        }
    }
    else if (args[0] == "USER")
    {
        CHECK_ARGS(5);
        user->set_user(args[1]);
        user->set_real(join(args.begin() + 4, args.end(), " "));
        if (user->get_registered())
            welcome(fd);
    }
    else if (args[0] == "NICK")
    {
        if (user->get_nick() == args[1])
            return;

        bool taken = find_user_by_nickname(args[1]) != NULL;
        if (taken)
        {
            send_message(fd, ":" + name + " " + c(ERR_NICKNAMEINUSE) + " " + args[1] + " :" + args[1]);
            return;
        }
        user->set_nick(args[1]);
        if (user->get_registered())
            send_message(fd, ":" + name + " NICK " + args[1]);
        user->set_registered(true);
        if (user->get_user() != "")
            welcome(fd);
    }
    else if (args[0] == "LIST")
    {
        send_message(fd, ":" + name + " " + c(RPL_LISTSTART) + " " + user->get_nick() + " Channel :Users Name");
        for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); it++)
            send_message(fd, ":" + name + " " + c(RPL_LIST) + " " + user->get_nick() + " " + it->second.get_name() + " " + it->second.get_users_count() + " :" + it->second.get_topic());
        send_message(fd, ":" + name + " " + c(RPL_LISTEND) + " " + user->get_nick() + " :End of /LIST");
    }
    else if (args[0] == "QUIT")
    {
        terminate_connection(fd);
        throw std::runtime_error("connection terminated");
    }
    else if (args[0] == "JOIN")
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
            if (channels.find(params[i]) != channels.end())
            {
                Channel &channel = channels[params[i]];
                if (channel.is_invited(user))
                {
                    int ret = channel.add_user(fd, user, channel_key);
                    if (ret == ERR_BADCHANNELKEY)
                        send_message(fd, ":" + name + " " + c(ERR_BADCHANNELKEY) + " " + user->get_nick() + " " + params[i] + " :Cannot join channel (+k)");
                    else
                    {
                        channel.remove_invite(user);
                        broadcast_message(channel, ":" + user->get_hostmask(user->get_nick()) + " JOIN :" + params[i]);
                        send_message(fd, ":" + name + " " + c(RPL_TOPIC) + " " + user->get_nick() + " " + params[i] + " :" + channel.get_topic());
                        send_message(fd, ":" + name + " " + c(RPL_NAMREPLY) + " " + user->get_nick() + " = " + params[i] + " :" + channel.get_users_list());
                        send_message(fd, ":" + name + " " + c(RPL_ENDOFNAMES) + " " + user->get_nick() + " " + params[i] + " :End of /NAMES list");
                    }
                }
                else
                    send_message(fd, ":" + name + " " + c(ERR_INVITEONLYCHAN) + " " + user->get_nick() + " " + params[i] + " :Cannot join channel (+i)");
            }
            else
                no_such_channel(fd, params[i]);
        }
    }
    else if (args[0] == "WHO")
    {
        CHECK_ARGS(2);

        if (channels.find(args[1]) == channels.end())
        {
            no_such_channel(fd, args[1]);
            return;
        }

        std::vector<std::string> who_list = channels[args[1]].get_who_list();

        for (size_t i = 0; i < who_list.size(); i++)
            send_message(fd, ":" + name + " " + c(RPL_WHOREPLY) + " " + user->get_nick() + " " + args[1] + " " + who_list[i]);
        send_message(fd, ":" + name + " " + c(RPL_ENDOFWHO) + " " + user->get_nick() + " " + args[1] + " :End of /WHO list");
    }
    else if (args[0] == "PRIVMSG")
    {
        if (args.size() < 3)
        {
            need_more_params(fd, args[0]);
            return;
        }

        std::string message = join(args.begin() + 2, args.end(), " ");

        if (args[1][0] == '#')
        {
            if (channels.find(args[1]) == channels.end())
            {
                no_such_channel(fd, args[1]);
                return;
            }

            Channel &channel = channels[args[1]];
            if (!channel.has_user(fd))
            {
                not_on_channel(fd, args[1]);
                return;
            }

            broadcast_message(channel, ":" + user->get_hostmask(user->get_prefixed_nick(channel.get_users())) + " PRIVMSG " + args[1] + " " + message, users[fd]);
        }
        else
        {
            User *target = find_user_by_nickname(args[1]);
            if (target == NULL)
            {
                no_such_nick(fd, args[1]);
                return;
            }

            send_message(target->get_fd(), ":" + user->get_hostmask(user->get_prefixed_nick()) + " PRIVMSG " + target->get_nick() + " " + message);
        }
    }
    else if (args[0] == "ISON")
    {
        if (args.size() < 2)
        {
            need_more_params(fd, args[0]);
            return;
        }

        if (args[1][0] == '#')
        {
            if (channels.find(args[1]) == channels.end())
            {
                send_message(fd, ":" + name + " " + c(RPL_NOWOFF) + " " + user->get_nick() + " " + args[1] + " :" + args[1] + " * * 0 is offline");
                return;
            }
        }
        else
        {
            User *user = find_user_by_nickname(args[1]);
            if (user == NULL)
            {
                send_message(fd, ":" + name + " " + c(RPL_ISON) + " " + user->get_nick() + " :" + args[1]);
                return;
            }
        }
        send_message(fd, ":" + name + " " + c(RPL_ISON) + " " + user->get_nick() + " :");
    }
    else if (args[0] == "PART")
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
    else if (args[0] == "PING")
    {
        std::string message = join(args.begin() + 1, args.end(), " ");

        send_message(fd, ":" + name + " PONG " + name + " :" + message);
    }
    else if (args[0] == "OPER")
    {
        CHECK_ARGS(3);

        bool authenticated = args[1] == operator_username && args[2] == operator_password;
        if (authenticated)
        {
            operators[fd] = user;
            user->set_server_operator(true);
            send_message(fd, ":" + name + " " + c(RPL_YOUREOPER) + " " + user->get_nick() + " :You are now an IRC operator");
            server_broadcast_message(":" + name + " MODE " + user->get_nick() + " :+o");
        }
        else
        {
            send_message(fd, ":" + name + " " + c(ERR_PASSWDMISMATCH) + " " + user->get_nick() + " :Password incorrect");
        }
    }
    else if (args[0] == "KICK")
    {
        // if (args.size() < 3)
        // {	
        // 	need_more_params(fd, args[0]);
        // 	return;
        // }

        // if (channels.find(args[1]) == channels.end())
        // {
        // 	no_such_channel(fd, args[1]);
        // 	return;
        // }

        // Channel &channel = channels[args[1]];

        // if (!channel.has_user(fd))
        // {
        // 	not_on_channel(fd, args[1]);
        // 	return;
        // }

        // if (!channel.has_user(args[2]))
        // {
        // 	no_such_nick(fd, args[2]);
        // 	return;
        // }

        // if (!channel.is_operator(fd))
        // {
        // 	send_message(fd, ":" + name + " " + c(ERR_CHANOPRIVSNEEDED) + " " + user->get_nick() + " " + args[1] + " :You're not channel operator");
        // 	return;
        // }
    }
    else if (args[0] == "INVITE")
    {
        // >> :mcharrad!mcharrad@197.230.24.20 INVITE mcharrad_ :#uwu

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

        if (channel.is_operator(fd) || is_operator(fd))
        {
            if (channel.has_user(target->get_fd()))
            {
                send_message(fd, ":" + name + " " + c(ERR_USERONCHANNEL) + " " + user->get_nick() + " " + args[1] + " " + args[2] + " :is already on channel");
                return;
            }
            send_message(target->get_fd(), ":" + user->get_hostmask(user->get_nick()) + " INVITE " + target->get_nick() + " " + args[2]);
            send_message(fd, ":" + name + " " + c(RPL_INVITING) + " " + user->get_nick() + " " + args[1] + " " + args[2]);
            channel.invite(target);
        }
        else
            channel_operator_privileges_needed(fd, channel.get_name());

    }
    else if (args[0] == "MODE")
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
            send_message(fd, ":" + name + " " + c(RPL_CHANNELMODEIS) + " " + user->get_nick() + " " + args[1] + " +nt");
            return ;
        }

        char operation = args[2][0];

        if (operation != '+' && operation != '-')
        {
            send_message(fd, ":" + name + " " + c(ERR_UNKNOWNMODE) + " " + user->get_nick() + " " + args[2] + " :retard");
            return;
        }

        int mode = 0;
        bool err = false;
        for (size_t i = 1; i < args[2].length(); i++)
        {
            switch (args[2][i]) {
            SET_MODE_OR_ERR('i', MODE_INVITEONLY);
            SET_MODE_OR_ERR('t', MODE_TOPIC);
            
            SET_MODE_OR_ERR('o', MODE_OPERATOR);
            SET_MODE_OR_ERR('k', MODE_KEY);
            SET_MODE_OR_ERR('l', MODE_LIMIT);
            default: err = true; break; }
            if (err) {
                send_message(fd, ":" + name + " " + c(ERR_UNKNOWNMODE) + " " + user->get_nick() + " " + args[2][i] + " :retard");
                return;
            }
        }

        if (mode & MODE_INVITEONLY)
        {
            if (is_operator(fd) || channel.is_operator(fd))
            {
                if (operation == '+')
                    channel.set_mode(channel.get_mode() | MODE_INVITEONLY);
                else
                    channel.set_mode(channel.get_mode() & ~MODE_INVITEONLY);
                broadcast_message(channel, ":" + user->get_hostmask(user->get_nick()) + " MODE " + args[1] + " :" + operation + "i");
            }
            else
                channel_operator_privileges_needed(fd, channel.get_name());
        }
        if (mode & MODE_OPERATOR)
        {
            CHECK_ARGS(4);
            if (is_operator(fd) || channel.is_operator(fd))
            {
                User *target = find_user_by_nickname(args[3]);
                if (target)
                {
                    if (channel.has_user(target))
                    {
                        if (operation == '+')
                            channel.add_operator(target);
                        else if (operation == '-')
                            channel.remove_operator(target);
                        broadcast_message(channel, ":" + name + " " + c(RPL_CHANNELMODEIS) + " " + user->get_nick() + " " + args[1] + " " + operation + "o " + target->get_nick());
                    }
                    else
                        user_not_in_channel(fd, args[3], channel.get_name());
                }
                else
                    no_such_nick(fd, args[3]);
            }
            else
                channel_operator_privileges_needed(fd, channel.get_name());
        }
    }
}

void Server::parse_data(int fd)
{
    std::istringstream iss(users[fd]->get_data());
    std::string line;

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

void Server::process_events(int fd)
{
    if (fd == server_fd)
        accept_connections();
    else
    {
        receive_data(fd);
        parse_data(fd);
    }
}

void Server::send_message(int fd, const std::string &message)
{
    std::cout << GREEN << "Sending to " << RESET << fd << GREEN ": `" RESET << escape(message) << GREEN "`" RESET << std::endl;
    std::string ircmsg(message + "\r\n");
    send(fd, ircmsg.c_str(), ircmsg.length(), 0);
}

void Server::broadcast_message(Channel & channel, const std::string &message, User *except)
{
    std::cout << BLUE << "Broadcasting to " << RESET << channel.get_name() << BLUE ": `" RESET << escape(message) << BLUE "`" RESET << std::endl;
    std::string ircmsg(message + "\r\n");
    for (UserList::iterator it = channel.get_users().begin(); it != channel.get_users().end(); ++it)
    {
        if (it->second != except)
            send(it->first, ircmsg.c_str(), ircmsg.length(), 0);
    }
}

void Server::server_broadcast_message(const std::string &message, User *except)
{
    std::cout << PURPLE << "Broadcasting to " << RESET << "Server" << PURPLE ": `" RESET << escape(message) << PURPLE "`" RESET << std::endl;
    std::string ircmsg(message + "\r\n");
    for (UserList::iterator it = users.begin(); it != users.end(); ++it)
    {
        if (it->second != except && it->second->get_registered())
            send(it->first, ircmsg.c_str(), ircmsg.length(), 0);
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
    close(fd);
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
    send_message(fd, ":" + name + " " + c(ERR_NEEDMOREPARAMS) + " " + users[fd]->get_nick() + " " + command + " :Not enough parameters");
}

void Server::no_such_channel(int fd, const std::string &channel)
{
    send_message(fd, ":" + name + " " + c(ERR_NOSUCHCHANNEL) + " " + users[fd]->get_nick() + " " + channel + " :No such channel");
}

void Server::not_on_channel(int fd, const std::string &channel)
{
    send_message(fd, ":" + name + " " + c(ERR_NOTONCHANNEL) + " " + users[fd]->get_nick() + " " + channel + " :You're not on that channel");
}

void Server::no_such_nick(int fd, const std::string &nickname)
{
    send_message(fd, ":" + name + " " + c(ERR_NOSUCHNICK) + " " + users[fd]->get_nick() + " " + nickname + " :No such nick/channel");
}

void Server::user_not_in_channel(int fd, const std::string &nickname, const std::string &channel)
{
    send_message(fd, ":" + name + " " + c(ERR_USERNOTINCHANNEL) + " " + users[fd]->get_nick() + " " + nickname + " " + channel + " :They aren't on that channel");
}

void Server::channel_operator_privileges_needed(int fd, const std::string &channel)
{
    send_message(fd, ":" + name + " " + c(ERR_CHANOPRIVSNEEDED) + " " + users[fd]->get_nick() + " " + channel + " :You're not channel operator");
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

int Server::remove_operator(User *user)
{
    operators.erase(user->get_fd());
    return 0;
}