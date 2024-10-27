#pragma once
#include "indexer.h"

Indexer::Indexer()
{
	locale_init();
}

void Indexer::parse_refs(const Link& link) noexcept
{
	auto words_begin = std::sregex_iterator(HtmlContent.begin(), HtmlContent.end(), word_regex);
	auto words_end = std::sregex_iterator();
	std::string buf; long sl{ 0 };

	for (std::sregex_iterator i = words_begin; i != words_end; ++i)
	{
		Link lnk;
		lnk.accept = link.accept; lnk.useragent = link.useragent;
		std::smatch match = *i;
		buf = match[1];
		if (buf.size())
		{
			(buf.find("https", 0) == std::string::npos) ? lnk.protocol = ProtocolType::HTTP : lnk.protocol = ProtocolType::HTTPS;
			buf.clear();
			buf = match[2];
			if (buf.find("www.", 0) == 0) { buf.replace(0, 4, ""); }
			sl = buf.find("/", 0);
			if (sl == -1)
			{
				lnk.hostName = buf;
				lnk.query = '/';
			}
			else
			{
				lnk.hostName = buf.substr(0, sl);
				lnk.query = buf.substr(sl);
			}
			
			this->links[match[1].str() + match[2].str()] = lnk;
		}
		else
		{
			lnk.protocol = link.protocol;
			buf.clear();
			buf = match[2];
			sl = buf.find("//", 0);
			if (sl == 0)
			{
				buf.erase(0, 2);
				if (buf.rfind('/') == buf.length()-1)
				{ 
					buf.erase(buf.length()-1, 1);
				}
				lnk.hostName = buf;
				lnk.query = '/';
				if (lnk.protocol == ProtocolType::HTTPS)
				{
					this->links["https://" + lnk.hostName] = lnk;
				}
				else
				{
					this->links["http://" + lnk.hostName] = lnk;
				}
			}
			else if (sl == -1)
			{
				sl = buf.find("/", 0);
				if (sl == -1 || sl > 0) { return; }
				lnk.hostName = link.hostName;
				lnk.query = buf;
				if (lnk.protocol == ProtocolType::HTTPS)
				{
					this->links["https://" + lnk.hostName + buf] = lnk;
				}
				else
				{
					this->links["http://" + lnk.hostName + buf] = lnk;
				}
			}
			else
			{
				return;
			}
		}
	}
}

std::map<std::string, Link> Indexer::getLinks() noexcept
{
	return links;
}

bool Indexer::addToDB(pqxx::connection* conn, const std::string& wordsdbName, const std::string& docsdbName, const std::string& indexdbName)
{
	try
	{
		if (conn == nullptr ) { throw std::runtime_error("Connection to Database not established!"); }
		if (!words.size()) { std::cerr << "No words found in " << url << std::endl; return false; }

		pqxx::nontransaction nw(*conn);
		nw.exec("begin;");
		nw.exec("INSERT INTO " + docsdbName + "(docName) VALUES ('" + url + "')");
		pqxx::result rs = nw.exec("SELECT id FROM " + docsdbName + " WHERE docName = '" + url + "'");
		for (const auto& [word, freq] : words)
		{
			pqxx::result rs1 = nw.exec("SELECT id FROM " + wordsdbName + " WHERE word = '" + word + "'");
			
			if (!rs1.size())
			{
				nw.exec("INSERT INTO " + wordsdbName + "(word) VALUES ('" + word + "')");
				rs1 = nw.exec("SELECT id FROM " + wordsdbName + " WHERE word = '" + word + "'");
			}
			nw.exec("INSERT INTO " + indexdbName + "(iddoc,idword,frequency) VALUES (" + rs[0][0].c_str() + "," + rs1[0][0].c_str() + "," + std::to_string(freq) + ")");
		}
		nw.exec("commit;");
		return true;
	}
	catch (std::runtime_error const& re)
	{
		std::cerr << "SQL ERROR: " << re.what() << std::endl;
		++DBerr_counter;
	}
	return false;
}

bool Indexer::get_http(pqxx::connection* conn, const std::string& docsdbName, http_request* httpreq, const std::string& url, const Link& lnk)
{
	try
	{
		pqxx::work w(*conn);
		pqxx::result rs = w.exec("SELECT id FROM " + docsdbName + " WHERE docName = '" + url + "'");
		w.commit();
		if (!rs.size())
		{
			this->url = url;
			HtmlContent = httpreq->getHtmlContent(lnk);
			httpreq->reset_redirects_cnt();
			if (HtmlContent == "no data")
			{ 
				++HTTPerr_counter; return false;
			}
			else if (HtmlContent == "notHTML")
			{
				return false;
			}
			boost::locale::to_lower(this->HtmlContent);
			return true;
		}
		else
		{
			return false;
		}
	}
	catch (const pqxx::usage_error ue)
	{
		std::cerr << "get_http SQL ERROR: " << ue.what() << std::endl;
		++DBerr_counter;
		return false;
	}
}

const unsigned short& Indexer::get_DBerr_counter()
{
	return DBerr_counter;
}

const unsigned short& Indexer::get_HTTPerr_counter()
{
	return HTTPerr_counter;
}

void Indexer::clear() noexcept
{
	HtmlContent.clear();
	HtmlContent.shrink_to_fit();
	url.clear();
	url.shrink_to_fit();
	links.clear();
	words.clear();
}

inline void Indexer::locale_init() noexcept
{
	boost::locale::generator gen;
	std::locale loc = gen("");
	std::locale::global(loc);
	std::cout.imbue(loc);
}

void Indexer::create_wordsbase() noexcept
{
	std::string buf;
	for (unsigned i = 0; i < HtmlContent.size(); ++i)
	{
		if (HtmlContent[i] == 32)
		{
			if (buf.size() >= MIN_WORD_LENGTH && buf.size() <= MAX_WORD_LENGTH)
			{
				(!words.count(buf)) ? words[buf] = 1 : words[buf] += 1;
			}
			buf.clear();
			buf.shrink_to_fit();
		}
		else if (!((HtmlContent[i] >= 0 && HtmlContent[i] < 32) ||
			(HtmlContent[i] > 32 && HtmlContent[i] < 48) ||
			(HtmlContent[i] > 57 && HtmlContent[i] < 65) ||
			(HtmlContent[i] > 90 && HtmlContent[i] < 97) ||
			(HtmlContent[i] > 122 && HtmlContent[i] < 128)))
		{
			(buf.size() <= MAX_WORD_LENGTH) ? buf.push_back(HtmlContent[i]) : false;
		}
	}
	if (!words.size() && (buf.size() >= MIN_WORD_LENGTH && buf.size() <= MAX_WORD_LENGTH))
	{
		words[buf] = 1;
	}
}

Indexer& Indexer::operator=(const Indexer& idx)
{
	if (*this != idx)
	{
		this->HtmlContent = idx.HtmlContent;
		this->links.clear();
		this->links = idx.links;
	}
	return *this;
}

bool Indexer::operator==(const Indexer& idx) const
{
	return this->HtmlContent == idx.HtmlContent && this->links == idx.links;
}

bool Indexer::operator!=(const Indexer& idx)
{
	return this->HtmlContent != idx.HtmlContent || this->links != idx.links;
}

Indexer::~Indexer()
{
}

