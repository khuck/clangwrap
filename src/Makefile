# ****************************************************************************
# **  TAU Portable Profiling Package                                        **
# **  http://tau.uoregon.edu                                                **
# ****************************************************************************
# **  Copyright 2021                                                        **
# **  Department of Computer and Information Science, University of Oregon  **
# ****************************************************************************

# IF you get this error:
# tau_wrap++.cpp:38:10: fatal error: 'clang-c/Index.h' file not found
# Then set this value to the location of clang-c/Index.h
LLVM_INCLUDE=-I/usr/lib/llvm-10/include

PWD=$(shell pwd)
MYCXXFLAGS=-fPIC -I. -g -O3 -std=c++11 -Wall -Werror ${LLVM_INCLUDE}
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
