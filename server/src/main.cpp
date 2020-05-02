#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "odbc.h"
//#include "tagexception.h"

#include "session.h"
//#include "sender.h"

class server
{
public:
	server(boost::asio::io_service &io_service, unsigned short portReq, const char *dataSource)
		: io_service_(io_service),
		  acceptor_(io_service,
					boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), portReq)),
		  context_(boost::asio::ssl::context::sslv23)
	{
		std::ostringstream err;
		if (odbc_login(err, dataSource, NULL, NULL)) {
			throw std::runtime_error("odbc_login: " + err.str());
		}
		SQLExecDirect(serverhstmt, (SQLCHAR*)"USE wechat;", SQL_NTS);
		context_.set_options(
			boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2
			 | boost::asio::ssl::context::single_dh_use);
		//context_.set_password_callback(boost::bind(&server::get_password, this));
		context_.use_certificate_chain_file("server.pem");
		context_.use_private_key_file("server.pem", boost::asio::ssl::context::pem);
		context_.use_tmp_dh_file("dh2048.pem");

		start_accept_req();
	}

	void start_accept_req()
	{
		session *new_session = new session(io_service_, context_);
		acceptor_.async_accept(new_session->socket(),
							   boost::bind(&server::handle_accept_req, this, new_session,
										   boost::asio::placeholders::error));
	}
	void handle_accept_req(session *new_session,
					   const boost::system::error_code &error)
	{
		if (!error)
		{
			new_session->start();
		}
		else
		{
			delete new_session;
		}

		start_accept_req();
	}

private:
	boost::asio::io_service &io_service_;
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::ssl::context context_;
};

int main(int argc, char *argv[])
{
	try
	{
		if (argc != 2)
		{
			std::cerr << "Usage: server <port>\n";
			return 1;
		}

		boost::asio::io_service io_service;
		int port = atoi(argv[1]);
		server s(io_service, port, "wechatserver");
		std::cerr << "Listening at port " << port << std::endl;
		//std::cout << "Listening at port " << port << std::endl;
		io_service.run();
	}
	catch (std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
