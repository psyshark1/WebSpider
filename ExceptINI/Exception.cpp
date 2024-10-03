#include"Exception.h"

std::string WrongINI::what()
{
	return message;
}

WrongINI::WrongINI(std::string&& msg) : message{std::move(msg)}
{
}

WrongINI::~WrongINI()
{
}