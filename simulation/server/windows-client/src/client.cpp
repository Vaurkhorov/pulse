#include "../../headers/server.hpp"
#include <memory>
#include <string>
#include <iostream>
#include <boost/asio.hpp>

namespace asio = boost::asio;

Client_Server::Client_Server(const std::string& server_ip, unsigned short PORT)
    : context(new asio::io_context()),  // Create io_context first
    socket(*context)                  // Then initialize socket with context
{
    try {
        std::cout << "Creating endpoint..." << std::endl;

        // Parse the IP address explicitly
        asio::ip::address ip_address;
        try {
            ip_address = asio::ip::address::from_string(server_ip);
            std::cout << "IP address parsed successfully" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to parse IP address: " << e.what() << std::endl;
            throw;
        }

        // Create endpoint after parsing IP
        endpoint = asio::ip::tcp::endpoint(ip_address, PORT);
        std::cout << "Endpoint created successfully" << std::endl;

        std::cout << "Client_Server constructor completed" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in Client_Server constructor: " << e.what() << std::endl;
        throw;
    }
}

std::string Client_Server::RunCUDAcode(std::string& sendData, bool& isLoading) {
    try {
        connect();
        sendingData(isLoading);
        return RecievingData();
    }
    catch (const std::exception& e) {
        std::cerr << "Error in RunCUDAcode: " << e.what() << std::endl;
        return "ERROR: " + std::string(e.what());
    }
}

void Client_Server::connect() {
    try {
        if (socket.is_open()) {
            socket.close();
        }

        std::cout << "Connecting to server..." << std::endl;
        socket.connect(endpoint);
        std::cout << "Connected successfully!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Connection failed: " << e.what() << std::endl;
        throw;
    }
}

void Client_Server::sendingData(bool& isLoading) {
    if (!socket.is_open()) {
        throw std::runtime_error("Socket is closed!");
    }

    std::string sendingStr = "Hello";
    isLoading = true;
    asio::write(socket, asio::buffer(sendingStr));
    isLoading = false;
    std::cout << "Sent data: " << sendingStr << std::endl;
}

std::string Client_Server::RecievingData() {
    asio::streambuf recv_buffer;
    asio::read_until(socket, recv_buffer, '\0');
    std::istream recv_ss(&recv_buffer);
    std::string recv_string((std::istreambuf_iterator<char>(recv_ss)), std::istreambuf_iterator<char>());
    return recv_string;
}