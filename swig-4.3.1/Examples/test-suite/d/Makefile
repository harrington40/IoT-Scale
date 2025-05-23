#######################################################################
# Makefile for D test-suite
#######################################################################

LANGUAGE     = d

HAVE_CXX11   = 1
HAVE_CXX14   = 1
HAVE_CXX17   = 1
HAVE_CXX20   = 1
srcdir       = .
top_srcdir   = ../../../..
top_builddir = ../../../..

CPP_TEST_CASES = \
	d_nativepointers \
	exception_partial_info

CPP11_TEST_CASES = \
	cpp11_shared_ptr_const \
	cpp11_shared_ptr_crtp_upcast \
	cpp11_shared_ptr_nullptr_in_containers \
	cpp11_shared_ptr_overload \
	cpp11_shared_ptr_upcast \

# "D classes support the single inheritance paradigm" https://dlang.org/spec/class.html
FAILING_CPP_TESTS += \
	using_composition \
	using_extend \

# Build error due to 2 classes with same name.
FAILING_CPP_TESTS += \
	director_nspace_director_name_collision \

# Swig D do not forward 'override' of functions through multiple swig run.
# You should compile your files at once.
FAILING_MULTI_CPP_TESTS += \
	imports \
	packageoption \

# TODO: probably a bug
# This test uses a struct extended property for a struct size.
# Using the same name for the struct and the struct type confuse the Swig D.
# It create a wrong property function in the C wrap code.
# Using different name for struct or struct type remove the function completly,
# which also implies on a bug.
# We should use proper testing with a run me script.
# To verify, the extended property actually works in run-time.
FAILING_C_TESTS += \
	nested_extend_c \

include $(srcdir)/../common.mk

# Overridden variables here
SRCDIR       = ../$(srcdir)/
TARGETSUFFIX = _wrap
SWIGOPT+=-splitproxy -package $*

# Rules for the different types of tests
%.cpptest:
	$(setup)
	+(cd $*.2 && $(swig_and_compile_cpp))
	+$(run_testcase)

%.ctest:
	$(setup)
	+(cd $*.2 && $(swig_and_compile_c))
	+$(run_testcase)

%.multicpptest:
	$(setup)
	+(cd $*.2 && $(swig_and_compile_multi_cpp))
	+$(run_testcase)

# Makes a directory for the testcase if it does not exist
setup = \
	if [ -f $(SCRIPTDIR)/$(SCRIPTPREFIX)$*_runme.2.d ]; then			\
	  $(ECHO_PROGRESS) "$(ACTION)ing $(LANGUAGE) testcase $* (with run test)" ;	\
	else									\
	  $(ECHO_PROGRESS) "$(ACTION)ing $(LANGUAGE) testcase $*" ;		\
	fi;									\
	if [ ! -d $*.2 ]; then					\
	  mkdir $*.2;						\
	fi;									\
	if [ ! -d $*.2/$* ]; then					\
	  mkdir $*.2/$*;						\
	fi

# Compiles D files then runs the testcase. A testcase is only run if
# a file is found which has _runme.d appended after the testcase name.
run_testcase = \
	if [ -f $(SCRIPTDIR)/$(SCRIPTPREFIX)$*_runme.2.d ]; then \
	  cd $*.2 && \
	  $(MAKE) -f $(top_builddir)/$(EXAMPLES)/Makefile \
	  DFLAGS='-of$*_runme' \
	  DSRCS='../$(SCRIPTDIR)/$(SCRIPTPREFIX)$*_runme.2.d `find $* -name *.d`' d_compile && \
	  env LD_LIBRARY_PATH=".:$$LD_LIBRARY_PATH" $(RUNTOOL) ./$*_runme; \
	else \
	  cd $*.2 && \
	  $(MAKE) -f $(top_builddir)/$(EXAMPLES)/Makefile \
	  DFLAGS='-c' \
	  DSRCS='`find $* -name *.d`' d_compile && cd .. ; \
	fi

# Clean: remove testcase directories
%.clean:
	@if [ -d $*.2 ]; then \
	  rm -rf $*.2; \
	fi
