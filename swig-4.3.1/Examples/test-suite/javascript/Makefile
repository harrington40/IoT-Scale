#######################################################################
# Makefile for javascript test-suite
#######################################################################

LANGUAGE     = javascript
NODEGYP = node-gyp
NODEJS = node
SCRIPTSUFFIX = _runme.js
OBJEXT = o
SO = .so
GYP_CXXFLAGS = -I/usr/include -std=c++20 -I$(shell npm config get prefix)/lib/node_modules/node-addon-api

HAVE_CXX11   = 1
HAVE_CXX14   = 1
HAVE_CXX17   = 1
HAVE_CXX20   = 1
srcdir       = .
top_srcdir   = ../../..
top_builddir = ../../..

CPP_TEST_CASES += \
	iadd \
	inplaceadd \
	input \
	javascript_lib_arrays \
	li_factory \
	li_std_containers_int \
	li_std_map_member

# napi fails
FAILING_CPP_TESTS += \
	smart_pointer_static \

SWIGEXE   = $(top_builddir)/swig
SWIG_LIB_DIR = $(top_srcdir)/Lib

ifneq (, $(ENGINE))
    JSENGINE=$(ENGINE)
else
ifneq (, $(NODEJS))
    JSENGINE=node
else
ifneq (, )
    JSENGINE=jsc
else
ifneq (, 1)
    JSENGINE=v8
else
    # Shouldn't happen, but avoid empty value if it does.
    JSENGINE=node
endif
endif
endif
endif

include $(srcdir)/../common.mk

_setup = \
	if [ -f $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX) ]; then	  \
	  $(ECHO_PROGRESS) "$(ACTION)ing $(LANGUAGE) ($(JSENGINE)) testcase $* (with run test)" ; \
	else								  \
	  $(ECHO_PROGRESS) "$(ACTION)ing $(LANGUAGE) ($(JSENGINE)) testcase $*" ;     \
	fi;

SWIGOPT += -$(JSENGINE)

ifeq ($(JSENGINE), $(filter $(JSENGINE), node napi))
  SWIGOPT += -DBUILDING_NODE_EXTENSION=1
  
  # shut up some warnings

  # dunno... ignoring generously
  apply_signed_char.cpptest: GYP_CFLAGS = \"-Wno-ignored-qualifiers\"
  constant_pointers.cpptest: GYP_CFLAGS = \"-Wno-ignored-qualifiers\"
  cpp11_ref_qualifiers.cpptest: GYP_CFLAGS = \"-Wno-ignored-qualifiers\"
  director_basic.cpptest: GYP_CFLAGS = \"-Wno-ignored-qualifiers\"
  enum_thorough.cpptest: GYP_CFLAGS = \"-Wno-ignored-qualifiers\"
  member_funcptr_galore.cpptest: GYP_CFLAGS = \"-Wno-ignored-qualifiers\"
  director_unwrap_result.cpptest: GYP_CFLAGS = \"-Wno-ignored-qualifiers\"

	# Run with asan:
	# ASAN=1 ENGINE=napi make <test_name>.cpptest
	#
	ifneq (, $(ASAN))
		GYP_CFLAGS += \"-fsanitize=address\"
		GYP_OPTS = --debug
		ASAN_PRELOAD = $(shell $(CC) -print-file-name=libasan.so)
		LSAN_OPTIONS = suppressions=$(srcdir)/node_template/napi-leaks-suppression.txt
		BUILD = Debug
	else
		BUILD = Release
	endif

	setup_node = \
		test -d $* || mkdir $* && \
		sed -e 's|$$testcase|$*|g; s|$$cflags|$(GYP_CFLAGS)|g; s|$$cxxflags|"$(GYP_CXXFLAGS)"|g; s|$$srcdir|$(srcdir)|g' \
			$(srcdir)/node_template/binding.gyp.in > $*/binding.gyp && \
		sed -e 's|$$testcase|$*|g; s|$$build|$(BUILD)|g;' \
			$(srcdir)/node_template/index.js.in > $*/index.js
		
	# Note: we need to use swig in C parse mode, but make node-gyp believe it is c++ (via file extension)
	swig_and_compile_c = \
		$(setup_node) && \
		$(MAKE) -f $(top_builddir)/$(EXAMPLES)/Makefile SRCDIR='$(SRCDIR)' \
		SWIG_LIB_DIR='$(SWIG_LIB_DIR)' SWIGEXE='$(SWIGEXE)' \
		SWIGOPT='-javascript $(SWIGOPT) -o $*_wrap.cxx $(srcdir)/../$*.i' swiginvoke && \
		MAKEFLAGS= $(COMPILETOOL) $(NODEGYP) --loglevel=silent --directory $* configure $(GYP_OPTS) build 1>>/dev/null

	swig_and_compile_cpp = \
		$(setup_node) && \
		$(MAKE) -f $(top_builddir)/$(EXAMPLES)/Makefile SRCDIR='$(SRCDIR)' \
		SWIG_LIB_DIR='$(SWIG_LIB_DIR)' SWIGEXE='$(SWIGEXE)' \
		SWIGOPT='-c++ -javascript $(SWIGOPT) $(srcdir)/../$*.i' swiginvoke && \
		MAKEFLAGS= $(COMPILETOOL) $(NODEGYP) --loglevel=silent --directory $* configure $(GYP_OPTS) build 1>>/dev/null

	run_testcase = \
		if [ -f $(srcdir)/$*$(SCRIPTSUFFIX) ]; then \
			env NODE_PATH=$$PWD:$(srcdir) LD_PRELOAD=$(ASAN_PRELOAD) LSAN_OPTIONS=$(LSAN_OPTIONS) \
				$(RUNTOOL) $(NODEJS) $(srcdir)/$*$(SCRIPTSUFFIX); \
		fi


  %.ctest:
		$(_setup)
		+$(swig_and_compile_c)
		$(run_testcase)

  %.cpptest:
		$(_setup)
		+$(swig_and_compile_cpp)
		$(run_testcase)

  %.multicpptest:
		$(_setup)

else

	run_testcase = \
		if [ -f $(srcdir)/$*$(SCRIPTSUFFIX) ]; then \
		  $(RUNTOOL) $(top_builddir)/Tools/javascript/javascript -$(JSENGINE) $(srcdir)/$*$(SCRIPTSUFFIX); \
		fi

  %.ctest:
		$(_setup)
		+$(swig_and_compile_c)
		$(run_testcase)

  %.cpptest:
		$(_setup)
		+$(swig_and_compile_cpp)
		$(run_testcase)

  %.multicpptest:
		$(_setup)
		+$(swig_and_compile_multi_cpp)
		$(run_testcase)

endif


%.clean:
	@rm -rf $*
	@rm -f $*_wrap.* $*$(SO) $*.$(OBJEXT)

clean:
	for ext in _wrap.cxx _wrap.$(OBJEXT) $(SO); do \
		rm -f clientdata_prop_a$${ext} clientdata_prop_b$${ext}; \
		rm -f imports_a$${ext} imports_b$${ext}; \
		rm -f import_stl_a$${ext} import_stl_b$${ext}; \
		rm -f mod_a$${ext} mod_b$${ext}; \
		rm -f multi_import_a$${ext} multi_import_b$${ext} multi_import_d$${ext}; \
		rm -f packageoption_a$${ext} packageoption_b$${ext} packageoption_c$${ext}; \
		rm -f template_typedef_cplx2$${ext}; \
	done
