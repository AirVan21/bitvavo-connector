#include <iostream>
#include <boost/asio.hpp>

int main() {
    std::cout << "Welcome to the Bitvavo Connector!" << std::endl;

    boost::asio::io_context io_context;
    io_context.run();

    return 0;
}