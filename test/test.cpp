#include "interprocess/acceptor.hpp"
#include <thread>

int main()
{
    boost::asio::io_context io_context;

    boost::asio::spawn(io_context, [&](boost::asio::yield_context yield){
        mio::interprocess::acceptor acc(io_context);
        acc.bind("127.0.0.1");

        mio::interprocess::socket s(io_context);
        acc.async_accept(s, yield);

            char msg[10] = "123456789";
    s.async_write(msg, 10, yield);
    });


    boost::asio::spawn(io_context, [&](boost::asio::yield_context yield){
        mio::interprocess::socket socket(io_context);
        socket.async_connect("127.0.0.1", yield);

                char msg[10];
        socket.async_read(msg, 10, yield);

        std::cout << msg << std::endl;
        socket.async_read(msg, 1, yield);

        });
        /*
    std::thread th([&](){
        mio::interprocess::socket socket(io_context);
        socket.connect("127.0.0.1");

        char msg[10];
        socket.read(msg, 10);

        std::cout << msg << std::endl;
        socket.read(msg, 1);
    });

    */
   io_context.run();
    return 0;
}