#include "../../headers/ServerHeaders/server.hpp"

// Helper functions declarations
void convertDotsToVehicles(const std::vector<Dot>& appDots,
    std::vector<serialization::Vehicle>& vehicles);
void updateDotsFromVehicles(std::vector<Dot>& appDots,
    const std::vector<serialization::Vehicle>& vehicles);
std::vector<serialization::LaneCell> createDummyLaneCells();

// Use existing constructor implementation
Client_Server::Client_Server(const std::string& server_ip, unsigned short PORT)
    : context(new asio::io_context()),
    socket(*context)
{
    try {
        std::cout << "Creating endpoint..." << std::endl;

        asio::ip::address ip_address;
        try {
            ip_address = asio::ip::address::from_string(server_ip);
            std::cout << "IP address parsed successfully" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to parse IP address: " << e.what() << std::endl;
            throw;
        }

        endpoint = asio::ip::tcp::endpoint(ip_address, PORT);
        std::cout << "Endpoint created successfully" << std::endl;

        std::cout << "Client_Server constructor completed" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in Client_Server constructor: " << e.what() << std::endl;
        throw;
    }
}

// Main method to run CUDA simulation on server
std::string Client_Server::RunCUDAcode(std::string& sendData, bool& isLoading,
    float deltaTime, int numSteps) {
    try {
        connect();
        isLoading = true;

        // Create a simulation request with lane data and vehicle data
        serialization::SimulationRequest request;

        // Use a function to create lane cells without directly accessing lane0_graph
        request.laneCells = createDummyLaneCells();

        request.deltaTime = deltaTime;
        request.numSteps = numSteps;


        // Extract vehicle data from global dots vector
        std::vector<serialization::Vehicle> serializableVehicles;
        convertDotsToVehicles(dots, serializableVehicles);
        request.vehicles = serializableVehicles;

        // Set simulation parameters
        request.deltaTime = 0.016f;  // Approx 60fps
        request.numSteps = 10;       // Simulate 10 steps per server call

        // Serialize the request
        std::ostringstream requestStream;
        boost::archive::binary_oarchive requestArchive(requestStream);
        requestArchive << request;
        std::string serializedRequest = requestStream.str();

        // Send data size first
        std::size_t requestSize = serializedRequest.size();
        asio::write(socket, asio::buffer(&requestSize, sizeof(requestSize)));

        // Send the actual data
        asio::write(socket, asio::buffer(serializedRequest));

        // Read response size
        std::size_t responseSize;
        asio::read(socket, asio::buffer(&responseSize, sizeof(responseSize)));

        // Read response data
        std::vector<char> responseBuffer(responseSize);
        asio::read(socket, asio::buffer(responseBuffer));

        // Deserialize the response
        serialization::SimulationResponse response;
        std::istringstream responseStream(std::string(responseBuffer.begin(), responseBuffer.end()));
        boost::archive::binary_iarchive responseArchive(responseStream);
        responseArchive >> response;

        // Process the response
        if (response.success) {
            // Update our application's dots with the simulation results
            updateDotsFromVehicles(dots, response.updatedVehicles);
            isLoading = false;
            return "Simulation completed successfully";
        }
        else {
            isLoading = false;
            return "Simulation error: " + response.errorMessage;
        }
    }
    catch (const std::exception& e) {
        isLoading = false;
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

// Helper functions for data conversion

// Create dummy lane cells for initial testing
// Later, you can replace this with actual lane cell data from your application
std::vector<serialization::LaneCell> createDummyLaneCells() {
    std::vector<serialization::LaneCell> laneCells;

    // Create a test road network with 100 cells
    for (int i = 0; i < 100; i++) {
        serialization::LaneCell cell;
        cell.next_in_lane = (i < 99) ? i + 1 : -1;
        cell.left_cell = (i % 2 == 0 && i < 50) ? i + 50 : -1;
        cell.right_cell = (i % 2 == 1 && i >= 50) ? i - 50 : -1;
        cell.length = 10.0f;
        laneCells.push_back(cell);
    }

    return laneCells;
}

// Convert from application dots to serializable vehicles
void convertDotsToVehicles(const std::vector<Dot>& appDots,
    std::vector<serialization::Vehicle>& vehicles) {
    vehicles.clear();
    vehicles.reserve(appDots.size());

    for (const auto& dot : appDots) {
        if (dot.active) {
            serialization::Vehicle vehicle;
            vehicle.s = dot.s;
            vehicle.v = dot.v;
            vehicle.segment = dot.segment;
            vehicle.active = dot.active;
            vehicle.pathIndex = dot.pathIndex;
            vehicles.push_back(vehicle);
        }
    }
}

// Update application dots with data from serializable vehicles
void updateDotsFromVehicles(std::vector<Dot>& appDots,
    const std::vector<serialization::Vehicle>& vehicles) {
    // Create a map of pathIndex+s to vehicle for efficient lookup
    std::map<std::pair<size_t, float>, const serialization::Vehicle*> vehicleMap;
    for (const auto& vehicle : vehicles) {
        vehicleMap[{vehicle.pathIndex, vehicle.s}] = &vehicle;
    }

    // Update each dot if a matching vehicle is found
    for (auto& dot : appDots) {
        auto key = std::make_pair(dot.pathIndex, dot.s);
        auto it = vehicleMap.find(key);

        if (it != vehicleMap.end()) {
            const auto& vehicle = *(it->second);
            dot.s = vehicle.s;
            dot.v = vehicle.v;
            dot.segment = vehicle.segment;
            dot.active = vehicle.active;
            // Note: pathIndex should remain the same
        }
    }
}