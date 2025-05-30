#######################################################################
# Makefile for php test-suite
#######################################################################

LANGUAGE     = php
SCRIPTSUFFIX = _runme.php

HAVE_CXX11   = 1
HAVE_CXX14   = 1
HAVE_CXX17   = 1
HAVE_CXX20   = 1
srcdir       = .
top_srcdir   = ../../..
top_builddir = ../../..

CPP_TEST_CASES += \
	callback \
	director_stl \
	exception_partial_info \
	li_cdata_carrays_cpp \
	li_factory \
	php_iterator \
	php_namewarn_rename \
	php_pragma \
	prefix \

C_TEST_CASES += \
	li_cdata_carrays \
	multivalue \

include $(srcdir)/../common.mk

# Overridden variables here
TARGETPREFIX =# Should be php_ for Windows, empty otherwise

# Custom tests - tests with additional commandline options
prefix.cpptest: SWIGOPT += -prefix Project

# write out tests without a _runme.php
missingcpptests:
	for test in $(CPP_TEST_CASES) ; do test -f $${test}_runme.php || echo $${test}; done

missingctests:
	for test in $(C_TEST_CASES) ; do test -f $${test}_runme.php || echo $${test}; done

missingtests: missingcpptests missingctests

# Rules for the different types of tests
%.cpptest:
	$(setup)
	+$(swig_and_compile_cpp)
	+$(run_testcase)

%.ctest:
	$(setup)
	+$(swig_and_compile_c)
	+$(run_testcase)

# Trying to load the modules for imports.multicpptest fails with:
#
# Warning: Function registration failed - duplicate name - global_test in Unknown on line 0
# Segmentation fault
#
# This was previously hidden because we didn't even try to load the modules for
# .multicpptest testcases, so for now just do the parts of the test we did
# before.  FIXME: Fix this!
%.multicpptest:
	$(setup)
	+$(swig_and_compile_multi_cpp)
	+[ "$*" = "imports" ] || $(run_multi_testcase)

# Smart target
%.test:
	@echo ' $(C_TEST_CASES) '|grep -F -v ' $* ' >/dev/null ||\
	    $(MAKE) $*.ctest
	@echo ' $(CPP_TEST_CASES) '|grep -F -v ' $* ' >/dev/null ||\
	    $(MAKE) $*.cpptest
	@echo ' $(MULTI_CPP_TEST_CASES) '|grep -F -v ' $* ' >/dev/null ||\
	    $(MAKE) $*.multicpptest

# Runs the testcase. Tries to run testcase_runme.php, and if that's not found,
# at least test that the module loads without errors.
run_testcase = \
	if [ -f $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX) ]; then \
	  $(MAKE) -f $(top_builddir)/$(EXAMPLES)/Makefile PHP_EXTENSION=$(TARGETPREFIX)$*.so PHP_SCRIPT=$(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX) RUNTOOL='$(RUNTOOL)' php_run; \
	else \
	  $(MAKE) -f $(top_builddir)/$(EXAMPLES)/Makefile PHP_EXTENSION=$(TARGETPREFIX)$*.so PHP_SCRIPT= RUNTOOL='$(RUNTOOL)' php_run; \
	fi

# Runs a multicpptest testcase. Tries to run testcase_runme.php, and if that's
# not found, at least test that the modules load without errors.
run_multi_testcase = \
	if [ -f $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX) ]; then \
	  $(MAKE) -f $(top_builddir)/$(EXAMPLES)/Makefile PHP_EXTENSION_LIST=$(top_srcdir)/$(EXAMPLES)/$(TEST_SUITE)/$*.list PHP_SCRIPT=$(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX) RUNTOOL='$(RUNTOOL)' php_run_multi; \
	else \
	  $(MAKE) -f $(top_builddir)/$(EXAMPLES)/Makefile PHP_EXTENSION_LIST=$(top_srcdir)/$(EXAMPLES)/$(TEST_SUITE)/$*.list PHP_SCRIPT= RUNTOOL='$(RUNTOOL)' php_run_multi; \
	fi

# Clean: remove the generated PHP-specific files
%.clean:
	@rm -f php_$*.h

clean:
	$(MAKE) -f $(top_builddir)/$(EXAMPLES)/Makefile SRCDIR='$(SRCDIR)' php_clean
	rm -f clientdata_prop_a.php clientdata_prop_b.php php_clientdata_prop_a.h php_clientdata_prop_b.h
	rm -f import_stl_a.php import_stl_b.php php_import_stl_a.h php_import_stl_b.h
	rm -f imports_a.php imports_b.php php_imports_a.h php_imports_b.h
	rm -f mod_a.php mod_b.php php_mod_a.h php_mod_b.h
	rm -f multi_import_a.php multi_import_b.php multi_import_d.php php_multi_import_a.h php_multi_import_b.h php_multi_import_d.h
	rm -f packageoption_a.php packageoption_b.php packageoption_c.php php_packageoption_a.h php_packageoption_b.h php_packageoption_c.h
	rm -f template_typedef_cplx2.php php_template_typedef_cplx2.h
