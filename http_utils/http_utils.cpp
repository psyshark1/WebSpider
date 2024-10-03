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
			(link.query.length() > 1) ? req.set(http::field::host, link.hostName) : req.set(http::field::host, "www." + link.hostName);
			req.set(http::field::user_agent, link.useragent);
			req.set(http::field::accept, link.accept);

			stream.handshake(ssl::stream_base::client);
			http::write(stream, req);

			beast::flat_buffer buffer;
			http::response<http::dynamic_body> res;
			http::read(stream, buffer, res);

			result = buffers_to_string(res.body().data());

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
			req.set(http::field::host, "www." + link.hostName);
			req.set(http::field::user_agent, link.useragent);
			req.set(http::field::accept, link.accept);

			http::write(stream, req);

			beast::flat_buffer buffer;
			http::response<http::dynamic_body> res;

			http::read(stream, buffer, res);

			result = buffers_to_string(res.body().data());
			beast::error_code ec;
			stream.socket().shutdown(tcp::socket::shutdown_both, ec);

			if (ec && ec != beast::errc::not_connected)
			{
				throw beast::system_error{ ec };
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cout << link.hostName << link.query << std::endl << e.what() << std::endl;
		return "no data";
	}
	if (result.find("<html", 0) == std::string::npos && result.find("<body", 5) == std::string::npos)
	{
		return "no data";
	}
	else
	{
		return result;
	}
	
}

http_request::~http_request()
{
}