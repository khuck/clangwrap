#!/bin/bash -e
# ****************************************************************************
# **  TAU Portable Profiling Package                                        **
# **  http://tau.uoregon.edu                                                **
# ****************************************************************************
# **  Copyright 2021                                                        **
# **  Department of Computer and Information Science, University of Oregon  **
# ****************************************************************************


set -x
tau_wrap++ \
`pwd`/secret.hpp \
-o wrapper.cpp \
-w /home/users/khuck/src/ADIOS2/install_nompi/lib64/libadios2.so

# for debugging:
# clang -Xclang -ast-dump -fsyntax-only ./secret.hpp
