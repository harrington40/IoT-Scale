#######################################################################
# Makefile for errors test-suite
#
# This test-suite is for checking SWIG errors and warnings and uses
# Python as the target language.
#
# It compares the stderr output from SWIG to the contents of the .stderr
# file for each test case. The test cases are different to those used by
# the language module test-suites. The .i files in this directory are
# used instead of those in the parent directory.
#
# When adding a new test case, be sure to commit the expected output
# file (.stderr) in addition to the test case itself.
#######################################################################

LANGUAGE     = errors
ERROR_EXT    = newerr

HAVE_CXX11   = 1
HAVE_CXX14   = 1
HAVE_CXX17   = 1
HAVE_CXX20   = 1
srcdir       = .
top_srcdir   = ../../..
top_builddir = ../../..

SWIG_LIB_SET = env SWIG_LIB=$(SWIG_LIB_DIR)
SWIGINVOKE   = $(SWIG_LIB_SET) $(SWIGTOOL) $(SWIGEXE)

# All .i files with prefix 'cpp_' will be treated as C++ input and remaining .i files as C input
ALL_ERROR_TEST_CASES := $(sort $(patsubst %.i,%, $(notdir $(wildcard $(srcdir)/*.i))))
CPP_ERROR_TEST_CASES := $(sort $(filter cpp_%, $(ALL_ERROR_TEST_CASES)))
C_ERROR_TEST_CASES := $(sort $(filter-out $(CPP_ERROR_TEST_CASES), $(ALL_ERROR_TEST_CASES)))
DOXYGEN_ERROR_TEST_CASES := $(sort $(filter doxygen_%, $(C_ERROR_TEST_CASES)))
C_ERROR_TEST_CASES := $(sort $(filter-out $(DOXYGEN_ERROR_TEST_CASES), $(C_ERROR_TEST_CASES)))

# Always use C++ for Doxygen tests, there doesn't seem to be any need to
# distinguish between C and C++ Doxygen tests.
DOXYGEN_ERROR_TEST_CASES := $(DOXYGEN_ERROR_TEST_CASES:=.cpptest)

ERROR_TEST_CASES := $(CPP_ERROR_TEST_CASES:=.cpptest) \
		    $(C_ERROR_TEST_CASES:=.ctest) \
		    $(DOXYGEN_ERROR_TEST_CASES)

include $(srcdir)/../common.mk

# This is tricky: we need to let common.mk define SWIGOPT before appending to
# it, if we do it before including it, its defining of SWIGOPT would override
# whatever we do here.
$(DOXYGEN_ERROR_TEST_CASES): SWIGOPT += -doxygen

# Unique module names are obtained from the .i file name (required for parallel make).
# Note: -module overrides %module in the .i file.
MODULE_OPTION=-module $*
nomodule.ctest: MODULE_OPTION =

# Portable dos2unix / fromdos for stripping CR
FROMDOS       = tr -d '\r'

# strip source directory from output, so that diffs compare
STRIP_SRCDIR = sed -e 's|\\|/|g' -e 's|^$(SRCDIR)||'

# Rules for the different types of tests
%.cpptest:
	$(ECHO_PROGRESS) "$(ACTION)ing errors testcase $*"
	-$(SWIGINVOKE) -c++ -python -Wall -Fstandard $(MODULE_OPTION) $(SWIGOPT) $(SRCDIR)$*.i 2>&1 | $(FROMDOS) | $(STRIP_SRCDIR) > $*.$(ERROR_EXT)
	$(COMPILETOOL) diff -c $(SRCDIR)$*.stderr $*.$(ERROR_EXT)

%.ctest:
	$(ECHO_PROGRESS) "$(ACTION)ing errors testcase $*"
	-$(SWIGINVOKE) -python -Wall -Fstandard $(MODULE_OPTION) $(SWIGOPT) $(SRCDIR)$*.i 2>&1 | $(FROMDOS) | $(STRIP_SRCDIR) > $*.$(ERROR_EXT)
	$(COMPILETOOL) diff -c $(SRCDIR)$*.stderr $*.$(ERROR_EXT)

%.clean:
	@exit 0

clean:
	$(MAKE) -f $(top_builddir)/$(EXAMPLES)/Makefile SRCDIR='$(SRCDIR)' python_clean
	@rm -f *.$(ERROR_EXT) *.py
