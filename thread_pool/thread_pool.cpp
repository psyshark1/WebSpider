#pragma once
#include "thread_pool.h"

thread_pool::thread_pool(const std::string& connstr, std::map<std::string, Link>& mlink, std::condition_variable* mcv, std::mutex& m, const std::string& wordsdbName, const std::string& docsdbName, const std::string& indexdbName) :
	wordsdbName(wordsdbName), docsdbName(docsdbName), indexdbName(indexdbName), safeq(mcv)
{
	if (!mlink.size())
	{ 
		throw std::runtime_error("No maplink");
	}
	const unsigned cores = std::thread::hardware_concurrency();
	for (unsigned i{ 0 }; i < cores; ++i)
	{
		vths.push_back(std::thread(&thread_pool::work, this, std::cref(connstr), std::ref(mlink), std::ref(m), std::ref(mcv), std::cref(this->wordsdbName), std::cref(this->docsdbName), std::cref(this->indexdbName)));//, idword, idref
	}
}

void thread_pool::work(const std::string& connstr, std::map<std::string, Link>& maplink, std::mutex& m, std::condition_variable* mcv, const std::string& wordsdbName, const std::string& docsdbName, const std::string& indexdbName)
{
	pqxx::connection* dbcon{ nullptr };
	Indexer* idx{ nullptr };
	http_request* httpreq{ nullptr };

	dbcon = new pqxx::connection(connstr);
	idx = new Indexer();
	httpreq = new http_request();

	this->safeq.pop(maplink, m, idx, dbcon, httpreq, wordsdbName, docsdbName, indexdbName);
	dbcon->close();
	delete dbcon; delete idx; delete httpreq;
	++emrg_exit_counter;
	(emrg_exit_counter == std::thread::hardware_concurrency() && !safeq.get_exit_status()) ? mcv->notify_one() : false;
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

const unsigned int& thread_pool::get_emrg_exit_counter()
{
	return emrg_exit_counter;
}

thread_pool::~thread_pool()
{
	for (auto& it : vths)
	{
		it.join();
	}
}