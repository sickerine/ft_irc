#include "Channel.hpp"
#include "Server.hpp"
#include "User.hpp"

Channel::Channel() {}
Channel::Channel(const std::string &n, const std::string &k, const std::string &t) : name(n), key(k), topic(t)
{
}

int Channel::add_user(int fd, User *user, const std::string &key)
{
    if (this->key != key)
        return ERR_BADCHANNELKEY;
    users[fd] = user;
    return 0;
}

void Channel::remove_user(int fd)
{
    users.erase(fd);
}

const std::string &Channel::get_name() { return name; }
std::string Channel::get_users_count()
{
    std::stringstream ss;
    ss << users.size();
    return ss.str();
}

const std::string &Channel::get_topic() { return topic; }
void Channel::set_topic(const std::string &t) { topic = t; }

UserList &Channel::get_users() { return users; }

std::string Channel::get_users_list()
{
    std::string list;
    for (UserList::iterator it = users.begin(); it != users.end(); it++)
    {
        list += it->second->get_prefixed_nick(operators) + " ";
    }
    return list;
}

std::vector<std::string> Channel::get_who_list()
{
    std::vector<std::string> list;
    for (UserList::iterator it = users.begin(); it != users.end(); it++)
    {
        list.push_back(it->second->who_this(*this));
    }
    return list;
}

bool Channel::has_user(User *user)
{
    for (UserList::iterator it = users.begin(); it != users.end(); it++)
    {
        if (it->second == user)
            return true;
    }
    return false;
}

bool Channel::has_user(int fd)
{
    return users.find(fd) != users.end();
}

bool Channel::is_operator(User *user)
{
    for (UserList::iterator it = operators.begin(); it != operators.end(); it++)
    {
        if (it->second == user)
            return true;
    }
    return false;
}

bool Channel::is_operator(int fd)
{
    return operators.find(fd) != operators.end();
}

int Channel::add_operator(User *user)
{
    operators[user->get_fd()] = user;
    return 0;
}

int Channel::remove_operator(User *user)
{
    operators.erase(user->get_fd());
    return 0;
}