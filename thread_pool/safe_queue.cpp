#pragma once
#include<mutex>
#include<queue>
#include<condition_variable>
#include"indexer.h"

template <class T>
class safe_queue
{
public:
	safe_queue(std::condition_variable* mcv) : m_cv(mcv) {};
	safe_queue(const safe_queue&) = delete;
	safe_queue(safe_queue&) = delete;
	safe_queue& operator= (safe_queue&) = delete;
	safe_queue& operator= (const safe_queue&) = delete;
	~safe_queue(){};

	void push(T&& tc)
	{
		//std::lock_guard lg{ m };
		sq.push(std::move(tc));
	};
	void pop(std::map<std::string, Link>& maplink, std::mutex& m, Indexer* idx, pqxx::connection* conn, http_request* httpreq, const std::string& wordsdbName, const std::string& docsdbName, const std::string& indexdbName, const std::string& useragent, const std::string& accept)
	{
		std::map<std::string, Link> mlinks;
		std::unique_lock ul{ m };
		while (true)
		{
			(!ul.owns_lock()) ? ul.lock() : false;
			cv_dat.wait(ul, [&]() {return !sq.empty() || exit; });
			if (!sq.empty())
			{
				T entity = std::move(sq.front());
				sq.pop();
				
				if (idx->get_http(conn, docsdbName, httpreq, entity.first, entity.second))
				{
					ul.unlock();
					idx->parse_refs(useragent, accept);
					mlinks = idx->getLinks();
					idx->create_wordsbase();
					ul.lock();
					if (idx->addToDB(conn, wordsdbName, docsdbName, indexdbName))
					{
						if (!mlinks.empty())
						{
							for (const auto& [url, link] : mlinks)
							{
								maplink.emplace(url, link);
							}
							mlinks.clear();
						}
					}
				}
				ul.unlock();
				idx->clear();
			}
			(ul.owns_lock()) ? ul.unlock() : false;
			m_cv->notify_one();
			if (exit) { break; }
		}
	};

	inline void notify() noexcept
	{
		cv_dat.notify_all();
	}

	inline void stop() noexcept
	{
		exit = true;
		notify();
	}

	size_t get_size() noexcept
	{
		return sq.size();
	}
private:
	std::queue<T> sq;
	std::condition_variable cv_dat;
	std::condition_variable* m_cv;
	bool exit{ false };
};