#include <iostream>
#include <functional>
#include <variant>

void test(const std::variant<std::function<void()>, int> & a)
{

}

int main(void)
{
    test([](){
        
    });
    return 0;
}