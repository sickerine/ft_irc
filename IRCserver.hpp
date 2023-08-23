#pragma once

#include <algorithm>
#include <arpa/inet.h>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <map>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <ctime>
#include <fstream>

#include "utils.hpp"

#define RED "\033[31m"
#define ORANGE "\033[38;5;208m"
#define MUSTARD "\033[38;5;214m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"
#define BLUE "\033[34m"
#define PURPLE "\033[35m"
#define CYAN "\033[36m"
#define MAGENTA "\033[35m"
#define GREY "\033[38;5;244m"

#define SET_MODE_OR_ERR(t, x) \
	case t: if (mode & x)	{ err = true; break; } mode |= x; break;

#define CHECK_ARGS(x) \
	if (args.size() < x) \
	{ \
		need_more_params(fd, args[0]); \
		return; \
	}

#define CHECK_CHANNEL(x) \
	if (channels.find(x) == channels.end()) \
	{ \
		no_such_channel(fd, x); \
		return; \
	} \
	Channel &channel = channels[x];

#define OPER_START() \
	if (user->is_server_operator() || channel.is_operator(user)) \
	{ 

#define OPER_END() \
	} \
	else \
		channel_operator_privileges_needed(fd, channel.get_name()); 

#define REQUIRE_CONF(x) insist((conf.x = configs[#x]).empty(), true, #x" is empty"); 
#define REQUIRE_PCONF(prefix, x) insist((conf.x = configs[#prefix"_"#x]).empty(), true, #x" is empty"); 

#define OPTIONAL_CONF(x) (configs.find(#x) != configs.end() ? configs[#x] : "")
#define OPTIONAL_PCONF(prefix, x) (configs.find(#x) != configs.end() ? configs[#prefix"_"#x] : "")

#define REQUIRE_CONF_NUMBER(x, type) try {conf.x = to_number<type>(configs[#x]); } catch (std::exception &e) { throw std::runtime_error(std::string(#x) + " is not a number"); }
#define REQUIRE_PCONF_NUMBER(prefix, x, type) try {conf.x = to_number<type>(configs[#prefix"_"#x]); } catch (std::exception &e) { throw std::runtime_error(std::string(#x) + " is not a number"); }



class User;

typedef std::map<int, User *> UserList;

enum
{
	RPL_WELCOME = 1,
	RPL_YOURHOST = 2,
	RPL_CREATED = 3,
	RPL_MYINFO = 4,
	RPL_ISUPPORT = 5,
	RPL_ISON = 303,
	RPL_ENDOFWHO = 315,
	RPL_LISTSTART = 321,
	RPL_LIST = 322,
	RPL_LISTEND = 323,
	RPL_CHANNELMODEIS = 324,
	RPL_TOPIC = 332,
	RPL_INVITING = 341,
	RPL_WHOREPLY = 352,
	RPL_NAMREPLY = 353,
	RPL_ENDOFNAMES = 366,
	RPL_MOTD = 372,
	RPL_STARTOFMOTD = 375,
	RPL_ENDOFMOTD = 376,
	RPL_YOUREOPER = 381,
	ERR_NOSUCHNICK = 401,
	ERR_NOSUCHCHANNEL = 403,
	ERR_ERRONEUSNICKNAME = 432,
	ERR_NICKNAMEINUSE = 433,
	ERR_USERNOTINCHANNEL = 441,
	ERR_NOTONCHANNEL = 442,
	ERR_USERONCHANNEL = 443,
	ERR_NEEDMOREPARAMS = 461,
	ERR_ALREADYREGISTRED = 462,
	ERR_PASSWDMISMATCH = 464,
	ERR_CHANNELISFULL = 471,
	ERR_UNKNOWNMODE = 472,
	ERR_INVITEONLYCHAN = 473,
	ERR_BADCHANNELKEY = 475,
	ERR_CHANOPRIVSNEEDED = 482,
	RPL_NOWOFF = 605,
};

enum {
	OPERATOR,    
	REGULAR
};
