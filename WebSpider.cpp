#pragma once
#include"stringstructs.h"
#include"INI_parser.h"
#include"thread_pool.h"
#include "http_server.h"

//std::atomic<unsigned>idword{ 1 };
//std::atomic<unsigned>idref{ 1 };

std::mutex m;
std::condition_variable cv;
std::map<std::string, Link> maplink;

void httpServer(tcp::acceptor& acceptor, tcp::socket& socket, std::string& dbTblDocsName, std::string& dbTblWordsName, std::string& dbTblIndexerName, pqxx::nontransaction& nt)
{
	acceptor.async_accept(socket,
		[&](beast::error_code ec)
		{
			if (!ec)
			{
				std::make_shared<http_server>(std::move(socket), std::move(&dbTblDocsName), std::move(&dbTblWordsName), std::move(&dbTblIndexerName), std::move(&nt))->start();
				httpServer(acceptor, socket, dbTblDocsName, dbTblWordsName, dbTblIndexerName, nt);
			}
		});
}

int main()
{
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
	std::system("chcp 1251");

	try
	{
		ini_parser parser("WebSpider.ini");
		const int searchdepth = parser.get_value<int>(inistr.searchdepth);
		if (searchdepth < 1) { throw std::exception("Search depth cannot be less than 1!"); }
		const unsigned short serverPort = parser.get_value<int>(inistr.ServerPort);
		if (!serverPort) { throw std::exception("Server Port cannot be 0!"); }

		const std::string serverHost = parser.get_value<std::string>(inistr.ServerHost);
		
		const std::string starturl = parser.get_value<std::string>(inistr.StartURL);
		
		std::string accept = parser.get_value<std::string>(inistr.accept);
		std::string useragent = parser.get_value<std::string>(inistr.useragent);

		const std::string connstr = dbstr.user + parser.get_value<std::string>(inistr.user) + ' ' +
			dbstr.host + parser.get_value<std::string>(inistr.host) + ' ' +
			dbstr.password + parser.get_value<std::string>(inistr.password) + ' ' +
			dbstr.dbname + parser.get_value<std::string>(inistr.dbname);
		pqxx::connection dbcon(connstr);
		pqxx::work w(dbcon);

		w.exec(requests.DropTableDocs);
		w.exec(requests.DropTableWords);
		w.exec(requests.DropTableIndexer);
		w.exec(requests.CreateTableDocs);
		w.exec(requests.CreateTableWords);
		w.exec(requests.CreateTableIndexer);
		w.commit();
		w.~transaction();

		http_request httpreq;

		maplink[starturl + '/'] = { ProtocolType::HTTPS, starturl, "/", accept, useragent };
		
		Indexer* idx{ new Indexer() };
		if (idx->get_http(&dbcon, dbstr.dbTblDocsName, &httpreq, starturl + '/', maplink[starturl + '/']))
		{
			idx->parse_refs(accept, useragent);
			idx->create_wordsbase();
			if (idx->addToDB(&dbcon, dbstr.dbTblWordsName, dbstr.dbTblDocsName, dbstr.dbTblIndexerName))
			{
				maplink.clear();
				maplink = idx->getLinks();
				idx->clear();

				if (searchdepth > 1)
				{
					if (maplink.size())
					{
						thread_pool thpool(connstr, maplink, &cv, m, dbstr.dbTblWordsName, dbstr.dbTblDocsName, dbstr.dbTblIndexerName, accept, useragent);
						int i = 1;
						std::unique_lock ul{ m };
						while (i <= searchdepth)
						{
							if (!maplink.empty() && !thpool.get_queue_size())
							{
								if (i < searchdepth)
								{
									(!ul.owns_lock()) ? ul.lock() : false;
									for (const auto& it : maplink)
									{
										std::pair<std::string, Link> pt1 = it;
										thpool.submit(std::move(pt1));
									}
									maplink.clear();
									ul.unlock();
									++i;
									thpool.go();
								}
								else
								{
									break;
								}
							}
							else if (maplink.empty() && !thpool.get_queue_size())
							{
								break;
							}
							(!ul.owns_lock()) ? ul.lock() : false;
							cv.wait(ul, [&]() {return (!maplink.empty() && !thpool.get_queue_size()) || (maplink.empty() && !thpool.get_queue_size()); });
							ul.unlock();
						}
						thpool.stop();
					}
				}
				maplink.clear();

				auto const address = net::ip::make_address(serverHost);

				net::io_context ioc{ 1 };

				tcp::acceptor acceptor{ ioc, { address, serverPort } };
				tcp::socket socket{ ioc };
				std::string dbTblDocsName = dbstr.dbTblDocsName;
				std::string dbTblWordsName = dbstr.dbTblWordsName;
				std::string dbTblIndexerName = dbstr.dbTblIndexerName;
				pqxx::nontransaction nw(dbcon);
				httpServer(acceptor, socket, dbTblDocsName, dbTblWordsName, dbTblIndexerName, nw);

				nw.exec("begin;");
				pqxx::result rs = nw.exec("SELECT COUNT(id) FROM " + std::string(dbstr.dbTblDocsName));
				pqxx::result rs1 = nw.exec("SELECT COUNT(id) FROM " + std::string(dbstr.dbTblWordsName));
				nw.exec("commit;");
				rs[0][0].as<unsigned>();
				std::cout << "Found " << rs1[0][0].as<unsigned>() << " word(s) in " << rs[0][0].as<unsigned>() << " HTML Document(s)" << std::endl 
					<< "Open browser and connect to http://" << serverHost << ":" << serverPort << " to see the web server operating" << std::endl;

				ioc.run();
			}
		}
		else
		{
			std::cerr << "ERROR: No HTML document found in " << starturl << std::endl;
		}
	}
	catch (std::exception const& e)
	{
		std::cerr << "ERROR: " << e.what() << std::endl;
		system("pause");
		return 1;
	}
	catch (WrongINI& w)
	{
		std::cerr << "INI ERROR: " << w.what() << std::endl;
		system("pause");
		return 2;
	}
	return 0;
}