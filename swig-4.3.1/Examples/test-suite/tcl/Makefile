#######################################################################
# Makefile for tcl test-suite
#######################################################################

LANGUAGE     = tcl
TCLSH        = tclsh
SCRIPTSUFFIX = _runme.tcl

HAVE_CXX11   = 1
HAVE_CXX14   = 1
HAVE_CXX17   = 1
HAVE_CXX20   = 1
srcdir       = .
top_srcdir   = ../../..
top_builddir = ../../..

CPP_TEST_CASES += \
	primitive_types \
	li_cstring \
	li_cwstring

C_TEST_CASES += \
	li_cstring \
	li_cwstring

include $(srcdir)/../common.mk

# Overridden variables here
# none!

# Custom tests - tests with additional commandline options
# none!

# Rules for the different types of tests
%.cpptest:
	$(setup)
	+$(swig_and_compile_cpp)
	$(run_testcase)

%.ctest:
	$(setup)
	+$(swig_and_compile_c)
	$(run_testcase)

%.multicpptest:
	$(setup)
	+$(swig_and_compile_multi_cpp)
	$(run_testcase)

# Runs the testcase. A testcase is only run if
# a file is found which has _runme.tcl appended after the testcase name.
run_testcase = \
	if [ -f $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX) ]; then \
	  env LD_LIBRARY_PATH=.:$$LD_LIBRARY_PATH $(RUNTOOL) $(TCLSH) $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX); \
	fi

# Clean
%.clean:
	@exit 0

clean:
	$(MAKE) -f $(top_builddir)/$(EXAMPLES)/Makefile SRCDIR='$(SRCDIR)' tcl_clean
