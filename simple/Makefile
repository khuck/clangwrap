# ****************************************************************************
# **  TAU Portable Profiling Package                                        **
# **  http://tau.uoregon.edu                                                **
# ****************************************************************************
# **  Copyright 2021                                                        **
# **  Department of Computer and Information Science, University of Oregon  **
# ****************************************************************************

include ${TAU_MAKEFILE}
TAUCXXFLAGS=$(shell tau_cxx.sh -tau:showincludes) $(TAU_DEFS)
TAUCXXLIBS=$(shell tau_cxx.sh -tau:showsharedlibs)
CXX=mpicxx

PWD=$(shell pwd)
MYCXXFLAGS=-fPIC -I. -g -O3 -std=c++11 -Wall
LIBS = -L$(PWD) -Wl,-rpath,$(PWD) -lsecret
LDFLAGS = -shared -g -O3

app: app.o libsecret.so libsecret_wrap.so
	$(CXX) -o app app.o $(LIBS)

libsecret.so: secret.o secret.h
	$(CXX) $(LDFLAGS) -o $@ $<

secret.o: secret.cpp secret.h
	$(CXX) $(MYCXXFLAGS) -c $<

app.o: app.cpp secret.h
	$(CXX) $(MYCXXFLAGS) -c $<

libsecret_wrap.so: secret_wrap.o
	$(TAU_CXX) $(LDFLAGS) -o $@ $< $(TAUCXXLIBS) -ldl

secret_wrap.o: wr.cpp
	$(TAU_CXX) $(MYCXXFLAGS) $(TAUCXXFLAGS) -c $< -o $@

wr.cpp: ../src/tau_wrap++ config.json secret.h libsecret.so
	../src/tau_wrap++ secret.h -w libsecret.so -n secret -c config.json

clean:
	/bin/rm -f app.o app *.so *.o profile.* *.log wr.cpp

test: app ../src/tau_wrap++ libsecret_wrap.so
	rm -rf profile.* skel
	tau_exec -T serial -loadlib=./libsecret_wrap.so -skel ./app
	pprof
	#cat skel/rank00000.trace
