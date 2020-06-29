#include "network/network.hpp"
#include <thread>
#include <atomic>
using namespace libcpp;
using namespace std;

std::atomic<uint64_t> cc = 0;

void test(void *socket, void *arg, uint32_t arg_size)
{
    cc.fetch_add(1);

}

int main(void)
{
    network::client c(20);

    for (size_t i = 0; i < 500; i++)
    {
        auto eve = c.make_event_client("127.0.0.1", 9999);
        eve->subscription("test", test);
    }
    while (1)
    {
        cout << cc.load() << endl;
    }
    
    return 0;
}