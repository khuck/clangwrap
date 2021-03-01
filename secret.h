#pragma once

#include <string>
#include <iostream>
#include <vector>

namespace secret {

using Dim = int;

template<typename T, typename R>
class Variable {
public:
    T t;
    T Data() const;
    template <typename V>
    void anotherTemplate (V a);
};

class Secret {

class InnerClass {
    private:
        std::string m_message;
    public:
        InnerClass();
        ~InnerClass();
        std::string getMessage() const;
};

private:
    InnerClass inner;
public:
    Secret();
    ~Secret();
    int foo1 (Dim a);
    void foo3 (std::string name);
    static void foo2 (Dim b, Dim c);
    template <typename T, typename R>
        void foo4 (Variable<T, R> a);
    std::string getMessage() const;
};

#if 0
extern template void Secret::foo4<int>(Variable<int> a);
extern template void Secret::foo4<float>(Variable<float> a);
extern template void Secret::foo4<double>(Variable<double> a);
#endif

}
