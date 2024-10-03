#pragma once
#include <sstream>
#include <iomanip>
#include <locale>
#include <codecvt>
#include <iostream>
#include <pqxx/pqxx>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class http_server : public std::enable_shared_from_this<http_server>
{
public:
	http_server(tcp::socket socket, std::string* dbTblDocsName, std::string* dbTblWordsName, std::string* dbTblIndexerName, pqxx::nontransaction* nt);
	http_server(const http_server&) = delete;
	http_server(http_server&) = delete;
	http_server& operator= (http_server&) = delete;
	http_server& operator= (const http_server&) = delete;
	void start();
	~http_server();

protected:
	tcp::socket socket_;

	beast::flat_buffer buffer_{ 8192 };

	http::request<http::dynamic_body> request_;

	http::response<http::dynamic_body> response_;

	net::steady_timer deadline_{
		socket_.get_executor(), std::chrono::seconds(60) };

	void readRequest();
	void processRequest();

	void createResponseGet();

	void createResponsePost();
	void writeResponse();
	void checkDeadline();

private:
	pqxx::nontransaction* nt;
	std::vector<std::string> getSQLresult(std::string& query);
	const std::string* dbTblDocsName;
	const std::string* dbTblWordsName;
	const std::string* dbTblIndexerName;
};