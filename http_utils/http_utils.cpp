#pragma once 
#include"http_utils.h"

http_request::http_request()
{
}

std::string http_request::getHtmlContent(const Link& link)
{
	std::string result;
	try
	{
		net::io_context ioc;
		tcp::resolver resolver(ioc);
		bool isText{ false };
		if (link.protocol == ProtocolType::HTTPS)
		{
			ssl::context ctx(ssl::context::tlsv13_client);
			ctx.set_default_verify_paths();

			beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
			stream.set_verify_mode(ssl::verify_none);

			stream.set_verify_callback([](bool preverified, ssl::verify_context& ctx)
				{
					return true; // Accept any certificate
				});

			if (!SSL_set_tlsext_host_name(stream.native_handle(), link.hostName.c_str()))
			{
				beast::error_code ec{ static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
				throw beast::system_error{ ec };
			}
			get_lowest_layer(stream).connect(resolver.resolve({ link.hostName, "https"}));
			get_lowest_layer(stream).expires_after(std::chrono::seconds(30));
			http::request<http::empty_body> req{ http::verb::get, link.query, 11 };
			req.set(http::field::host, link.hostName);
			req.set(http::field::user_agent, link.useragent);
			req.set(http::field::accept, link.accept);

			stream.handshake(ssl::stream_base::client);
			http::write(stream, req);

			beast::flat_buffer buffer;
			http::response<http::dynamic_body> res;
			http::read(stream, buffer, res);

			if (res.result_int() == 301 || res.result_int() == 302)
			{
				++recv_counter;
				if (recv_counter == MAX_REDIRECT) { throw std::runtime_error("Looping Redirect!"); }
				return getHtmlContent(redirect(res, link));
			}
			else if (res.result_int() == 200)
			{
				result = buffers_to_string(res.body().data());
				if (result.find("301 Moved Permanently", 0) != std::string::npos && result.length() < 250)
				{
					++recv_counter;
					if (recv_counter == MAX_REDIRECT) { throw std::runtime_error("Looping Redirect!"); }
					Link lnk = link;
					lnk.protocol = ProtocolType::HTTPS;
					return getHtmlContent(lnk);
				}
			}
			else if (res.result_int() == 307)
			{
				++recv_counter;
				if (recv_counter == MAX_REDIRECT) { throw std::runtime_error("Looping Redirect!"); }
				std::string loc = res.base()["Location"];
				Link lnk = link; lnk.query = loc;
				return getHtmlContent(lnk);
			}
			else
			{
				result = buffers_to_string(res.body().data());
			}

			beast::error_code ec;
			stream.shutdown(ec);
			if (ec == net::error::eof || ec == ssl::error::stream_truncated)
			{
				ec = {};
			}

			if (ec)
			{
				throw beast::system_error{ ec };
			}
		}
		else
		{
			beast::tcp_stream stream(ioc);

			auto const results = resolver.resolve(link.hostName, "http");

			stream.connect(results);

			http::request<http::string_body> req{ http::verb::get, link.query, 11 };
			req.set(http::field::host, link.hostName);
			req.set(http::field::user_agent, link.useragent);
			req.set(http::field::accept, link.accept);

			http::write(stream, req);

			beast::flat_buffer buffer;
			http::response<http::dynamic_body> res;
			http::read(stream, buffer, res);

			if (res.result_int() == 301 || res.result_int() == 302)
			{
				++recv_counter;
				if (recv_counter == MAX_REDIRECT) { throw std::runtime_error("Looping Redirect!"); }
				return getHtmlContent(redirect(res, link));
			}
			else if (res.result_int() == 200)
			{
				result = buffers_to_string(res.body().data());
				if (result.find("301 Moved Permanently", 0) != std::string::npos && result.length() < 250)
				{
					++recv_counter;
					if (recv_counter == MAX_REDIRECT) { throw std::runtime_error("Looping Redirect!"); }
					Link lnk = link;
					lnk.protocol = ProtocolType::HTTPS;
					return getHtmlContent(lnk);
				}
			}

			beast::error_code ec;
			stream.socket().shutdown(tcp::socket::shutdown_both, ec);

			if (ec && ec != beast::errc::not_connected)
			{
				throw beast::system_error{ ec };
			}
		}
	}
	catch (const std::runtime_error& re)
	{
		std::cout << "HTTP Client ERROR: " << link.hostName << link.query << std::endl << re.what() << std::endl;
		return "no data";
	}
	if (result.find("<html", 0) == std::string::npos && result.find("<body", 5) == std::string::npos)
	{
		return "notHTML";
	}
	else
	{
		return result;
	}
}

void http_request::reset_redirects_cnt()
{
	recv_counter = 0;
}

inline Link http_request::redirect(http::response<http::dynamic_body>& res, const Link& link)
{
	Link lnk;

	std::string loc = res.base()["Location"];
	if (loc.find("https", 0) == 0)
	{
		lnk.protocol = ProtocolType::HTTPS;
	}
	else if (loc.find("http", 0) == 0)
	{
		lnk.protocol = ProtocolType::HTTP;
	}
	else
	{
		lnk.protocol = ProtocolType::HTTPS;
	}

	long sls = loc.find("://", 0);
	if (sls == -1)
	{
		lnk.hostName = link.hostName;
		lnk.query = loc;
	}
	else
	{
		long dot = loc.find(".", sls);
		if (dot == -1) { throw std::runtime_error("No domain dot separate!");}
		long sl = loc.find('/', sls + 3);
		if (sl == -1)
		{
			lnk.hostName = loc.substr(sls + 3);
			lnk.query = '/';
			loc.push_back('/');
		}
		else
		{
			lnk.hostName = loc.substr(sls + 3, sl - sls - 3);
			lnk.query = loc.substr(sl);
		}
	}
	lnk.accept = link.accept; lnk.useragent = link.useragent;
	return lnk;
}

http_request::~http_request()
{
}