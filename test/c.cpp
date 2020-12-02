#include <iostream>
#include "mq/req_rep.hpp"

void print_exception(const std::exception& e, int level =  0)
{
    std::cerr << std::string(level, ' ') << "exception: " << e.what() << '\n';
    try {
        std::rethrow_if_nested(e);
    } catch(const std::exception& e) {
        print_exception(e, level+1);
    } catch(...) {}
}

int main(void)
{
    boost::asio::io_context io_context;

    mio::mq::req_rep::req req_(io_context);
    req_.connect("ipv4:127.0.0.1:9999");

    std::thread th([&](){

        while (1)
        {
            try
            {
                io_context.run();
            }
            catch(const std::exception& e)
            {
                print_exception(e);
            }
        }
            usleep(10000);
    });
    

    while (1)
    {
        
        mio::mq::req_rep::request req;

        char msg[] = "123456789.0";
        req.data.resize(sizeof(msg));
        memcpy(&req.data[0], msg, req.data.size());
        req.uuid = boost::uuids::random_generator()();
        auto fu = req_.write(std::move(req));

        std::cout << &fu.get().data[0] << std::endl;
        usleep(10000);
    }
    
    th.join();
    return 0;
}