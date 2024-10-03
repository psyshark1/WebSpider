#pragma once
#include "thread_pool.h"

thread_pool::thread_pool(const std::string& connstr, std::map<std::string, Link>& mlink, std::condition_variable* mcv, std::mutex& m, const std::string& wordsdbName, const std::string& docsdbName, const std::string& indexdbName, const std::string& useragent, const std::string& accept) :
	wordsdbName(wordsdbName), docsdbName(docsdbName), indexdbName(indexdbName), safeq(mcv)
{
	if (!mlink.size())
	{ 
		throw std::exception("No maplink");
	}
	const unsigned cores = std::thread::hardware_concurrency();
	for (unsigned i{ 0 }; i < cores; ++i)
	{
		vths.push_back(std::thread(&thread_pool::work, this, std::cref(connstr), std::ref(mlink), std::ref(m), std::cref(this->wordsdbName), std::cref(this->docsdbName), std::cref(this->indexdbName), std::cref(useragent), std::cref(accept)));//, idword, idref
	}
}

void thread_pool::work(const std::string& connstr, std::map<std::string, Link>& maplink, std::mutex& m, const std::string& wordsdbName, const std::string& docsdbName, const std::string& indexdbName, const std::string& useragent, const std::string& accept)
{
	pqxx::connection* dbcon = new pqxx::connection(connstr);
	Indexer* idx{ new Indexer() };
	http_request* httpreq {new http_request()};
	//udx->parse_refs(mlink.accept, mlink.useragent);
	//udx->create_wordsbase();

	this->safeq.pop(maplink, m, idx, dbcon, httpreq, wordsdbName, docsdbName, indexdbName, useragent, accept);
	dbcon->close();
}

void thread_pool::submit(std::pair<std::string, Link>& pt)
{
	safeq.push(std::move(pt));
}

void thread_pool::go()
{
	this->safeq.notify();
}

void thread_pool::stop()
{
	this->safeq.stop();
}

size_t thread_pool::get_queue_size()
{
	return safeq.get_size();
}

thread_pool::~thread_pool()
{
	for (auto& it : vths)
	{
		it.join();
	}
}