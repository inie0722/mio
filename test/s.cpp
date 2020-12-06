#include <iostream>
#include "mq/manager.hpp"

using namespace mio::mq;

int main(void)
{
    mio::mq::manager m(1);

    m.registered("call", 0, [&](std::pair<std::weak_ptr<manager::session>, std::unique_ptr<message>> msg){
        /*
        auto msg_res = std::make_shared<mio::mq::message>();
        msg_res->name = "call";
        msg_res->data= msg->data;
        msg_res->uuid = msg->uuid;
        msg_res->type = mio::mq::message_type::RESPONSE;

        ptr.lock()->response(msg_res);
        */
    });

    m.bind("ipv4:127.0.0.1:9999");

    mio::mq::message msg;
    msg.type = mio::mq::message_type::NOTICE;

    msg.name = "ipv4:127.0.0.1:9999";
    m.on_acceptor([&](const std::string &address, std::weak_ptr<mio::mq::manager::session> session){
        std::cout << address << std::endl;
        //std::cout << session->get_uuid() << std::endl;
        session.lock()->add_group("test");
    });

    while(1)
    {
        m.push("test", msg);
        sleep(3);
    }
    return 0;
}