#include "interprocess/acceptor.hpp"
#include <thread>

int main()
{
    boost::asio::io_context io_context;
    mio::interprocess::acceptor acc(io_context);
    acc.bind("127.0.0.1");

    std::thread th([&](){
        mio::interprocess::socket socket(io_context);
        socket.connect("127.0.0.1");

        char msg[10];
        socket.read(msg, 10);

        std::cout << msg << std::endl;
    });

    mio::interprocess::socket s(io_context);
    acc.accept(s);

    char msg[10] = "123456789";
    s.write(msg, 10);
    th.join();
    return 0;
}