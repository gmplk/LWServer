//Based on: https://www.boost.org/doc/libs/1_86_0/libs/beast/example/http/server/small/http_server_small.cpp
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <cctype>
#include <string>
#include <fstream>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class http_connection : public std::enable_shared_from_this<http_connection>
{
public:
    http_connection(tcp::socket socket)
        : socket_(std::move(socket))
    {
    }

    void
    start()
    {
        read_request();
        check_deadline();
    }

private:
    // The socket for the currently connected client.
    tcp::socket socket_;

    // The buffer for performing reads.
    beast::flat_buffer buffer_{8192};

    // The request message.
    http::request<http::dynamic_body> request_;

    // The response message.
    http::response<http::dynamic_body> response_;

    // The timer for putting a deadline on connection processing.
    net::steady_timer deadline_{
        socket_.get_executor(), std::chrono::seconds(60)};

    // Asynchronously receive a complete request message.
    void
    read_request()
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
                if(!ec)
                    self->process_request();
            });
    }

    // Determine what needs to be done with the request message.
    void
    process_request()
    {
        response_.version(request_.version());
        response_.keep_alive(false);

        switch(request_.method())
        {
        case http::verb::get:
            response_.result(http::status::ok);
            response_.set(http::field::server, "Beast");
            create_response();
            break;

        default:
            // We return responses indicating an error if
            // we do not recognize the request method.
            response_.result(http::status::bad_request);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body())
                << "Invalid request-method '"
                << std::string(request_.method_string())
                << "'";
            break;
        }

        write_response();
    }

    std::string remove_all_whitespaces(const std::string& str) {
    	std::string result = str;
    	result.erase(std::remove_if(result.begin(), result.end(), ::isspace), result.end());
    	return result;
    }

    // Construct a response message based on the program state.
    void
    create_response()
    {
	//default "root" request
        if(request_.target() == "/")
        {
	    //show info about request
	    std::cout << request_.base();
	    //print client's IP
	    std::cout << "\naddr:" << socket_.remote_endpoint().address();
	    std::cout << "---\n";

	    //Find client's timezone based on their IP
	    std::string client_timezone = find_timezone("8.8.8.8"); //should be replaced with client's ip address (retreived as shown above)
            response_.set(http::field::content_type, "text/html");
            beast::ostream(response_.body())
                <<  "<html>\n"
                <<  "<head><title>Serwer Docker</title></head>\n"
                <<  "<body>\n"
                <<  "<h1>Serwer docker - zadanie 1</h1>\n"
                <<  "<p>IMIE NAZWISKO</p>\n"
                <<  "<p>Polaczono z adresu:</p>\n"
		<<  "<p>"
		<<  socket_.remote_endpoint().address()
		<<  "</p>\n"
		<<  "<p>Czas w strefie czasowej klienta: "
		<< std::chrono::zoned_time{remove_all_whitespaces(client_timezone), std::chrono::system_clock::now()}
	    	<<  "</p>"
                <<  "</body>\n"
                <<  "</html>\n";
        }
        else
        {
            response_.result(http::status::not_found);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body()) << "File not found\r\n";
        }
    }

    std::string find_timezone(std::string ip){
	std::string tz = "ERROR";
	try{
	net::io_context ioc;
        net::ip::tcp::resolver resolver(ioc);

	//ip-api.com can provide timezone based on IP, example: ip-api.com/csv/8.8.8.8?fields=timezone

        auto const results = resolver.resolve("ip-api.com", "80");
        net::ip::tcp::socket socket(ioc);
        net::connect(socket, results.begin(), results.end());

	std::string request_addr = "/csv/" + ip + "?fields=timezone";

        http::request<http::string_body> req(http::verb::get, request_addr, 11);
        req.set(http::field::host, "ip-api.com");
        req.set(http::field::user_agent, "Boost.Beast/1.0");

        http::write(socket, req);

        beast::flat_buffer buffer;

        http::response<http::string_body> res;

        http::read(socket, buffer, res);

        // Print the response
        std::cout << res << std::endl;
	tz = res.body();
	std::cout << tz << std::endl;

        // Gracefully close the socket
        beast::error_code ec;
        socket.shutdown(net::ip::tcp::socket::shutdown_both, ec);
        if (ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};
    	} catch (const std::exception& e) {
        	std::cerr << "Error: " << e.what() << std::endl;
    	}

    	return tz;
    }

    // Asynchronously transmit the response message.
    void
    write_response()
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

    // Check whether we have spent enough time on this connection.
    void
    check_deadline()
    {
        auto self = shared_from_this();

        deadline_.async_wait(
            [self](beast::error_code ec)
            {
                if(!ec)
                {
                    // Close socket to cancel any outstanding operation.
                    self->socket_.close(ec);
                }
            });
    }
};

// "Loop" forever accepting new connections.
void
http_server(tcp::acceptor& acceptor, tcp::socket& socket)
{
  acceptor.async_accept(socket,
      [&](beast::error_code ec)
      {
          if(!ec)
              std::make_shared<http_connection>(std::move(socket))->start();
          http_server(acceptor, socket);
      });
}

int
main(int argc, char* argv[])
{
    try
    {
        // Check command line arguments.
        if(argc != 3)
        {
            std::cerr << "Usage: " << argv[0] << " <address> <port>\n";
            std::cerr << "  For IPv4, try:\n";
            std::cerr << "    receiver 0.0.0.0 80\n";
            std::cerr << "  For IPv6, try:\n";
            std::cerr << "    receiver 0::0 80\n";
            return EXIT_FAILURE;
        }

	std::ofstream log_file("/srv_log/log.txt");
	log_file << "Server started on " << std::chrono::system_clock::now() << "\nNAME_LASTNAME\nListening on port: " << argv[2] << ".\n";
	log_file.close();

        auto const address = net::ip::make_address(argv[1]);
        unsigned short port = static_cast<unsigned short>(std::atoi(argv[2]));

        net::io_context ioc{1};

        tcp::acceptor acceptor{ioc, {address, port}};
        tcp::socket socket{ioc};
        http_server(acceptor, socket);

        ioc.run();
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
