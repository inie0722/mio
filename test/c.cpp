#include <iostream>
#include "mq/manager.hpp"

using namespace mio::mq;

int main(void)
{
    mio::mq::manager m(1);

    m.registered("test", 0, [&](std::pair<std::weak_ptr<manager::session>, std::unique_ptr<message>> msg){
    });

    m.on_connect([&](const std::string &address, const std::weak_ptr<mio::mq::manager::session>& session){
        std::cout << address << std::endl;
        //std::cout << session->get_uuid() << std::endl;
    });

    m.connect("ipv4:127.0.0.1:9999");
    
    while(1)
    {
        sleep(1);
    }
        
    return 0;
}