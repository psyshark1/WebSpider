﻿#pragma once
#include "stringstructs.h"
#include "INI_parser.h"
#include "thread_pool.h"
#include <tlhelp32.h>

//std::atomic<unsigned>idword{ 1 };
//std::atomic<unsigned>idref{ 1 };

std::mutex m;
std::condition_variable cv;
std::map<std::string, Link> maplink;

bool getWinHandle(const char* proc)
{
	bool r{ false };
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (Process32First(snapshot, &entry) == TRUE)
	{
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			if (stricmp(entry.szExeFile, proc) == 0)
			{
				HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
				r = true;
				CloseHandle(hProcess);
			}
		}
	}
	CloseHandle(snapshot);
	return r;
}

int main()
{
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
	std::system("chcp 1251");

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	bool WebSpider_S{ false };

	Indexer* idx{ nullptr };
	try
	{
		ini_parser parser(inistr.INIFile);
		const int searchdepth = parser.get_value<int>(inistr.searchdepth);
		if (searchdepth < 1) { throw std::runtime_error("Search depth cannot be less than 1!"); }
		const unsigned short serverPort = parser.get_value<int>(inistr.ServerPort);
		if (!serverPort) { throw std::runtime_error("Server Port cannot be 0!"); }

		const std::string serverHost = parser.get_value<std::string>(inistr.ServerHost);
		
		std::string starturl = parser.get_value<std::string>(inistr.StartURL);
		const std::string accept = parser.get_value<std::string>(inistr.accept);
		const std::string useragent = parser.get_value<std::string>(inistr.useragent);
		
		Link lnk;
		{
			if (starturl.find("https", 0) == 0)
			{
				lnk.protocol = ProtocolType::HTTPS;
			}
			else if (starturl.find("http", 0) == 0)
			{
				lnk.protocol = ProtocolType::HTTP;
			}
			else
			{
				throw std::runtime_error(inistr.StartURLERR);
			}

			long sls = starturl.find("://", 0);
			if (sls == -1)
			{
				throw std::runtime_error(inistr.StartURLERR);
			}
			else
			{
				long port = starturl.find(":", sls + 3);
				if (port > -1)
				{
					starturl.erase(port, starturl.find("/", port) - port);
				}
				long dot = starturl.find(".", sls);
				if(dot == -1){ throw std::runtime_error(inistr.StartURLERR); }
				long sl = starturl.find('/', sls+3);
				if (sl == -1)
				{
					lnk.hostName = starturl.substr(sls+3);
					lnk.query = '/';
					starturl.push_back('/'); 
				}
				else
				{
					lnk.hostName = starturl.substr(sls+3, sl-sls-3);
					lnk.query = starturl.substr(sl);
				}
			}
		}
		lnk.accept = accept; lnk.useragent = useragent;

		const std::string postgresuser = parser.get_value<std::string>(inistr.user);
		const std::string postgreshost = parser.get_value<std::string>(inistr.host);
		const std::string postgrespass = parser.get_value<std::string>(inistr.password);
		const std::string postgresdbname = parser.get_value<std::string>(inistr.dbname);

		const std::string connstr = dbstr.user + postgresuser + ' ' +
			dbstr.host + postgreshost + ' ' +
			dbstr.password + postgrespass + ' ' +
			dbstr.dbname + postgresdbname;

		pqxx::connection dbcon(connstr);

		{
			pqxx::work w(dbcon);
			w.exec(requests.DropTableIndexer);
			w.exec(requests.DropTableDocs);
			w.exec(requests.DropTableWords);
			w.exec(requests.CreateTableDocs);
			w.exec(requests.CreateTableWords);
			w.exec(requests.CreateTableIndexer);
			w.commit();
		}

		http_request httpreq;

		maplink[starturl] = lnk;
		
		idx = new Indexer();
		if (idx->get_http(&dbcon, dbstr.dbTblDocsName, &httpreq, starturl, maplink[starturl]))
		{
			idx->parse_refs(maplink[starturl]);
			idx->create_wordsbase();
			if (idx->addToDB(&dbcon, dbstr.dbTblWordsName, dbstr.dbTblDocsName, dbstr.dbTblIndexerName))
			{
				maplink.clear();
				maplink = idx->getLinks();
				idx->clear();

				/*std::string startserver = ' ' + serverHost + ' ' + std::to_string(serverPort) + ' ' +
					postgresuser + ' ' + postgreshost + ' ' + postgrespass + ' ' + postgresdbname + ' ' +
					dbstr.dbTblDocsName + ' ' + dbstr.dbTblWordsName + ' ' + dbstr.dbTblIndexerName;*/
				if (!getWinHandle(dbstr.httpServerexec))
				{
					WebSpider_S = CreateProcess(dbstr.httpServerexec, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
					if (!WebSpider_S)//&startserver.front()
					{
						std::cerr << "WebSpider_Server ERROR: WebSpider_Server not started!" << std::endl << "Database word search is not possible." << std::endl;
					}
				}
				if (searchdepth > 1)
				{
					if (maplink.size())
					{
						thread_pool thpool(connstr, maplink, &cv, m, dbstr.dbTblWordsName, dbstr.dbTblDocsName, dbstr.dbTblIndexerName);
						int i = 1;
						std::unique_lock ul{ m };
						while (i <= searchdepth)
						{
							if (thpool.get_emrg_exit_counter() == std::thread::hardware_concurrency())
							{
								break;
							}
							else if (!maplink.empty() && !thpool.get_queue_size())
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
							cv.wait(ul, [&]() {return (!maplink.empty() && !thpool.get_queue_size()) || (maplink.empty() && !thpool.get_queue_size()) || (thpool.get_emrg_exit_counter() == std::thread::hardware_concurrency()); });
							ul.unlock();
						}
						thpool.stop();
					}
				}
				maplink.clear();

				pqxx::nontransaction nw(dbcon);
				nw.exec("begin;");
				pqxx::result rs = nw.exec("SELECT COUNT(id) FROM " + std::string(dbstr.dbTblDocsName));
				pqxx::result rs1 = nw.exec("SELECT COUNT(id) FROM " + std::string(dbstr.dbTblWordsName));
				nw.exec("commit;");

				std::cout << "Parsing Complete" << std::endl << "Found " << rs1[0][0].as<unsigned>() << " word(s) in " << rs[0][0].as<unsigned>() << " HTML Document(s)" << std::endl;

				if (WebSpider_S)
				{
					std::cout << "Open browser and connect to http://" << serverHost << ":" << serverPort << " to see the web server operating" << std::endl;
					WaitForSingleObject(pi.hProcess, INFINITE);

					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
				}
				else
				{
					std::cout << "Try running the WebSpider_Server program manually to search for the words found." << std::endl;
				}
			}
		}
		else
		{
			std::cerr << "ERROR: No HTML document found in " << starturl << std::endl;
		}
		delete idx;
	}
	catch (std::runtime_error const& re)
	{
		delete idx;
		std::cerr << "ERROR: " << re.what() << std::endl;
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