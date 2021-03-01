#include ${TAU_MAKEFILE}
include /home/users/khuck/src/tau2/ibm64linux/lib/Makefile.tau-gnu-mpi
CXX=mpicxx
#CXX=g++
#CFLAGS=-I$(HOME)/src/ADIOS2.ornl/source -I./internal_include -I./include $(TAU_DEFS) $(TAU_INCLUDE) $(TAU_MPI_INCLUDE) -DADIOS2_USE_MPI -DMPICH_SKIP_MPICXX -DOMPI_SKIP_MPICXX -Dadios2_cxx11_EXPORTS -g -fPIC -std=gnu++11 #-Wabi-tag -D_GLIBCXX_USE_CXX11_ABI=0
CFLAGS=-I./include $(TAU_DEFS) $(TAU_INCLUDE) $(TAU_MPI_INCLUDE) -DADIOS2_USE_MPI -DMPICH_SKIP_MPICXX -DOMPI_SKIP_MPICXX -Dadios2_cxx11_EXPORTS -g -fPIC -std=gnu++11 #-Wabi-tag -D_GLIBCXX_USE_CXX11_ABI=0
EXTRA_FLAGS=-ggdb -O0

libadios2_wrap.so: adios2_wrap.o
	$(CXX) $(TAU_SHFLAGS) $@ $< $(TAU_SHLIBS) -ldl -Wabi-tag

adios2_wrap.o: wr.cpp Makefile
	$(CXX) $(CFLAGS) $(EXTRA_FLAGS) -c $< -o $@

wr.cpp: ../../utils/tau_wrap++ config.json Makefile
	../../utils/tau_wrap++ /usr/local/packages/ADIOS2/2021.02.05-Debug/mpi/include/adios2.h -w /usr/local/packages/ADIOS2/2021.02.05-Debug/mpi/lib64/libadios2_cxx11.so -n adios2 -c config.json
clean:
	/bin/rm -f adios2_wrap.o libadios2_wrap.so
	#/bin/rm -f adios2_wrap.o libadios2_wrap.so wr.cpp

