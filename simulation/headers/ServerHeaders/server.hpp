#pragma once
#include "../../headers/Visualisation_Headers/roadStructure.hpp"
#include "../../headers/CUDA_SimulationHeaders/idm.hpp"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <iostream>
#include <sstream>

namespace asio = boost::asio;
namespace Boostsystem = boost::system;

// Serializable data structures that mirror our application structures
namespace serialization {
    struct LaneCell {
        int next_in_lane;
        int left_cell;
        int right_cell;
        float length;

        template<class Archive>
        void serialize(Archive& ar, const unsigned int) {
            ar& next_in_lane;
            ar& left_cell;
            ar& right_cell;
            ar& length;
        }
    };

    struct Vehicle {
        float s;
        float v;
        size_t segment;
        bool active;
        size_t pathIndex;

        template<class Archive>
        void serialize(Archive& ar, const unsigned int) {
            ar& s;
            ar& v;
            ar& segment;
            ar& active;
            ar& pathIndex;
        }
    };

    struct SimulationRequest {
        std::vector<LaneCell> laneCells;
        std::vector<Vehicle> vehicles;
        float deltaTime;
        int numSteps;

        template<class Archive>
        void serialize(Archive& ar, const unsigned int) {
            ar& laneCells;
            ar& vehicles;
            ar& deltaTime;
            ar& numSteps;
        }
    };

    struct SimulationResponse {
        std::vector<Vehicle> updatedVehicles;
        bool success;
        std::string errorMessage;

        template<class Archive>
        void serialize(Archive& ar, const unsigned int) {
            ar& updatedVehicles;
            ar& success;
            ar& errorMessage;
        }
    };
}

class Client_Server {
public:
    Client_Server(const std::string& server_ip, unsigned short PORT);
    std::string RunCUDAcode(std::string& sendData, bool& isLoading,
        float deltaTime = 0.016f, int numSteps = 10);
    void setLaneCells(const std::vector<LaneCell>& cells);

private:
    void sendingData(bool& isLoading);
    std::string RecievingData();
    void connect();
    std::vector<serialization::LaneCell> laneCells;

    // Use a unique_ptr for io_context to ensure it's properly initialized before socket
    std::unique_ptr<asio::io_context> context;
    asio::ip::tcp::socket socket;
    asio::ip::tcp::endpoint endpoint;
};


class SimulationServer {
public:
	SimulationServer(asio::io_context& io, unsigned short port);
private:
	asio::ip::tcp::acceptor acceptor_;
	void handle_client(std::shared_ptr<asio::ip::tcp::socket> socket);
	void start_accept();
};