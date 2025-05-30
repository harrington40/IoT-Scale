#######################################################################
# Makefile for Go test-suite
#######################################################################

LANGUAGE	= go
GO		= 
GOGCC		= false
GCCGO		= 
GOVERSIONOPTION	= 
host		= x86_64-pc-linux-gnu
SCRIPTSUFFIX	= _runme.go

SO = .so

HAVE_CXX11   = 1
HAVE_CXX14   = 1
HAVE_CXX17   = 1
HAVE_CXX20   = 1
srcdir         = .
top_srcdir     = ../../..
top_builddir   = ../../..
abs_top_srcdir = /home/harrington/scale_iot_design_04-30-2025/swig-4.3.1

CPP_TEST_CASES = \
	go_inout \
	go_director_inout

MULTI_CPP_TEST_CASES = \
	go_subdir_import

include $(srcdir)/../common.mk

INCLUDES = -I$(abs_top_srcdir)/$(EXAMPLES)/$(TEST_SUITE)

.SUFFIXES: .cpptest .ctest .multicpptest

# Rules for the different types of tests
%.cpptest:
	$(setup)
	+$(swig_and_compile_cpp)
	$(run_testcase_cpp)

%.ctest:
	$(setup)
	+$(swig_and_compile_c)
	$(run_testcase)

%.multicpptest:
	$(setup)
	$(go_multicpp_setup)
	+for f in `cat $(top_srcdir)/$(EXAMPLES)/$(TEST_SUITE)/$*.list` ; do \
	  $(call swig_and_compile_cpp_helper,$${f},'$(SWIGOPT)') GOMOD="$*"; \
	done
	$(run_multi_testcase)

li_windows.cpptest:
	# Does not work because go build won't build li_windows.go,
	# because file names with "windows" are only built on Windows.

go_subdir_import.multicpptest:
	$(setup)
	$(go_multicpp_setup)
	mkdir -p testdir/go_subdir_import 2>/dev/null || true
	mkdir -p gopath/go_subdir_import/src/testdir/go_subdir_import 2>/dev/null || true
	$(MAKE) -f $(top_builddir)/$(EXAMPLES)/Makefile SRCDIR='$(SRCDIR)' CXXSRCS='$(CXXSRCS)' \
	SWIG_LIB_DIR='$(SWIG_LIB_DIR)' SWIGEXE='$(SWIGEXE)' \
	LIBS='$(LIBS)' INTERFACEPATH='$(SRCDIR)$(INTERFACEDIR)go_subdir_import_b.i' \
	INCLUDES='$(INCLUDES)' SWIGOPT='$(SWIGOPT) -outdir .' NOLINK=true \
	TARGET='$(TARGETPREFIX)go_subdir_import_b$(TARGETSUFFIX)' INTERFACEDIR='$(INTERFACEDIR)' \
	INTERFACE='testdir/go_subdir_import/go_subdir_import_b.i' \
	GOMOD="go_subdir_import" \
	$(LANGUAGE)$(VARIANT)_cpp;
	+for f in testdir/go_subdir_import/go_subdir_import_c go_subdir_import_a ; do \
	  $(call swig_and_compile_cpp_helper,$${f},'$(SWIGOPT)') GOMOD="go_subdir_import"; \
	done
	$(run_multi_testcase)

go_multicpp_setup = \
	mkdir -p gopath/$*/src 2>/dev/null || true; \
	if ! test -d gopath/$*/src/swigtests; then \
	  (cd gopath/$*/src && ln -s . swigtests); \
	fi

# Runs the testcase.
run_testcase = \
	if test -f $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX); then \
	  GOPATH=`pwd`/gopath/; \
	  export GOPATH; \
	  CGO_CPPFLAGS="$(CPPFLAGS) $(INCLUDES) -I `cd $(SRCDIR) && pwd` -I `pwd`"; \
	  export CGO_CPPFLAGS; \
	  CGO_CFLAGS="$(CFLAGS)"; \
	  export CGO_CFLAGS; \
	  CGO_CXXFLAGS="$(CXXFLAGS)"; \
	  export CGO_CXXFLAGS; \
	  CGO_LDFLAGS="$(LDFLAGS) -lm"; \
	  export CGO_LDFLAGS; \
	  mkdir gopath/src/swigtests 2>/dev/null || true; \
	  mkdir gopath/src/swigtests/$* 2>/dev/null || true; \
	  cp gopath/src/$*/* gopath/src/swigtests/$*/; \
	  mkdir gopath/src/$*/runme 2>/dev/null || true; \
	  cp $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX) gopath/src/$*/runme/runme.go; \
	  (cd gopath/src/$*/runme && $(COMPILETOOL) $(GO) build `if $(GOGCC); then echo -compiler=gccgo; fi` -o ../../../../$*_runme runme.go); \
	  env LD_LIBRARY_PATH=.:$$LD_LIBRARY_PATH $(RUNTOOL) ./$*_runme; \
	fi

