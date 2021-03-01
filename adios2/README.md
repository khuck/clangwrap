# clangwrap
Clang-based wrapper for C++ libraries

This example is a little more complicated.  This example shows how to use the wrapper
generator with a complicated C++ library like ADIOS2.  This example assumes you have
an installation of ADIOS2, with MPI enabled.  To run this example, you still need the
clang compiler to build `tau_wrap++`, but you will also need a working MPI installation
and TAU will need to be configured a little differently.

```bash
git clone https://github.com/UO-OACISS/tau2.git
cd tau2
# Configure with MPI settings - no other settings necessary
./configure -mpi
make -j install
TAU_ARCH=`./utils/archfind`
export TAU_MAKEFILE=`pwd`/${TAU_ARCH}/lib/Makefile.tau-gnu-mpi
```

Modify the Makefile (if necessary - it shouldn't be necessary).

Build the example, first build `tau_wrap++` in the main directory.
Then you can build this example, which will use the modified ADIOS2 headers as input,
generate the wrapper source code, and then build the wrapper library.
To run the example, just modify your `mpirun` statement to use `tau_exec`,
like this:

```bash
mpirun -np 4 tau_exec -T mpi -loadlib=/path/to/libadios2_wrap.so ./build/gray-scott ./simulation/settings-files.json
```

...assuming your original mpirun statement was `mpirun -np 4 ./build/gray-scott ./simulation/settings-files.json`.
