#pragma once
#include"http_utils.h"
#include"indexer.h"
#include"safe_queue.cpp"
#include<iostream>
#include<thread>
#include<vector>
#include<map>

class thread_pool
{
public:
	thread_pool(const std::string& connstr, std::map<std::string, Link>& mlink, std::condition_variable* mcv, std::mutex& m, const std::string& wordsdbName, const std::string& docsdbName, const std::string& indexdbName, const std::string& useragent, const std::string& accept);
	thread_pool(const thread_pool&) = delete;
	thread_pool(thread_pool&) = delete;
	thread_pool& operator= (thread_pool&) = delete;
	thread_pool& operator= (const thread_pool&) = delete;
	~thread_pool();
	
	void submit(std::pair<std::string, Link>& pt);
	void go();
	void stop();
	size_t get_queue_size();
private:
	void work(const std::string& connstr, std::map<std::string, Link>& maplink, std::mutex& m, const std::string& wordsdbName, const std::string& docsdbName, const std::string& indexdbName, const std::string& useragent, const std::string& accept);
	const std::string wordsdbName;
	const std::string docsdbName;
	const std::string indexdbName;
	std::vector<std::thread> vths;
	safe_queue<std::pair<std::string, Link>> safeq;
	std::map<std::string, Link>* mlink;
};