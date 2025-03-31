/*All requested packages are currently installed.
1 > Total install time : 3.55 ms
1 > assimp provides CMake targets :
1 >
1 > # this is heuristically generated, and may not be correct
1 > find_package(assimp CONFIG REQUIRED)
1 > target_link_libraries(main PRIVATE assimp::assimp)
1 >
1 > assimp provides pkg - config modules :
1 >
1 > # Import various well - known 3D model formats in an uniform manner.
1 > assimp
1 >
1 > glad provides CMake targets :
1 >
1 > # this is heuristically generated, and may not be correct
1 > find_package(glad CONFIG REQUIRED)
1 > target_link_libraries(main PRIVATE glad::glad)
1 >
1 > glfw3 provides CMake targets :
1 >
1 > # this is heuristically generated, and may not be correct
1 > find_package(glfw3 CONFIG REQUIRED)
1 > target_link_libraries(main PRIVATE glfw)
1 >
1 > glfw3 provides pkg - config modules :
1 >
1 > # A multi - platform library for OpenGL, window and input
1 > glfw3
1 >
1 > The package glm provides CMake targets :
1 >
1 > find_package(glm CONFIG REQUIRED)
1 > target_link_libraries(main PRIVATE glm::glm)
1 >
1 > # Or use the header - only version
1 > find_package(glm CONFIG REQUIRED)
1 > target_link_libraries(main PRIVATE glm::glm - header - only)
1 >
1 > imgui provides CMake targets :
1 >
1 > # this is heuristically generated, and may not be correct
1 > find_package(imgui CONFIG REQUIRED)
1 > target_link_libraries(main PRIVATE imgui::imgui)
1 >
1 > libosmium is header - only and can be used from CMake via :
1 >
1 > find_path(OSMIUM_INCLUDE_DIRS "osmium/version.hpp")
1 > target_include_directories(main PRIVATE ${ OSMIUM_INCLUDE_DIRS })
1 >
1 > soil2 provides CMake targets :
1 >
1 > # this is heuristically generated, and may not be correct
1 > find_package(soil2 CONFIG REQUIRED)
1 > target_link_libraries(main PRIVATE soil2)
1 >
1 > simulation.vcxproj->C:\Users\Akhil\source\repos\pulse\x64\Debug\simulation.exe*/

#include "../../headers/server.hpp"

Client_Server::Client_Server(const std::string& server_ip, unsigned short PORT) : context(), socket(context), endpoint(asio::ip::make_address(server_ip), PORT) {};

std::string Client_Server::RunCUDAcode(std::string& sendData, bool& isLoading) {
	connect();
	if (!socket.is_open()) {
		throw std::runtime_error("Socket is not open after connection attempt.");
	}
	sendingData(isLoading);
	return RecievingData();
}

void Client_Server::connect() {
	try {
		if (!socket.is_open()) {
			socket = asio::ip::tcp::socket(context); // Recreate if closed
		}

		std::cout << "Connecting to server..." << std::endl;
		socket.connect(endpoint);
		std::cout << "Connected successfully!" << std::endl;
	}
	catch (const boost::system::system_error& ex) {
		std::cerr << "Connection failed: " << ex.what() << std::endl;
		throw;
	}
}

// sendingData
void Client_Server::sendingData(bool& isLoading) {
	if (!socket.is_open()) {
		std::cout << "Error::SOCKET NOT OPEN" << std::endl;
		throw std::runtime_error("Socket is closed!");
	}
	std::string sendingStr = "Hello";

	isLoading = true;
	// sending data:
	asio::write(socket, asio::buffer(sendingStr));
	isLoading = false;
	std::cout << "Sent the data: " << sendingStr << std::endl;
}

// RecieveingData
std::string Client_Server::RecievingData() {
	// receiving the data into the buffer
	asio::streambuf recv_buffer;
	asio::read_until(socket, recv_buffer, '\0'); // read until null terminates

	// deserialising the data:
	std::istream recv_ss(&recv_buffer);
	std::string recv_string((std::istreambuf_iterator<char>(recv_ss)), std::istreambuf_iterator<char>());
	std::cout << "RecievedData: " << recv_string << std::endl;
	return recv_string;
}



