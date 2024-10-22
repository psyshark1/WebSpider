#pragma once 
#include <string>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <openssl/ssl.h>
#include <iostream>
#include "lnk.h"

#ifndef MAX_REDIRECT
#define MAX_REDIRECT 5
#endif

namespace ssl = boost::asio::ssl;
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ip = boost::asio::ip;
using tcp = boost::asio::ip::tcp;

class http_request
{
public:
	http_request();
	http_request(const http_request&) = delete;
	http_request(http_request&) = delete;
	http_request& operator= (http_request&) = delete;
	http_request& operator= (const http_request&) = delete;
	~http_request();
	std::string getHtmlContent(const Link& link);
	void reset_redirects_cnt();
private:
	Link redirect(http::response<http::dynamic_body>& res, const Link& link);
	unsigned short recv_counter{ 0 };
};