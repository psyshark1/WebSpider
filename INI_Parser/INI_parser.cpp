#include "INI_parser.h"

ini_parser::ini_parser(const std::string&& fpath)
{
	read_file(fpath);
}
ini_parser::ini_parser(const std::string& fpath)
{
	read_file(fpath);
}

ini_parser::ini_parser(const ini_parser& ip)
{
	this->data = ip.data;
}

ini_parser::ini_parser(const ini_parser&& rip) noexcept
{
	this->data = rip.data;
}

ini_parser& ini_parser::operator = (const ini_parser& ip)
{
	if (this != &ip)
	{
		if (this->data.size()){ this->data.clear(); }
		this->data = ip.data;
		return *this;
	}
	return *this;
}

ini_parser& ini_parser::operator = (const ini_parser&& rip) noexcept
{
	if (this != &rip)
	{
		if (this->data.size()) { this->data.clear(); }
		this->data = rip.data;
		/*auto* p = &rip;
		p = nullptr;*/
		return *this;
	}
	return *this;
}

void ini_parser::read_file(const std::string& fpath)
{
	std::ifstream file(fpath);
	std::string sline;
	int line{ 0 };
	bool equal{ false };
	std::string section;
	std::string key;
	std::string val;

	if (!file.is_open())
	{
		throw std::runtime_error("Unable to open INI file!");
	}

	while (std::getline(file, sline))
	{
		++line;
		if (check_file(sline, line))
		{
			if (sline[0] == '[')
			{
				if (!section.empty() && !data.count(section)){ data[section];}
				section.clear();
				auto it = sline.begin() + 1;
				while (*it != ']')
				{
					section.push_back(std::move(*it));
					++it;
				}
				continue;
			}
			for (char& it : sline)
			{
				if (it != '=')
				{
					if (!equal)
					{
						key.push_back(std::move(it));
					}
					else
					{
						if (it == ' ')
						{
							char* next = &it + 1;
							if (next != sline.end()._Unwrapped())
							{
								val.push_back(std::move(it));
								continue;
							}
							break;
						}
						val.push_back(std::move(it));
					}
				}
				else
				{
					equal = true;
				}
			}
			equal = false;
			if (!check_mod(section,key,val)) { data[section].push_back({ key, val }); }
			key.clear(); val.clear();
		}
	}
	file.close();
}

bool ini_parser::check_file(std::string& sline, const int& line)
{
	if (sline[0] == ';' || sline[0] == ' ' || sline == "") { return false; }
	if (sline[0] == '=') { throw WrongINI("Incorrect file structure in line " + std::to_string(line)); }
	
	if (sline[0] != '[')
	{
		for (char& it : sline)
		{
			if (it == ';') { break; }
			if (it == '=')
			{
				char* next = &it + 1;
				if (*next == ';' || *next == ' ') { break; }
				return true;
			}
		}
		throw WrongINI("Incorrect file structure in line " + std::to_string(line));
	}
	else
	{
		if (sline[1] == ']') { throw WrongINI("Incorrect file structure in line " + std::to_string(line)); }
		if (*sline.rbegin() == ']') { return true; }
		for (const char& it : sline)
		{
			if (it == ']') { return true; }
		}
		throw WrongINI("Incorrect file structure in line " + std::to_string(line));
	}
	return true;
}

bool ini_parser::check_mod(const std::string& section, const std::string& key, const std::string& val)
{
	if (!data.empty())
	{
		for (auto& it : data[section])
		{
			if (it.first == key) { it.second = val; return true; }
		}
	}
	return false;
}

bool ini_parser::check_find(const std::string& find)
{
	if (*find.begin() == '.' || *find.rbegin() == '.') { return false; }
	for (const char& it : find){if (it == '.') { return true; }}
	return false;
}

