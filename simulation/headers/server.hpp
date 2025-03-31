#pragma once
#include<boost/asio.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include<iostream>

namespace asio = boost::asio;
namespace Boostsystem = boost::system;

class Client_Server {
public:
	Client_Server(const std::string &server_ip, unsigned short PORT);
	std::string RunCUDAcode(std::string& sendData, bool& isLoading);
private:
	void sendingData(bool& isLoading);
	std::string RecievingData();
	void connect();
	asio::ip::tcp::socket socket;
	asio::ip::tcp::endpoint endpoint;
	asio::io_context context;
};


class Server {
public:
	Server();
private:

};