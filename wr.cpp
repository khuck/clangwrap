
#include <Profile/Profiler.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string>
#include <iostream>
#include <complex>

//#define MARKER printf("*** %s %s: %d\n", __func__, __FILE__, __LINE__)
#define MARKER
#include "secret.h"

void * load_handle(void) {
    MARKER;
    const char * tau_orig_libname = "libsecret.so";
    static void *handle = (void *) dlopen(tau_orig_libname, RTLD_NOW);
    if (handle == NULL) {
        std::cerr << "Error opening library "
                  << tau_orig_libname
                  << " in dlopen call"
                  << std::endl;
    }
    return handle;
}

template<class T> T* load_symbol(char const* name) {
    MARKER;
    static auto handle = load_handle();
    void * tmp = dlsym(handle,name);
    if (tmp == NULL) {
        std::cerr << "Error obtaining symbol "
                  << name
                  << " from library"
                  << std::endl;
    }
    return reinterpret_cast<T*>(tmp);
}

size_t& getDepth() {
    thread_local static size_t depth{0};
    return depth;
}

class DepthCounter {
public:
    DepthCounter() {
        getDepth()++;
    }
    ~DepthCounter() {
        getDepth()--;
    }
    bool timeit() {
        if (getDepth() == 1) {
            return true;
        }
        return false;
    }
};

#define WRAPPER(name) \
  static void *tauFI = 0; \
  if (tauFI == 0) tauCreateFI(&tauFI, name, "", (TauGroup_t)TAU_USER, "SECRET"); \
  Tau_Profile_Wrapper tauFProf(tauFI);

namespace secret {

/* Target: std::string secret::Secret::InnerClass::getMessage() const*/
/* Found:  secret::Secret::InnerClass::getMessage[abi:cxx11]() const*/
/* Score:  11*/
/********************************************************************************
 Secret::InnerClass::getMessage
 ********************************************************************************/

std::string Secret::InnerClass::getMessage() const {
    MARKER;
    const char * mangled = "_ZNK6secret6Secret10InnerClass10getMessageB5cxx11Ev";
    const char * timer_name = "*** std::string secret::Secret::InnerClass::getMessage() const***";
    using f_t = std::string(const void*);
    static f_t* f{load_symbol<f_t>(mangled)};
    DepthCounter depth;
    if (depth.timeit()) {
        WRAPPER(timer_name);
        std::string retval = f(this);
        return std::move(retval);
    } else {
        std::string retval = f(this);
        return std::move(retval);
    }
}

/********************************************************************************
 Secret::foo1
 ********************************************************************************/

int Secret::foo1(secret::Dim a) {
    MARKER;
    const char * mangled = "_ZN6secret6Secret4foo1Ei";
    const char * timer_name = "*** int secret::Secret::foo1(secret::Dim a)***";
    using f_t = int(void*,secret::Dim);
    static f_t* f{load_symbol<f_t>(mangled)};
    DepthCounter depth;
    if (depth.timeit()) {
        WRAPPER(timer_name);
        int retval = f(this, a);
        return std::move(retval);
    } else {
        int retval = f(this, a);
        return std::move(retval);
    }
}

/********************************************************************************
 Secret::foo3
 ********************************************************************************/

void Secret::foo3(std::string name) {
    MARKER;
    const char * mangled = "_ZN6secret6Secret4foo3ENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE";
    const char * timer_name = "*** void secret::Secret::foo3(std::string name)***";
    using f_t = void(void*,std::string);
    static f_t* f{load_symbol<f_t>(mangled)};
    DepthCounter depth;
    if (depth.timeit()) {
        WRAPPER(timer_name);
        f(this, name);
    } else {
        f(this, name);
    }
}

/********************************************************************************
 Secret::foo2
 ********************************************************************************/

void Secret::foo2(secret::Dim b, secret::Dim c) {
    MARKER;
    const char * mangled = "_ZN6secret6Secret4foo2Eii";
    const char * timer_name = "*** void secret::Secret::foo2(secret::Dim b, secret::Dim c)***";
    using f_t = void(secret::Dim,secret::Dim);
    static f_t* f{load_symbol<f_t>(mangled)};
    DepthCounter depth;
    if (depth.timeit()) {
        WRAPPER(timer_name);
        f(b, c);
    } else {
        f(b, c);
    }
}

/* Target: std::string secret::Secret::getMessage() const*/
/* Found:  secret::Secret::getMessage[abi:cxx11]() const*/
/* Score:  11*/
/********************************************************************************
 Secret::getMessage
 ********************************************************************************/

std::string Secret::getMessage() const {
    MARKER;
    const char * mangled = "_ZNK6secret6Secret10getMessageB5cxx11Ev";
    const char * timer_name = "*** std::string secret::Secret::getMessage() const***";
    using f_t = std::string(const void*);
    static f_t* f{load_symbol<f_t>(mangled)};
    DepthCounter depth;
    if (depth.timeit()) {
        WRAPPER(timer_name);
        std::string retval = f(this);
        return std::move(retval);
    } else {
        std::string retval = f(this);
        return std::move(retval);
    }
}

} // end namespace secret

