#pragma once

#include "IRCserver.hpp"

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

std::string join(const std::string arr[], size_t size, const std::string& separator);
#define JN(...) join((const std::string[]){__VA_ARGS__}, sizeof((const std::string[]){__VA_ARGS__})/sizeof(std::string), " ")

std::string escape(const std::string &str);

std::vector<std::string> split(const std::string &str, char delim, bool trim = false);

std::string c(int code);

template <typename T>
std::string to_string(T v)
{
	std::stringstream ss;
	ss << v;
	return ss.str();
}

std::string trimstr(const std::string &str);
