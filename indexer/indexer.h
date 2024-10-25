#pragma once
#include<regex>
#include <pqxx/pqxx>
#include <boost/locale.hpp>
#include <iostream>
#include"http_utils.h"
#include "lnk.h"

class Indexer
{
public:
	Indexer();
	void create_wordsbase() noexcept;
	void parse_refs(const Link& link) noexcept;
	std::map<std::string, Link> getLinks() noexcept;
	bool addToDB(pqxx::connection* conn, const std::string& wordsdbName, const std::string& docsdbName, const std::string& indexdbName);
	bool get_http(pqxx::connection* conn, const std::string& docsdbName, http_request* httpreq, const std::string& url, const Link& lnk);
	const unsigned short& get_DBerr_counter();
	const unsigned short& get_HTTPerr_counter();
	void clear() noexcept;
	Indexer& operator = (const Indexer& idx);
	bool operator == (const Indexer& idx) const;
	bool operator != (const Indexer& idx);
	~Indexer();

private:
	std::string url;
	std::string HtmlContent;
	std::map<std::string, Link> links;
	std::unordered_map<std::string, unsigned> words;
	unsigned short DBerr_counter{ 0 };
	unsigned short HTTPerr_counter{ 0 };
	const std::regex word_regex{ "<a href=\"(https?\:\/\/)?(\/?[^#@].[^:;]*?)\"" };
	void locale_init() noexcept;
};