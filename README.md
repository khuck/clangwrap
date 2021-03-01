# clangwrap
Clang-based wrapper for C++ libraries

This wrapper is based on the `tau_wrap` utility that comes with TAU.
The only pre-requisite is a working Clang++ compiler.

To demonstrate this simple example, do the following:

Configure and build TAU: https://github.com/UO-OACISS/tau2

```bash
git clone https://github.com/UO-OACISS/tau2.git
cd tau2
# Configure with simple settings - no complicated settings necessary
./configure
make -j install
TAU_ARCH=`./utils/archfind`
export TAU_MAKEFILE=`pwd`/${TAU_ARCH}/lib/Makefile.tau-gnu
```

Modify the Makefile (if necessary).

