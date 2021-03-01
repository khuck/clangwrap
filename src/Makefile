PWD=$(shell pwd)
MYCXXFLAGS=-fPIC -I. -g -O3 -std=c++11 -Wall -Werror
LDFLAGS = -shared -g -O3

all: tau_wrap++

test: all

# tau_wrap++ has to be compiled and linked with clang++!
tau_wrap++: tau_wrap++.o
	clang++ -o $@ $< -lclang

# tau_wrap++ has to be compiled and linked with clang++!
tau_wrap++.o: tau_wrap++.cpp
	clang++ -c $< -o $@ $(MYCXXFLAGS)

clean:
	/bin/rm -f tau_wrap++.o tau_wrap++

.PHONY: test all