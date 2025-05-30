#######################################################################
# Makefile for java test-suite
#######################################################################

LANGUAGE     = java
JAVA         = "java"
JAVAC        = "javac"
JAVAFLAGS    = 
JAVA_CLASSPATH_SEP = :
JAVA_TOOLS_JAR     = @JAVA_TOOLS_JAR@
SCRIPTSUFFIX = _runme.java
SKIP_DOXYGEN_TEST_CASES = 

HAVE_CXX11   = 1
HAVE_CXX14   = 1
HAVE_CXX17   = 1
HAVE_CXX20   = 1
srcdir       = .
top_srcdir   = ../../../..
top_builddir = ../../../..

C_TEST_CASES = \
	java_lib_arrays \
	java_lib_various

CPP_TEST_CASES = \
	enum_thorough_proper \
	enum_thorough_simple \
	enum_thorough_typeunsafe \
	exception_partial_info \
	intermediary_classname \
	java_constants \
	java_director \
	java_director_assumeoverride \
	java_director_exception_feature \
	java_director_exception_feature_nspace \
	java_director_ptrclass \
	java_director_typemaps \
	java_director_typemaps_ptr \
	java_enums \
	java_jnitypes \
	java_lib_arrays_dimensionless \
	java_lib_various \
	java_nspacewithoutpackage \
	java_pgcpp \
	java_pragmas \
	java_prepost \
	java_throws \
	java_typemaps_proxy \
	java_typemaps_typewrapper \
	nested_scope \
	li_std_list \
	li_std_set \
#	li_boost_intrusive_ptr

CPP11_TEST_CASES = \
	cpp11_shared_ptr_const \
	cpp11_shared_ptr_crtp_upcast \
	cpp11_shared_ptr_nullptr_in_containers \
	cpp11_shared_ptr_overload \
	cpp11_shared_ptr_template_upcast \
	cpp11_shared_ptr_upcast \
	cpp11_std_unordered_map \
	cpp11_std_unordered_set \
	cpp11_strongly_typed_enumerations_simple \

DOXYGEN_TEST_CASES := \
	doxygen_parsing_enums_simple \
	doxygen_parsing_enums_proper \
	doxygen_parsing_enums_typesafe \
	doxygen_parsing_enums_typeunsafe \

include $(srcdir)/../common.mk

# Overridden variables here
SRCDIR       = ../$(srcdir)/
JAVA_PACKAGE = $*
JAVA_PACKAGEOPT = -package $(JAVA_PACKAGE)
SWIGOPT += $(JAVA_PACKAGEOPT)

# Custom tests - tests with additional commandline options
cpp17_nspace_nested_namespaces.%: JAVA_PACKAGE = $*Package
director_nspace.%: JAVA_PACKAGE = $*Package
director_nspace_director_name_collision.%: JAVA_PACKAGE = $*Package
java_director_exception_feature_nspace.%: JAVA_PACKAGE = $*Package
java_nspacewithoutpackage.%: JAVA_PACKAGEOPT =
multiple_inheritance_nspace.%: JAVA_PACKAGE = $*Package
nspace.%: JAVA_PACKAGE = $*Package
nspace_extend.%: JAVA_PACKAGE = $*Package
nspacemove.%: JAVA_PACKAGE = $*Package
nspacemove_nested.%: JAVA_PACKAGE = $*Package
nspacemove_stl.%: JAVA_PACKAGE = $*Package

# Rules for the different types of tests
%.cpptest:
	$(setup)
	+(cd $(JAVA_PACKAGE) && $(swig_and_compile_cpp))
	$(run_testcase)

%.ctest:
	$(setup)
	+(cd $(JAVA_PACKAGE) && $(swig_and_compile_c))
	$(run_testcase)

%.multicpptest:
	$(setup)
	+(cd $(JAVA_PACKAGE) && $(swig_and_compile_multi_cpp))
	$(run_testcase)

# Makes a directory for the testcase if it does not exist
setup = \
	if [ -f $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX) ]; then	  \
	  $(ECHO_PROGRESS) "$(ACTION)ing $(LANGUAGE) testcase $* (with run test)" ; \
	else								  \
	  $(ECHO_PROGRESS) "$(ACTION)ing $(LANGUAGE) testcase $*" ;	  \
	fi;								  \
	if [ ! -d $(JAVA_PACKAGE) ]; then						  \
	  mkdir $(JAVA_PACKAGE);							  \
	fi

# Doxygen test cases need to be compiled together with the CommentParser class.
CommentParser.class:
	$(COMPILETOOL) $(JAVAC) -classpath $(JAVA_CLASSPATH) -d . $(srcdir)/CommentParser.java

JAVA_CLASSPATH := .
$(DOXYGEN_TEST_CASES:=.cpptest): CommentParser.class

# Compiles java files then runs the testcase. A testcase is only run if
# a file is found which has _runme.java appended after the testcase name.
# Note Java uses LD_LIBRARY_PATH under Unix, PATH under Cygwin/Windows, SHLIB_PATH on HPUX and DYLD_LIBRARY_PATH on Mac OS X.
run_testcase = \
	cd $(JAVA_PACKAGE) && $(COMPILETOOL) $(JAVAC) -classpath . `find . -name "*.java"` && cd .. && \
	if [ -f $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX) ]; then \
	  $(COMPILETOOL) $(JAVAC) -classpath $(JAVA_CLASSPATH) -d . $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX) && \
	  env LD_LIBRARY_PATH="$(JAVA_PACKAGE):$$LD_LIBRARY_PATH" PATH="$(JAVA_PACKAGE):$$PATH" SHLIB_PATH="$(JAVA_PACKAGE):$$SHLIB_PATH" DYLD_LIBRARY_PATH="$(JAVA_PACKAGE):$$DYLD_LIBRARY_PATH" $(RUNTOOL) $(JAVA) $(JAVAFLAGS) -classpath $(JAVA_CLASSPATH) $*_runme; \
	fi

# Clean: remove testcase directories
%.clean:
	@if [ -d $(JAVA_PACKAGE) ]; then \
	  rm -rf $(JAVA_PACKAGE); \
	fi
	@rm -f $*_runme.class

clean:
	@rm -f *.class hs_err*.log
