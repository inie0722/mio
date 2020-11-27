#include <atomic>
#include <boost/asio.hpp>
#include <iostream>

#include "network/tcp.hpp"
#include "interprocess/pipe.hpp"

#include "mq/mq.hpp"
#include "mq/rpc.hpp"


struct Protocol
{
    
    typedef mio::interprocess::pipe::acceptor acceptor_t;
    typedef mio::interprocess::pipe::socket socket_t;
    
    /*
    typedef mio::network::tcp::acceptor acceptor_t;
    typedef mio::network::tcp::socket socket_t;
    */
};

int main(void)
{
    mio::mq::service<Protocol, mio::mq::rpc::service> s(1);

    s.run("ipv4:0.0.0.0:9988");

    s.on_accept([](void *socket, const std::vector<char> & msg)->bool{
        std::cout << &msg[0] << std::endl;
        return true;
    });

    auto aa = s.get_service<0>();

    sleep(1);
    mio::mq::client<Protocol, mio::mq::rpc::client> c(1);
    c.on_connect([](void *socket, std::vector<char> & arg){

        char msg[] = "123456789.0";
        arg.resize(sizeof(msg));

        memcpy(&arg[0], msg, sizeof(msg));

    });

    auto rpc = c.make_client<0>("ipv4:0.0.0.0:9988");
    while(1)
        sleep(10);
    return 0;
}