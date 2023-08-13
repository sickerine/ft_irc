#include "utils.hpp"

std::string escape(const std::string &str)
{
	std::string escaped;
	for (size_t i = 0; i < str.length(); i++)
	{
		if (str[i] == '\n')
			escaped += "\\n";
		else if (str[i] == '\r')
			escaped += "\\r";
		else
			escaped += str[i];
	}
	return escaped;
}

std::vector<std::string> split(const std::string &str, char delim, bool trim)
{
	std::vector<std::string> splits;
	std::istringstream iss(str);
	std::string line;

	while (std::getline(iss, line, delim))
	{
		if (line.empty())
			splits.back() += delim;
		else
			splits.push_back(line);
	}
	if (trim)
		splits.back().erase(splits.back().length() - 1);

	for (size_t i = 0; i < splits.size(); i++)
	{
		std::cout << "[" << i << "] " << splits[i] << std::endl;
	}
	return splits;
}

std::string c(int code)
{
	std::stringstream ss;
	ss << std::setw(3) << std::setfill('0') << code;
	return ss.str();
}