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

#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"


#pragma region TODO

    #define ERR_ALREADYREGISTRED

#pragma endregion

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
    ERR_NEEDMOREPARAMS = 461,
    ERR_BADCHANNELKEY = 475,
    ERR_NOSUCHCHANNEL = 403,
    RPL_TOPIC = 332,
    RPL_NAMREPLY = 353,
    RPL_ENDOFNAMES = 366,
    RPL_WHOREPLY = 352,
    RPL_ENDOFWHO = 315,
};

enum {
    OPERATOR,    
    REGULAR
};