#include <iostream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "mq/manager.hpp"
#include "interprocess/pipe.hpp"

#include "mq/manager.hpp"

using namespace mio::mq;

struct protocol
{
    using socket_t = mio::interprocess::pipe::socket;
    using acceptor_t = mio::interprocess::pipe::acceptor;
};

int main(void)
{
    mio::mq::manager m(1);

    m.registered("test", 0, [&](std::pair<std::weak_ptr<manager::session>, std::shared_ptr<message>> msg){
        std::cout << &msg.second->name[0] << std::endl;

        auto msg_req = std::make_shared<mio::mq::message>();
        msg_req->name = "call";
        msg_req->data= msg.second->data;
        msg_req->uuid = boost::uuids::random_generator()();
        msg_req->type = mio::mq::message_type::REQUEST;

        auto f = msg.first.lock()->request(msg_req);
        f.get();
        std::cout << "请求成功" << std::endl;
    });

    m.on_connect([&](const std::string &address, const std::weak_ptr<mio::mq::manager::session>& session){
        std::cout << address << std::endl;
        //std::cout << session->get_uuid() << std::endl;
    });

    m.connect<protocol>("ipv4:127.0.0.1:9999");
    
    while(1)
    {
        sleep(1);
    }
        
    return 0;
}