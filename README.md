# clangwrap
Clang-based wrapper for C++ libraries

This wrapper is based on the `tau_wrap` utility that comes with TAU.
The only pre-requisite is a working clang++ and a working g++ compiler.

To demonstrate this simple example, do the following:

Configure and build TAU: https://github.com/UO-OACISS/tau2

```bash
git clone https://github.com/UO-OACISS/tau2.git
cd tau2
# Configure with simple settings - no complicated settings necessary
./configure
make -j install
# Set the TAU makefile and add TAU to your path
TAU_ARCH=`./utils/archfind`
export TAU_MAKEFILE=`pwd`/${TAU_ARCH}/lib/Makefile.tau-gnu
export PATH=`pwd`/${TAU_ARCH}/bin:${PATH}
```

Modify the Makefile (if necessary - it shouldn't be necessary).

Build the example, which will build the `libsecret.so` library and the `app` executable,
then build the `tau_wrap++` wrapper utility, run it, then build the generated wrapper library.
To run the example, just type `make test`.  The output should look something like this:

```bash
[khuck@cyclops clangwrap]$ make
make -C src
make[1]: Entering directory `/storage/users/khuck/src/clangwrap/src'
clang++ -c tau_wrap++.cpp -o tau_wrap++.o -fPIC -I. -g -O3 -std=c++11 -Wall -Werror
clang++ -o tau_wrap++ tau_wrap++.o -lclang
make[1]: Leaving directory `/storage/users/khuck/src/clangwrap/src'
make -C simple
make[1]: Entering directory `/storage/users/khuck/src/clangwrap/simple'
g++ -fPIC -I. -g -O3 -std=c++11 -Wall -Werror -c app.cpp
g++ -fPIC -I. -g -O3 -std=c++11 -Wall -Werror -c secret.cpp
g++ -shared -g -O3 -o libsecret.so secret.o
../src/tau_wrap++ secret.h -w libsecret.so -n secret -c config.json
Header file to be parsed: secret.h
Library to be wrapped: libsecret.so
Namespace to be wrapped: secret
Configuration file to be used: config.json
Writing the library symbol log to cursor.log
Parsing symbols in namespace secret from library libsecret.so
Writing the parser log to cursor.log
..
New Class: Variable
..........
Found Class: secret::Secret.
Found Class: secret::Secret::InnerClass......................
Wrote library wrapper to wr.cpp
g++ -m64 		   	  	    -fPIC -I. -g -O3 -std=c++11 -Wall -Werror -I/home/users/khuck/src/tau2/include -DPROFILING_ON                        -DTAU_GNU -DTAU_DOT_H_LESS_HEADERS                     -DTAU_LINUX_TIMERS                                 -DTAU_LARGEFILE -D_LARGEFILE64_SOURCE                    -DTAU_BFD   -DHAVE_GNU_DEMANGLE   -DHAVE_TR1_HASH_MAP    -DTAU_SS_ALLOC_SUPPORT  -DEBS_CLOCK_RES=1  -DTAU_STRSIGNAL_OK    -DTAU_TRACK_LD_LOADER                                -I/usr/local/packages/otf2/2.2_python3.8.0/include -DTAU_OTF2        -c wr.cpp -o secret_wrap.o
g++ -m64 		   	  	    -shared -g -O3 -o libsecret_wrap.so secret_wrap.o -L/home/users/khuck/src/tau2/ibm64linux/lib -Wl,-rpath,/home/users/khuck/src/tau2/ibm64linux/lib -lTAUsh-gnu -L/usr/local/packages/binutils/2.34/lib -L/usr/local/packages/binutils/2.34/lib64 -Wl,-rpath,/usr/local/packages/binutils/2.34/lib -Wl,-rpath,/usr/local/packages/binutils/2.34/lib64 -lbfd -Wl,--export-dynamic -lrt -L/usr/local/packages/otf2/2.2_python3.8.0/lib -lotf2 -lotf2 -Wl,-rpath,/usr/local/packages/otf2/2.2_python3.8.0/lib -Wl,-rpath,/home/users/khuck/src/tau2/ibm64linux/lib/shared-gnu -ldl
g++ -o app app.o -L/storage/users/khuck/src/clangwrap/simple -Wl,-rpath,/storage/users/khuck/src/clangwrap/simple -lsecret
make[1]: Leaving directory `/storage/users/khuck/src/clangwrap/simple'
[khuck@cyclops clangwrap]$ make test
make -C src test
make[1]: Entering directory `/storage/users/khuck/src/clangwrap/src'
make[1]: Nothing to be done for `test'.
make[1]: Leaving directory `/storage/users/khuck/src/clangwrap/src'
make -C simple test
make[1]: Entering directory `/storage/users/khuck/src/clangwrap/simple'
rm -f profile.*
tau_exec -T serial -loadlib=./libsecret_wrap.so ./app
Inside Secret
ahh!
Inside foo1: x = 1
Inside foo2: b = 4, c = 1
Inside foo3: Hello World!
Inside Data
9
Inside foo4: Hello 9!
Inside foo4: Hello 2.3!
Inside foo4: Hello 1.1!
anotherTemplate 1
Inside ~Secret
pprof
Reading Profile files in profile.*

NODE 0;CONTEXT 0;THREAD 0:
---------------------------------------------------------------------------------------
%Time    Exclusive    Inclusive       #Call      #Subrs  Inclusive Name
              msec   total msec                          usec/call
---------------------------------------------------------------------------------------
100.0        0.267        0.433           1          13        433 .TAU application
 16.9        0.073        0.073           1           0         73 [WRAPPER] secret::Secret::Secret()
  6.9         0.03         0.03           1           0         30 [WRAPPER] void secret::Secret::foo4<double, void>(Variable<double, void> a)
  3.0        0.013        0.013           1           0         13 [WRAPPER] int secret::Secret::foo1(secret::Dim a)
  1.6        0.007        0.007           1           0          7 [WRAPPER] secret::Secret::~Secret()
  1.6        0.007        0.007           1           0          7 [WRAPPER] void secret::Secret::foo3(std::string name)
  1.6        0.007        0.007           1           0          7 [WRAPPER] void secret::Secret::foo4<float, void>(Variable<float, void> a)
  1.4        0.006        0.006           1           0          6 [WRAPPER] void secret::Secret::foo2(secret::Dim b, secret::Dim c)
  1.2        0.005        0.005           1           0          5 [WRAPPER] int secret::Variable<int, void>::Data() const
  1.2        0.005        0.005           1           0          5 [WRAPPER] std::string secret::Secret::getMessage() const
  1.2        0.005        0.005           1           0          5 [WRAPPER] void secret::Secret::foo4<int, void>(Variable<int, void> a)
  1.2        0.005        0.005           1           0          5 [WRAPPER] void secret::Variable<float, void>::anotherTemplate<int>(int a)
  0.7        0.003        0.003           1           0          3 [WRAPPER] secret::Secret::InnerClass::InnerClass()
  0.0            0            0           1           0          0 [WRAPPER] secret::Secret::InnerClass::~InnerClass()
```
