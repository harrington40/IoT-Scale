#######################################################################
# Makefile for C# test-suite
#######################################################################

LANGUAGE     = csharp
SCRIPTSUFFIX = _runme.cs
CSHARPCILINTERPRETER  = 
CSHARPCILINTERPRETER_FLAGS = 
CSHARPCONVERTPATH     = ../../../Tools/convertpath -u

HAVE_CXX11   = 1
HAVE_CXX14   = 1
HAVE_CXX17   = 1
HAVE_CXX20   = 1
srcdir       = .
top_srcdir   = ../../../..
top_builddir = ../../../..

CPP_TEST_CASES = \
	complextest \
	csharp_argument_defaults_feature \
	csharp_attributes \
	csharp_director_typemaps \
	csharp_exceptions \
	csharp_features \
	csharp_lib_arrays \
	csharp_lib_arrays_bool \
	csharp_namespace_system_collision \
	csharp_prepost \
	csharp_swig2_compatibility \
	csharp_typemaps \
	director_wstring \
	enum_thorough_simple \
	enum_thorough_typesafe \
	exception_partial_info \
	intermediary_classname \
	nested_scope \
	li_boost_intrusive_ptr \
	li_std_list \


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

include $(srcdir)/../common.mk

# Overridden variables here
SRCDIR       = ../$(srcdir)/
SWIGOPT += -namespace $*Namespace

CSHARPFLAGSSPECIAL =

# Custom tests - tests with additional commandline options
complextest.cpptest: CSHARPFLAGSSPECIAL = -r:System.Numerics.dll
csharp_lib_arrays.cpptest: CSHARPFLAGSSPECIAL = -unsafe
csharp_lib_arrays_bool.cpptest: CSHARPFLAGSSPECIAL = -unsafe
csharp_swig2_compatibility.cpptest: SWIGOPT += -DSWIG2_CSHARP
# Doxygen tests have specific flags to generate XML documentation and deactivate warnings about XML generation
doxygen_%.cpptest: CSHARPFLAGSSPECIAL = -doc:./$*/$*.xml -nowarn:"CS0419,CS1572,CS1573,CS1574,CS1584,CS1591"
doxygen_%.cpptest: CSHARPSRCS_EXTRA_RUN = `$(CSHARPCONVERTPATH) $(SCRIPTDIR)/doxygen_checker.cs`
intermediary_classname.cpptest: SWIGOPT += -dllimport intermediary_classname

# Rules for the different types of tests
%.cpptest:
	$(setup)
	+(cd $* && $(swig_and_compile_cpp))
	+$(run_testcase)

%.ctest:
	$(setup)
	+(cd $* && $(swig_and_compile_c))
	+$(run_testcase)

%.multicpptest:
	$(setup)
	+(cd $* && $(swig_and_compile_multi_cpp))
	+$(run_testcase)

# Makes a directory for the testcase if it does not exist
setup = \
	if [ -f $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX) ]; then	  \
	  $(ECHO_PROGRESS) "$(ACTION)ing $(LANGUAGE) testcase $* (with run test)" ; \
	else								  \
	  $(ECHO_PROGRESS) "$(ACTION)ing $(LANGUAGE) testcase $*" ;	  \
	fi;								  \
	if [ ! -d $* ]; then						  \
	  mkdir $*;							  \
	fi

# Compiles C# files then runs the testcase. A testcase is only run if
# a file is found which has _runme.cs appended after the testcase name.
# Note C# uses LD_LIBRARY_PATH under Unix, PATH under Cygwin/Windows and SHLIB_PATH on HPUX.
# DYLD_FALLBACK_LIBRARY_PATH is cleared for Mac OS X.
run_testcase = \
	if [ -f $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX) ]; then \
	  $(MAKE) -f $*/$(top_builddir)/$(EXAMPLES)/Makefile \
	  CSHARPFLAGS='-nologo -debug+ $(CSHARPFLAGSSPECIAL) -out:$*_runme.exe' \
	  CSHARPSRCS='`$(CSHARPCONVERTPATH) $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX)` `find $* -name "*.cs" -exec $(CSHARPCONVERTPATH) "{}" \+` $(CSHARPSRCS_EXTRA_RUN)' csharp_compile && \
	  env LD_LIBRARY_PATH="$*:$$LD_LIBRARY_PATH" PATH="$*:$$PATH" SHLIB_PATH="$*:$$SHLIB_PATH" DYLD_FALLBACK_LIBRARY_PATH= $(RUNTOOL) $(CSHARPCILINTERPRETER) $(CSHARPCILINTERPRETER_FLAGS) ./$*_runme.exe; \
	else \
	  $(MAKE) -f $*/$(top_builddir)/$(EXAMPLES)/Makefile \
	  CSHARPFLAGS='-nologo -debug+ $(CSHARPFLAGSSPECIAL) -t:module -out:$*/$*.netmodule' \
	  CSHARPSRCS='`find $* -name "*.cs" -exec $(CSHARPCONVERTPATH) "{}" \+`' csharp_compile; \
	fi

# Clean: remove testcase directories
%.clean:
	@if [ -d $* ]; then \
	  rm -rf $*; \
	fi

clean:
	@rm -f *.exe *.exe.mdb
