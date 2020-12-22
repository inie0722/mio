
#include <string>
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>
#include "mio/log.hpp"

//#include <fmt/args.h>

int main()
{

    uint64_t file = __LOG.open_file("test.txt");


    while (1)
    {
        LOG(file, "test");
        sleep(1);
    }
    
    
    

    return 0;
}