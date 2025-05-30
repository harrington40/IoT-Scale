#######################################################################
# Makefile for lua test-suite
#######################################################################

LANGUAGE     = lua
LUA          = 
SCRIPTSUFFIX = _runme.lua

HAVE_CXX11   = 1
HAVE_CXX14   = 1
HAVE_CXX17   = 1
HAVE_CXX20   = 1
srcdir       = .
top_srcdir   = ../../..
top_builddir = ../../..

# sorry, currently very few test cases work/have been written

CPP_TEST_CASES += \
	lua_no_module_global \
	lua_inherit_getitem  \
	lua_lightuserdata  \


C_TEST_CASES += \
	lua_no_module_global \


include $(srcdir)/../common.mk

# Overridden variables here
LIBS       = -L.

# Custom tests - tests with additional commandline options
lua_no_module_global.%: SWIGOPT += -nomoduleglobal

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
# a file is found which has _runme.lua appended after the testcase name.
run_testcase = \
	if [ -f $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX) ]; then \
	  env LUA_PATH="$(srcdir)/?.lua;" $(RUNTOOL) $(LUA) $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX); \
	fi

# Clean: (does nothing, we don't generate extra lua code)
%.clean:
	@exit 0

clean:
	$(MAKE) -f $(top_builddir)/$(EXAMPLES)/Makefile SRCDIR='$(SRCDIR)' lua_clean
