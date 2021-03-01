#include <stdio.h>
#include <secret.h>
#include <string>

using namespace secret;

int main(int argc, char **argv)
{
    for (int i = 1 ; i < 2 ; i++) {
        Secret object;
        std::cout << object.getMessage() << std::endl;
        object.foo1(i);
        Secret::foo2(4, i);
        std::string name{"World"};
        object.foo3(name);
        Variable<int, void> nine;
        nine.t = 9 * i;
        std::cout << nine.Data() << std::endl;
        object.foo4(nine);
        Variable<double, void> two_point_three;
        two_point_three.t = 2.3 * i;
        object.foo4(two_point_three);
        Variable<float, void> one_point_one;
        one_point_one.t = 1.1 * i;
        object.foo4(one_point_one);
        one_point_one.anotherTemplate(i);
    }
    return 0;
}
