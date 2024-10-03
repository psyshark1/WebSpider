#pragma once
#include"http_server.h"

std::string convert_to_utf8(const std::string& encoded)
{
	std::string res;
	std::istringstream iss(encoded);
	char ch;

	while (iss.get(ch))
	{
		if (ch == '%')
		{
			int hex;
			iss >> std::hex >> hex;
			res += static_cast<char>(hex);
		}
		else
		{
			res += ch;
		}
	}
	return res;
}

http_server::http_server(tcp::socket socket, std::string* dbTblDocsName, std::string* dbTblWordsName, std::string* dbTblIndexerName, pqxx::nontransaction* nt) : socket_(std::move(socket)),
dbTblDocsName(std::move(dbTblDocsName)), dbTblWordsName(std::move(dbTblWordsName)), dbTblIndexerName(std::move(dbTblIndexerName)), nt(std::move(nt))
{
}

void http_server::start()
{
	readRequest();
	checkDeadline();
}

void http_server::readRequest()
{
	auto self = shared_from_this();

	http::async_read(
		socket_,
		buffer_,
		request_,
		[self](beast::error_code ec,
			std::size_t bytes_transferred)
		{
			boost::ignore_unused(bytes_transferred);
			if (!ec)
				self->processRequest();
		});
}

void http_server::processRequest()
{
	response_.version(request_.version());
	response_.keep_alive(false);

	switch (request_.method())
	{
	case http::verb::get:
		response_.result(http::status::ok);
		response_.set(http::field::server, "Beast");
		createResponseGet();
		break;
	case http::verb::post:
		response_.result(http::status::ok);
		response_.set(http::field::server, "Beast");
		createResponsePost();
		break;

	default:
		response_.result(http::status::bad_request);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body())
			<< "Invalid request-method '"
			<< std::string(request_.method_string())
			<< "'";
		break;
	}

	writeResponse();
}

void http_server::createResponseGet()
{
	if (request_.target() == "/")
	{
		response_.set(http::field::content_type, "text/html");
		beast::ostream(response_.body())
			<< "<html>\n"
			<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
			<< "<body>\n"
			<< "<h1>Search Engine</h1>\n"
			<< "<p>Welcome!<p>\n"
			<< "<form action=\"/\" method=\"post\">\n"
			<< "    <label for=\"search\">Search:</label><br>\n"
			<< "    <input type=\"text\" id=\"search\" name=\"search\"><br>\n"
			<< "    <input type=\"submit\" value=\"Search\">\n"
			<< "</form>\n"
			<< "</body>\n"
			<< "</html>\n";
	}
	else
	{
		response_.result(http::status::not_found);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body()) << "File not found\r\n";
	}
}

void http_server::createResponsePost()
{
	if (request_.target() == "/")
	{
		std::string s = buffers_to_string(request_.body().data());

		std::cout << "POST data: " << s << std::endl;

		size_t pos = s.find('=');
		if (pos == std::string::npos)
		{
			response_.result(http::status::not_found);
			response_.set(http::field::content_type, "text/plain");
			beast::ostream(response_.body()) << "File not found\r\n";
			return;
		}

		std::string key = s.substr(0, pos);
		std::string value = s.substr(pos + 1);

		std::string utf8value = convert_to_utf8(value);

		if (key != "search")
		{
			response_.result(http::status::not_found);
			response_.set(http::field::content_type, "text/plain");
			beast::ostream(response_.body()) << "File not found\r\n";
			return;
		}

		std::vector<std::string> searchResult = getSQLresult(utf8value);

		size_t start{ utf8value.find("+")};
		while (start != std::string::npos)
		{
			utf8value.replace(start, 1, " ");
			start = utf8value.find("+", start + 1);
		}

		response_.set(http::field::content_type, "text/html");
		beast::ostream(response_.body())
			<< "<html>\n"
			<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
			<< "<body>\n"
			<< "<h1>Search Engine</h1>\n"
			<< "<form action=\"/\" method=\"post\">\n"
			<< "    <label for=\"search\">Search:</label><br>\n"
			<< "    <input type=\"text\" id=\"search\" name=\"search\" value=\""<< utf8value <<"\"><br>\n"
			<< "    <input type=\"submit\" value=\"Search\">\n"
			<< "</form>\n"
			<< "<p>Results for: "<< utf8value <<"<p>\n"
			<< "<ul>\n";

		if (searchResult.size())
		{
			for (const auto& url : searchResult) {

				if (url.find("http", 0) != std::string::npos)
				{
					beast::ostream(response_.body())
						<< "<li><a href=\""
						<< url << "\">"
						<< url << "</a></li>";
				}
				else
				{
					response_.result(http::status::internal_server_error);
					beast::ostream(response_.body())
						<< "<li>" << url << "</li>";
				}
			}
		}
		else
		{
			beast::ostream(response_.body()) << "<li>Not found!</li>";
		}

		beast::ostream(response_.body())
			<< "</ul>\n"
			<< "</body>\n"
			<< "</html>\n";
	}
	else
	{
		response_.result(http::status::not_found);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body()) << "File not found\r\n";
	}
}