std::string ini_parser::var_hint(const std::string& section)
{
	std::cout << "Maybe you meant " << std::endl;
	for (const auto& it : data[section])
	{
		std::cout << it.first << ' ';
	}
	
	std::string input;

	while (true)
	{
		std::cout << "Enter available variables: ";
		std::cin >> input;
		for (auto& it : data[section])
		{
			if (it.first == input) { return input; }
		}
	}
}
template<typename T>
T ini_parser::get_value(std::string&& find)
{
	std::string section;
	std::string key;
	bool dot{ false };
	if (!check_find(find)) { throw WrongINI("Incorrect search query!"); }

	for (char& it : find)
	{
		if (it != '.')
		{
			(!dot) ? section.push_back(std::move(it)) : key.push_back(std::move(it));
		}
		else
		{
			dot = true;
		}
	}

	if (!data.count(section)) { throw WrongINI("Section not found!"); }
	if (!data[section].size()) { throw WrongINI("Section missing data!"); }

	if (
		![&key, &section, this]() -> bool {
		for (const auto& it : data[section])
		{
			if (it.first == key) { return true; }
		}
		return false;
		}())
	{
		key = std::move(var_hint(section));
	}

	dot = false;
	for (const auto& it : data[section])
	{
		if (it.first == key)
		{ 
			if (!it.second.size()) { throw WrongINI("Variable " + key + " has no data!"); }
			if (*it.second.begin() != '-' && (*it.second.begin() < 48 || *it.second.begin() > 57))
			{ 
				throw WrongINI("Incorrect digital data type contains in section " + section + ", variable " + key + ": " + it.second);
			}

			auto it1 = it.second.begin() + 1;
			while (it1 != it.second.end())
			{
				if (*it1 == '.' && !dot)
				{
					dot = true; ++it1; continue;
				}
				else if (*it1 == '.' && dot)
				{
					throw WrongINI("Incorrect digital data type contains in section " + section + ", variable " + key + ": " + it.second);
				}
				if ((*it1 != '.') && (*it1 < 48 || *it1 > 57))
				{
					throw WrongINI("Incorrect digital data type contains in section " + section + ", variable " + key + ": " + it.second);
				}
				++it1;
			}

			if (std::is_same<T, int>::value)
			{
				return static_cast<T>(std::stoi(it.second));
			}
			else if (std::is_same<T, float>::value)
			{
				return static_cast<T>(std::stof(it.second));
			}
			else if (std::is_same<T, double>::value)
			{
				return static_cast<T>(std::stod(it.second));
			}
			else if (std::is_same<T, long double>::value)
			{
				return static_cast<T>(std::stold(it.second));
			}
			else if (std::is_same<T, long>::value)
			{
				return static_cast<T>(std::stol(it.second));
			}
			else if (std::is_same<T, long long>::value)
			{
				return static_cast<T>(std::stoll(it.second));
			}
			throw WrongINI("Incorrect data type calling");
		}
	}
	throw WrongINI("Section missing data!");
}

template<>
INI std::string ini_parser::get_value(std::string&& find)
{
	std::string section;
	std::string key;
	bool dot{ false };
	if (!check_find(find)) { throw WrongINI("Incorrect search query!"); }

	for (char& it : find)
	{
		if (it != '.')
		{
			(!dot) ? section.push_back(std::move(it)) : key.push_back(std::move(it));
		}
		else
		{
			dot = true;
		}
	}

	if (!data.count(section)) { throw WrongINI("Section not found!"); }
	if (!data[section].size()) { throw WrongINI("Section missing data!"); }

	if (![&key, &section, this]() -> bool {
		for (const auto& it : data[section])
		{
			if (it.first == key) { return true; }
		}
		return false;
		}()) {
		key = var_hint(section);
	}

	for (const auto& it : data[section])
	{
		if (it.first == key) { return it.second; }
	}
	throw WrongINI("Section missing data!");
}

template INI int ini_parser::get_value(std::string&& find);
template INI float ini_parser::get_value(std::string&& find);
template INI double ini_parser::get_value(std::string&& find);
template INI long ini_parser::get_value(std::string&& find);
template INI long long ini_parser::get_value(std::string&& find);
template INI long double ini_parser::get_value(std::string&& find);

ini_parser::~ini_parser()
{
}