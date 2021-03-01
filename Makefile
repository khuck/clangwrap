include ${TAU_MAKEFILE}
TAUCXXFLAGS=$(shell tau_cxx.sh -tau:showincludes) $(TAU_DEFS)
TAUCXXLIBS=$(shell tau_cxx.sh -tau:showsharedlibs)
CXX=g++

PWD=$(shell pwd)
MYCXXFLAGS=-fPIC -I. -g -O3 -std=c++11 -Wall -Werror
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

# tau_wrap++ has to be compiled and linked with clang++!
tau_wrap++: tau_wrap++.o
	clang++ -o $@ $< -lclang

# tau_wrap++ has to be compiled and linked with clang++!
tau_wrap++.o: tau_wrap++.cpp
	clang++ -c $< -o $@ $(MYCXXFLAGS)

wr.cpp: tau_wrap++ config.json secret.h libsecret.so
	tau_wrap++ secret.h -w libsecret.so -n secret -c config.json

clean:
	/bin/rm -f app.o app *.so *.o profile.* *.log tau_wrap++

test:
	rm -f profile.*
	tau_exec -T serial -loadlib=./libsecret_wrap.so ./app
	pprof
