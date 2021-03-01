include ${TAU_MAKEFILE}
TAUCXXFLAGS=$(shell tau_cxx.sh -tau:showincludes) $(TAU_DEFS)
TAUCXXLIBS=$(shell tau_cxx.sh -tau:showsharedlibs)

CXXFLAGS=-I./include -DADIOS2_USE_MPI -DMPICH_SKIP_MPICXX -DOMPI_SKIP_MPICXX -Dadios2_cxx11_EXPORTS -g -O3 -fPIC -std=c++11
LDFLAGS = -shared -g -O3

libadios2_wrap.so: adios2_wrap.o
	$(TAU_CXX) $(LDFLAGS) -o $@ $< $(TAUCXXFLIBS) -ldl $(LDFLAGS)

adios2_wrap.o: wr.cpp
	$(TAU_CXX) $(CXXFLAGS) $(TAUCXXFLAGS) -c $< -o $@

wr.cpp: ../src/tau_wrap++ config.json
	../src/tau_wrap++ $(ADIOS2_ROOT)/include/adios2.h -w $(ADIOS2_ROOT)/lib64/libadios2_cxx11.so -n adios2 -c config.json
clean:
	/bin/rm -f adios2_wrap.o libadios2_wrap.so wr.cpp cursor.log symbol.log

