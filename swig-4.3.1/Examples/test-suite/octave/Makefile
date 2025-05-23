#######################################################################
# Makefile for octave test-suite
#######################################################################

LANGUAGE     = octave
OCTAVE       = 
SCRIPTSUFFIX = _runme.m
PCHSUPPORT   = yes

HAVE_CXX11   = 1
HAVE_CXX14   = 1
HAVE_CXX17   = 1
HAVE_CXX20   = 1
srcdir       = .
top_srcdir   = ../../..
top_builddir = ../../..

CPP_TEST_CASES += \
	li_std_pair_extra \
	li_std_string_extra \
	octave_cell_deref\
	octave_dim

CPP11_TEST_CASES += \
	cpp11_shared_ptr_const \
	cpp11_shared_ptr_crtp_upcast \
	cpp11_shared_ptr_nullptr_in_containers \
	cpp11_shared_ptr_overload \
	cpp11_shared_ptr_upcast \

include $(srcdir)/../common.mk

# Overridden variables here
LIBS       = -L.
CSRCS      = octave_empty.c

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

# Pre-compile Octave headers, if supported

ifeq (yes,$(PCHSUPPORT))

export OCTHEADERSSRC = ../../../Lib/octave/octheaders.hpp
export OCTHEADERS    = ../../../Examples/test-suite/octave/octheaders.hpp
export OCTHEADERSGCH = $(OCTHEADERS).gch
export SWIGOCTHDROPT = -DSWIG_OCTAVE_EXTERNAL_OCTHEADERS
export IOCTHEADERS   = -I../../../Examples/test-suite/octave -include $(OCTHEADERS)

$(OCTHEADERSGCH): $(OCTHEADERSSRC)
	$(MAKE) -f $(top_builddir)/$(EXAMPLES)/Makefile octave_precompile_headers

$(NOT_BROKEN_TEST_CASES) $(BROKEN_TEST_CASES): $(OCTHEADERSGCH)

endif

# Runs the testcase. A testcase is only run if
# a file is found which has _runme.m appended after the testcase name.
run_testcase = \
	if [ -f $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX) ]; then \
	  env LD_LIBRARY_PATH=.:$$LD_LIBRARY_PATH OCTAVE_PATH=$(srcdir) OCTAVE_HISTFILE=/dev/null $(RUNTOOL) $(OCTAVE) $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX); \
	fi

# Clean: remove the generated .m file
%.clean:
	@rm -f hugemod.h hugemod_a.i hugemod_b.i hugemod_a.m hugemod_b.m hugemod_runme.m
	@rm -f $*.m;

clean:
	$(MAKE) -f $(top_builddir)/$(EXAMPLES)/Makefile SRCDIR='$(SRCDIR)' octave_clean

hugemod:
	perl hugemod.pl
	$(MAKE) hugemod_a.cpptest
	$(MAKE) hugemod_b.cpptest
	time $(OCTAVE) hugemod_runme.m
	time $(OCTAVE) hugemod_runme.m
