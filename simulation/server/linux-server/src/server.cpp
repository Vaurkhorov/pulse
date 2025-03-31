//#include"../../headers/server.hpp"
//using namespace boost::system;
//
//class SimulationServer {
//public:
//    SimulationServer(asio::io_context& io, unsigned short port)
//        : acceptor_(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
//    {
//        start_accept();
//    }
//
//private:
//    void start_accept() {
//        // Create new socket for incoming connection
//        auto socket = std::make_shared<asio::ip::tcp::socket>(acceptor_.get_executor());
//
//        // Async accept with lambda handler
//        acceptor_.async_accept(*socket, [this, socket](error_code ec) {
//            if (!ec) {
//                // Handle the new connection
//                handle_client(socket);
//            }
//            // Continue accepting new connections
//            start_accept();
//            });
//    }
//
//    void handle_client(std::shared_ptr<asio::ip::tcp::socket> socket) {
//        // Create read buffer
//        auto buffer = std::make_shared<std::vector<char>>(1024);
//
//        // Async read with lambda handler
//        socket->async_read_some(asio::buffer(*buffer),
//            [this, socket, buffer](error_code ec, size_t bytes_read) {
//                if (!ec) {
//                    // Deserialize received data
//                    std::string data(buffer->begin(), buffer->begin() + bytes_read);
//                    std::istringstream iss(data);
//                    boost::archive::binary_iarchive ia(iss);
//
//                    SimData received_data;
//                    ia >> received_data;
//
//                    // Process simulation data (CUDA processing would go here)
//                    std::cout << "Received " << received_data.vehicles.size()
//                        << " vehicles\n";
//
//                    // Create response data
//                    SimData response;
//                    response.heatmap = { 0.1f, 0.5f, 1.0f };  // Dummy heatmap
//
//                    // Serialize and send response
//                    std::ostringstream oss;
//                    boost::archive::binary_oarchive oa(oss);
//                    oa << response;
//
//                    std::string response_str = oss.str();
//                    asio::async_write(*socket, asio::buffer(response_str),
//                        [socket](error_code ec, size_t) {});
//                }
//            });
//    }
//
//    asio::ip::tcp::acceptor acceptor_;
//};
//
////int main() {
////    try {
////        asio::io_context io;
////        SimulationServer server(io, 12345);  // Listen on port 12345
////        std::cout << "Server started. Waiting for connections...\n";
////        io.run();
////    }
////    catch (std::exception& e) {
////        std::cerr << "Server error: " << e.what() << "\n";
////    }
////    return 0;
////}