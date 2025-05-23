#######################################################################
# Makefile for perl5 test-suite
#######################################################################

LANGUAGE     = perl5
PERL         = perl
SCRIPTSUFFIX = _runme.pl
TEST_RUNNER  = $(srcdir)/run-perl-test.pl

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
	li_cdata_carrays_cpp \
	li_reference \
	memberin1 \
	director_nestedmodule \

CPP11_TEST_CASES += \
	cpp11_strongly_typed_enumerations_perl_const

C_TEST_CASES += \
	li_cstring \
	li_cdata_carrays \
	multivalue \

include $(srcdir)/../common.mk

# Overridden variables here
# none!

# Custom tests - tests with additional commandline options
cpp11_strongly_typed_enumerations_perl_const.cpptest: SWIGOPT += -const

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
# a file is found which has _runme.pl appended after the testcase name.
run_testcase = \
	if [ -f $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX) ]; then \
	  env LD_LIBRARY_PATH=.:$$LD_LIBRARY_PATH $(RUNTOOL) $(PERL) $(TEST_RUNNER) $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX); \
	fi

# Clean: remove the generated .pm file
%.clean:
	@rm -f $*.pm;

clean:
	$(MAKE) -f $(top_builddir)/$(EXAMPLES)/Makefile SRCDIR='$(SRCDIR)' perl5_clean
