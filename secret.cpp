#include <iostream>
#include <unistd.h>
#include "secret.h"

namespace secret {

Secret::InnerClass::InnerClass() : m_message("ahh!") {}

Secret::InnerClass::~InnerClass() { /* do nothing */ }

std::string Secret::InnerClass::getMessage() const {
    return m_message;
}

template <typename T, typename R>
T Variable<T, R>::Data() const {
    std::cout << "Inside " << __func__ << std::endl;
    return t;
}

template <typename T, typename R>
template <typename V>
void Variable<T,R>::anotherTemplate(V a) {
    std::cout << __func__ << " " << a << std::endl;;
}

Secret::Secret() {
    // constructor
    std::cout << "Inside " << __func__ << std::endl;
}

Secret::~Secret() {
    // destructor
    std::cout << "Inside " << __func__ << std::endl;
}

int Secret::foo1 (int x)
{
    std::cout << "Inside " << __func__ << ": x = " << x << std::endl;
    return x+1;
}

void Secret::foo2 (int b, int c)
{
    std::cout << "Inside " << __func__ << ": b = " << b << ", c = " << c << std::endl;
    return;
}

void Secret::foo3 (std::string name)
{
    std::cout << "Inside " << __func__ << ": Hello " << name << "!" << std::endl;
    return;
}

template <typename T, typename R>
void Secret::foo4 (Variable<T,R> a) {
    std::cout << "Inside " << __func__ << ": Hello " << a.t << "!" << std::endl;
    return;
}

template int Variable<int,void>::Data() const;
template double Variable<double,void>::Data() const;
template float Variable<float,void>::Data() const;
template void Variable<float,void>::anotherTemplate<int>(int);
template void Secret::foo4<int,void>(Variable<int,void>);
template void Secret::foo4<double,void>(Variable<double,void>);
template void Secret::foo4<float,void>(Variable<float,void>);

std::string Secret::getMessage() const {
    return inner.getMessage();
}
}
