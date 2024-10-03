#pragma once
#include<stdexcept>
const static class WrongINI
{
public:
	WrongINI(std::string&& msg);
	~WrongINI();
	std::string what();
private:
	const std::string message;
};