void http_server::writeResponse()
{
	auto self = shared_from_this();

	response_.content_length(response_.body().size());

	http::async_write(
		socket_,
		response_,
		[self](beast::error_code ec, std::size_t)
		{
			self->socket_.shutdown(tcp::socket::shutdown_send, ec);
			self->deadline_.cancel();
		});
}

void http_server::checkDeadline()
{
	auto self = shared_from_this();

	deadline_.async_wait(
		[self](beast::error_code ec)
		{
			if (!ec)
			{
				self->socket_.close(ec);
			}
		});
}

std::vector<std::string> http_server::getSQLresult(std::string& query)
{
	std::vector<std::string> result;
	if (nt == nullptr) 
	{
		result.push_back("Connection to Database not established!");
		return result;
	}
	if (dbTblDocsName == nullptr || dbTblWordsName == nullptr || dbTblIndexerName == nullptr)
	{
		result.push_back("Internal Server Error!");
		return result;
	}
	std::map<std::string, unsigned> words;
	std::string buf;
	pqxx::result rs;
	unsigned char wordschk{0};
	unsigned i = 0;

	for (; i < query.size(); ++i)
	{
		if (query[i] == 43)
		{
			if (buf.size() > 3 && buf.size() < 31)
			{
				words[buf] = 0;
				buf.clear();
				if (words.size() == 4) { break; }
			}
			
		}
		else if (!((query[i] >= 0 && query[i] < 32) ||
			(query[i] > 32 && query[i] < 48) ||
			(query[i] > 57 && query[i] < 65) ||
			(query[i] > 90 && query[i] < 97) ||
			(query[i] > 122 && query[i] < 128)))
		{
			buf.push_back(query[i]);
		}

	}
	if (!words.size() && (buf.size() > 3 && buf.size() < 31))
	{
		words[buf] = 0; buf.clear();
	}
	buf.shrink_to_fit();

	if (!words.size()) { return result; }

	try
	{
		nt->exec("begin;");
		i = 0;
		for (const auto& [word, id] : words)
		{
			rs = nt->exec("SELECT id FROM "+ *dbTblWordsName +" WHERE word = '" + word + "'");
			if (rs.size())
			{
				words[word] = rs[0][0].as<unsigned>();
				wordschk |= (1 << i);
				rs.clear();
			}
			++i;
		}
		nt->exec("commit;");

		if (wordschk)
		{
			nt->exec("begin;");
			buf = "SELECT docname FROM "+ *dbTblDocsName +" AS h INNER JOIN (SELECT iddoc, sum(frequency) s FROM ((";

			for (const auto& [word, id] : words)
			{
				if (id) { buf.append("SELECT iddoc, frequency FROM "+ *dbTblIndexerName +" h WHERE idword = " + std::to_string(id) + " UNION ALL "); }
			}

			buf.erase(buf.size() - 11);
			buf.append(") ORDER BY frequency DESC LIMIT 10) as f GROUP BY iddoc ORDER BY s DESC) AS fin ON h.id = fin.iddoc");
			rs = nt->exec(buf);

			if (rs.size())
			{ 
				for (const auto& row : rs)
				{
					result.push_back(row[0].c_str());
				}
			}
			nt->exec("commit;");
		}
		return result;
	}
	catch (const std::exception& e)
	{
		nt->abort();
		result.push_back(e.what());
		return result;
	}
}

http_server::~http_server()
{
}
