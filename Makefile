# ****************************************************************************
# **  TAU Portable Profiling Package                                        **
# **  http://tau.uoregon.edu                                                **
# ****************************************************************************
# **  Copyright 2021                                                        **
# **  Department of Computer and Information Science, University of Oregon  **
# ****************************************************************************

TOPTARGETS=all clean test

SUBDIRS=src simple

$(TOPTARGETS): $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
