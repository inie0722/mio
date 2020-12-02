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

    mio::mq::req_rep::rep rep(io_context);
    rep.bind("ipv4:127.0.0.1:9999");

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

        req.uuid = boost::uuids::random_generator()();
        auto c = rep.read(req);

        std::cout << &req.data[0] << std::endl;

        mio::mq::req_rep::response res;

        res.uuid = req.uuid;

        res.data = req.data;
        rep.write(c, std::move(res));
        usleep(10000);
    }
    
    th.join();
    return 0;
}