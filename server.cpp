#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;

void session(tcp::socket socket) {
    try {
        char data[1024];
        char databuf[200];
        for (;;) {
            std::size_t length = socket.read_some(boost::asio::buffer(databuf));
            snprintf(data, 1024,"Slmsslikmchk, brot. You wrot %s", databuf);
            boost::asio::write(socket, boost::asio::buffer(data, strlen(data)));
            std::cout << data << std::endl;
        }
    } catch (std::exception& e) {
        std::cerr << "Exxxxxxception: " << e.what() << "\n";
    }
}

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 53861));

        for (;;) {
            tcp::socket socket(io_context);
            acceptor.accept(socket);
            std::thread(session, std::move(socket)).detach();
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}