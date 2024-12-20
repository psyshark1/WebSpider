#pragma once
#include <mutex>
#include <queue>
#include <condition_variable>
#include "indexer.h"

#ifndef MAX_DBERRORS
#define MAX_DBERRORS 5
#endif

#ifndef MAX_HTTPERRORS
#define MAX_HTTPERRORS 5
#endif

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
	void pop(std::map<std::string, Link>& maplink, std::mutex& m, Indexer* idx, pqxx::connection* conn, http_request* httpreq, const std::string& wordsdbName, const std::string& docsdbName, const std::string& indexdbName)
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
					idx->parse_refs(entity.second);
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
					else
					{
						if (idx->get_DBerr_counter() == MAX_DBERRORS)
						{ 
							std::cerr << "The maximum number of exceptions when accessing the database has been exceeded. Thread " << std::this_thread::get_id() << 
								" has been terminated!" << std::endl;
							break;
						}
					}
				}
				else
				{
					if (idx->get_HTTPerr_counter() == MAX_HTTPERRORS)
					{
						std::cerr << "The maximum number of network connection exceptions has been exceeded. Thread " << std::this_thread::get_id() <<
							" has been terminated!" << std::endl;
						break;
					}
				}
				ul.unlock();
				idx->clear();
			}
			(ul.owns_lock()) ? ul.unlock() : false;
			m_cv->notify_one();
			if (exit) { break; }
		}
		(ul.owns_lock()) ? ul.unlock() : false;
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

	const bool& get_exit_status() noexcept
	{
		return exit;
	}
private:
	std::queue<T> sq;
	std::condition_variable cv_dat;
	std::condition_variable* m_cv;
	bool exit{ false };
};