run_testcase_cpp = \
	if test -f $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX); then \
	  GOPATH=`pwd`/gopath/; \
	  export GOPATH; \
	  CGO_CPPFLAGS="$(CPPFLAGS) $(INCLUDES) -I `cd $(SRCDIR) && pwd` -I `pwd`"; \
	  export CGO_CPPFLAGS; \
	  CGO_CFLAGS="$(CFLAGS)"; \
	  export CGO_CFLAGS; \
	  CGO_CXXFLAGS="$(CXXFLAGS)"; \
	  export CGO_CXXFLAGS; \
	  CGO_LDFLAGS="$(LDFLAGS) -lm"; \
	  export CGO_LDFLAGS; \
	  mkdir gopath/src/swigtests 2>/dev/null || true; \
	  mkdir gopath/src/swigtests/$* 2>/dev/null || true; \
	  cp gopath/src/$*/* gopath/src/swigtests/$*/; \
	  mkdir gopath/src/$*/runme 2>/dev/null || true; \
	  cp $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX) gopath/src/$*/runme/runme.go; \
	  (cd gopath/src/$*/runme && $(COMPILETOOL) $(GO) build `if $(GOGCC); then echo -compiler=gccgo; fi` -o ../../../../$*_runme runme.go); \
	  env LD_LIBRARY_PATH=.:$$LD_LIBRARY_PATH $(RUNTOOL) ./$*_runme; \
	fi

run_multi_testcase = \
	if test -f $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX); then \
	  files=`cat $(top_srcdir)/$(EXAMPLES)/$(TEST_SUITE)/$*.list`; \
	  mkdir gopath/$*/src/$* 2>/dev/null || true; \
	  cp $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX) gopath/$*/src/$*; \
	  GOPATH="`pwd`/gopath/$*"; \
	  export GOPATH; \
	  CGO_CPPFLAGS="$(CPPFLAGS) $(INCLUDES) `for f in $$files; do echo -I ../$$f; done`"; \
	  export CGO_CPPFLAGS; \
	  CGO_CFLAGS="$(CFLAGS)"; \
	  export CGO_CFLAGS; \
	  CGO_CXXFLAGS="$(CXXFLAGS)"; \
	  export CGO_CXXFLAGS; \
	  CGO_LDFLAGS="$(LDFLAGS) -lm"; \
	  export CGO_LDFLAGS; \
	  (cd gopath/$*/src/$* && \
	    $(COMPILETOOL) $(GO) build `if $(GOGCC); then echo -compiler=gccgo; fi` -o ../../../../$*_runme) && \
	  env LD_LIBRARY_PATH=.:$$LD_LIBRARY_PATH $(RUNTOOL) ./$*_runme; \
	fi

%.clean:
	@rm -rf $*.go $*_gc.c $*_wrap.* $*_runme $*.gox $*.a

clean:
	$(MAKE) -f $(top_builddir)/$(EXAMPLES)/Makefile SRCDIR='$(SRCDIR)' go_clean
	rm -f clientdata_prop_a.go clientdata_prop_a.gox
	rm -f clientdata_prop_b.go clientdata_prop_b.gox
	rm -f import_stl_a.go import_stl_a.gox
	rm -f import_stl_b.go import_stl_b.gox
	rm -f imports_a.go imports_a.gox imports_b.go imports_b.gox
	rm -f mod_a.go mod_a.gox mod_b.go mod_b.gox
	rm -f multi_import_a.go multi_import_a.gox
	rm -f multi_import_b.go multi_import_b.gox
	rm -f multi_import_d.go multi_import_d.gox
	rm -f packageoption_a.go packageoption_a.gox
	rm -f packageoption_b.go packageoption_b.gox
	rm -f packageoption_c.go packageoption_c.gox
	rm -f template_typedef_cplx2.go template_typedef_cplx2.gox
	rm -rf go_subdir_import_a.go go_subdir_import_a.gox testdir
	rm -rf gopath
