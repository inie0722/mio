#include "log.hpp"

#include <boost/format.hpp>
#define LOG libcpp::log<boost::format>::get_log()

int main(void)
{
    std::thread write([](){
        LOG.run(std::chrono::milliseconds(10));
    });
    using namespace std::chrono;

    for (size_t i = 0; i < 1000; i++)
    {
        auto time = nanoseconds(duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count());
        LOG("test.txt", "%1% 123456789 %2% %3%\n", time, " test", i);
    }
    
    std::cout << "complete" << std::endl;
    write.join();
    return 0;
}