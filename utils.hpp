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

template <typename T>
std::string to_string(T v)
{
	std::stringstream ss;
	ss << v;
	return ss.str();
}

template <typename T>
T to_number(const std::string &str)
{
	std::istringstream iss(str);
	T ret;
	iss >> ret;
	if (iss.fail() || !iss.eof())
		throw std::runtime_error("Invalid number");
	return ret;
}

template <typename T>
T to_number_safe(const std::string &str)
{
	try {
		return to_number<T>(str);
	} catch (std::exception &e) {
		return 0;
	}
}

std::string c(int code);
std::string escape(const std::string &str);
std::string trimstr(const std::string &str);

std::string join(const std::string arr[], size_t size, const std::string& separator);
#define JN(...) join((const std::string[]){__VA_ARGS__}, sizeof((const std::string[]){__VA_ARGS__})/sizeof(std::string), " ")

std::vector<std::string> split(const std::string &str, char delim, bool trim = false);
