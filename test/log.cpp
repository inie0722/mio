#include "mio/log.hpp"
#include <thread>
#include <chrono>

using namespace std;

int main()
{
    mio::log_service log("log.shm", 65536 * 70);
    log.run(std::chrono::milliseconds(0));
    return 0;
}