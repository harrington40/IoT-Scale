#######################################################################
# Makefile for scilab test-suite
#######################################################################

LANGUAGE     = scilab
SCILAB       = 
SCILAB_OPT   = 
SCRIPTSUFFIX = _runme.sci

HAVE_CXX11   = 1
HAVE_CXX14   = 1
HAVE_CXX17   = 1
HAVE_CXX20   = 1
srcdir       = .
top_srcdir   = ../../../..
top_builddir = ../../../..

C_TEST_CASES += \
	scilab_consts \
	scilab_identifier_name \

CPP_TEST_CASES += \
	primitive_types \
	scilab_li_matrix \
	scilab_multivalue \
	scilab_enums \
	scilab_pointer_conversion_functions \

CPP_STD_TEST_CASES += \
	li_std_container_typemaps \
	li_std_string_extra \

include $(srcdir)/../common.mk

# Overridden variables
SRCDIR = ../$(srcdir)/

# Local variables
TEST_DIR = $*.dir
RUNME_SCRIPT = $(SCRIPTPREFIX)$*$(SCRIPTSUFFIX)
SRC_RUNME_SCRIPT = $(srcdir)/$(RUNME_SCRIPT)

# Hide too long identifier warnings
SWIGOPT += -w720

# Rules for the different types of tests
%.cpptest:
	$(setup)
	+(cd $(TEST_DIR) && $(swig_and_compile_cpp))
	cd $(TEST_DIR) && $(run_testcase)

%.ctest:
	$(setup)
	+(cd $(TEST_DIR) && $(swig_and_compile_c))
	cd $(TEST_DIR) && $(run_testcase)

%.multicpptest:
	$(setup)
	+(cd $(TEST_DIR) && $(swig_and_compile_multi_cpp))
	cd $(TEST_DIR) && $(run_testcase)

# Copies files and creates directories needed for the test case
setup = \
	if [ ! -d $(TEST_DIR) ]; then	\
	  mkdir $(TEST_DIR); \
	fi; \
	if [ -f $(SRC_RUNME_SCRIPT) ]; then	\
	  $(ECHO_PROGRESS) "$(ACTION)ing $(LANGUAGE) testcase $* (with run test)" ; \
	  if [ ! -f $(TEST_DIR) ]; then	\
	    cp $(SRC_RUNME_SCRIPT) $(TEST_DIR); \
	  fi; \
	  if [ ! -f $(TEST_DIR)/swigtest.start ]; then	\
	    cp $(srcdir)/swigtest.start $(TEST_DIR); \
	  fi; \
	  if [ ! -f $(TEST_DIR)/swigtest.quit ]; then \
	    cp $(srcdir)/swigtest.quit $(TEST_DIR); \
	  fi; \
	else \
	  $(ECHO_PROGRESS) "$(ACTION)ing $(LANGUAGE) testcase $*" ; \
	fi; \

# Runs the testcase. A testcase is only run if
# a file is found which has _runme.sci appended after the testcase name.
run_testcase = \
	if [ -f $(RUNME_SCRIPT) ]; then ( \
	  env LD_LIBRARY_PATH=$(srcdir):$$LD_LIBRARY_PATH $(RUNTOOL) $(SCILAB) $(SCILAB_OPT) -f $(RUNME_SCRIPT); ) \
	fi

# Clean: remove the generated files
%.clean:
	@rm -rf $(TEST_DIR)

clean:
