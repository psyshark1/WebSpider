#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include"Exception.h"

#ifdef PARSER_DLL
#define INI __declspec(dllexport)
#else
#define INI __declspec(dllimport)
#endif

class ini_parser
{
public:
	INI ini_parser(const std::string&& fpath);
	INI ini_parser(const std::string& fpath);
	INI ini_parser(const ini_parser& ip);
	INI ini_parser(const ini_parser&& rip) noexcept;
	ini_parser& operator = (const ini_parser& ip);
	ini_parser& operator = (const ini_parser&& rip) noexcept;
	template<typename T>
	T get_value(std::string&& find);
	INI ~ini_parser();
	std::unordered_map<std::string, std::list<std::pair<std::string, std::string>>> data;

private:
	bool check_file(std::string& sline, const int& line);
	bool check_find(const std::string& find);
	std::string var_hint(const std::string& section);
	bool check_mod(const std::string& section, const std::string& key, const std::string& val);
	void read_file(const std::string& fpath);
};