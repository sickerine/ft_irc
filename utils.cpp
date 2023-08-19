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

std::string ltrim(const std::string &str)
{
	size_t start = str.find_first_not_of(" \t\n\t");
	return (start == std::string::npos) ? "" : str.substr(start);
}

std::string rtrim(const std::string &str)
{
	size_t end = str.find_last_not_of(" \t\n\t");
	return (end == std::string::npos) ? "" : str.substr(0, end + 1);
}

// Removed line end and any whitespace at the start and end of the string
std::string trimstr(const std::string &str)
{
	return rtrim(ltrim(str));
}

std::vector<std::string> split(const std::string &str, char delim, bool trim)
{
	std::vector<std::string> splits;
	std::istringstream iss(str);
	std::string line;

	if (iss.str().empty())
		return splits;

	while (std::getline(iss, line, delim))
	{
		if (line.empty())
			splits.back() += delim;
		else
			splits.push_back(line);
	}
	if (trim)
		splits.back().erase(splits.back().length() - 1);

	// Removing empty string at the end of the vector
	while (splits.back().empty())
		splits.pop_back();

	//for (size_t i = 0; i < splits.size(); i++)
	//{
	//	std::cout << "[" << i << "] " << splits[i] << " (" << splits[i].length() << "): ";
	//	for (size_t j = 0; j < splits[i].length(); j++)
	//		std::cout << " " << (int)splits[i][j];
	//	std::cout << std::endl;
	//}
	return splits;
}

std::string c(int code)
{
	std::stringstream ss;
	ss << std::setw(3) << std::setfill('0') << code;
	return ss.str();
}

std::string join(const std::string arr[], size_t size, const std::string& separator)
{
    std::string result;
    for (size_t i = 0; i < size; i++)
    {
        result += arr[i];
        if (i < size - 1)
            result += separator;
    }
    return result;
}
